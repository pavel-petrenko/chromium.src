// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/pepper_video_capture_host.h"

#include "content/renderer/media/media_stream_video_source.h"
#include "content/renderer/pepper/host_globals.h"
#include "content/renderer/pepper/pepper_media_device_manager.h"
#include "content/renderer/pepper/pepper_platform_video_capture.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/pepper/renderer_ppapi_host_impl.h"
#include "content/renderer/render_frame_impl.h"
#include "media/base/limits.h"
#include "media/base/video_frame.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/host_dispatcher.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/host_resource.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_buffer_api.h"

using ppapi::HostResource;
using ppapi::TrackedCallback;
using ppapi::thunk::EnterResourceNoLock;
using ppapi::thunk::PPB_Buffer_API;

namespace {

// Maximum number of buffers to actually allocate.
const uint32_t kMaxBuffers = 20;

}  // namespace

namespace content {

PepperVideoCaptureHost::PepperVideoCaptureHost(RendererPpapiHostImpl* host,
                                               PP_Instance instance,
                                               PP_Resource resource)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
      renderer_ppapi_host_(host),
      buffer_count_hint_(0),
      status_(PP_VIDEO_CAPTURE_STATUS_STOPPED),
      enumeration_helper_(this,
                          PepperMediaDeviceManager::GetForRenderFrame(
                              host->GetRenderFrameForInstance(pp_instance())),
                          PP_DEVICETYPE_DEV_VIDEOCAPTURE,
                          host->GetDocumentURL(instance)) {
}

PepperVideoCaptureHost::~PepperVideoCaptureHost() {
  Close();
}

bool PepperVideoCaptureHost::Init() {
  return !!renderer_ppapi_host_->GetPluginInstance(pp_instance());
}

int32_t PepperVideoCaptureHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  int32_t result = PP_ERROR_FAILED;
  if (enumeration_helper_.HandleResourceMessage(msg, context, &result))
    return result;

  PPAPI_BEGIN_MESSAGE_MAP(PepperVideoCaptureHost, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_VideoCapture_Open, OnOpen)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_VideoCapture_StartCapture,
                                        OnStartCapture)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_VideoCapture_ReuseBuffer,
                                      OnReuseBuffer)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_VideoCapture_StopCapture,
                                        OnStopCapture)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_VideoCapture_Close,
                                        OnClose)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

void PepperVideoCaptureHost::OnInitialized(bool succeeded) {
  if (succeeded) {
    open_reply_context_.params.set_result(PP_OK);
  } else {
    DetachPlatformVideoCapture();
    open_reply_context_.params.set_result(PP_ERROR_FAILED);
  }

  host()->SendReply(open_reply_context_,
                    PpapiPluginMsg_VideoCapture_OpenReply());
}

void PepperVideoCaptureHost::OnStarted() {
  if (SetStatus(PP_VIDEO_CAPTURE_STATUS_STARTED, false))
    SendStatus();
}

void PepperVideoCaptureHost::OnStopped() {
  if (SetStatus(PP_VIDEO_CAPTURE_STATUS_STOPPED, false))
    SendStatus();
}

void PepperVideoCaptureHost::OnPaused() {
  if (SetStatus(PP_VIDEO_CAPTURE_STATUS_PAUSED, false))
    SendStatus();
}

void PepperVideoCaptureHost::OnError() {
  PostErrorReply();
}

void PepperVideoCaptureHost::PostErrorReply() {
  // It either comes because some error was detected while starting (e.g. 2
  // conflicting "master" resolution), or because the browser failed to start
  // the capture.
  SetStatus(PP_VIDEO_CAPTURE_STATUS_STOPPED, true);
  host()->SendUnsolicitedReply(
      pp_resource(),
      PpapiPluginMsg_VideoCapture_OnError(
          static_cast<uint32_t>(PP_ERROR_FAILED)));
}

void PepperVideoCaptureHost::OnFrameReady(
    const scoped_refptr<media::VideoFrame>& frame) {
  DCHECK(frame.get());

  if (alloc_size_ != frame->visible_rect().size() || buffers_.empty()) {
    alloc_size_ = frame->visible_rect().size();
    double frame_rate;
    int rounded_frame_rate;
    if (frame->metadata()->GetDouble(media::VideoFrameMetadata::FRAME_RATE,
                                     &frame_rate))
      rounded_frame_rate = static_cast<int>(frame_rate + 0.5 /* round */);
    else
      rounded_frame_rate = MediaStreamVideoSource::kUnknownFrameRate;
    AllocBuffers(alloc_size_, rounded_frame_rate);
  }

  for (uint32_t i = 0; i < buffers_.size(); ++i) {
    if (!buffers_[i].in_use) {
      DCHECK_EQ(frame->format(), media::VideoFrame::I420);
      if (buffers_[i].buffer->size() <
          media::VideoFrame::AllocationSize(frame->format(), alloc_size_)) {
        // TODO(ihf): handle size mismatches gracefully here.
        return;
      }
      uint8* dst = reinterpret_cast<uint8*>(buffers_[i].data);
      static_assert(media::VideoFrame::kYPlane == 0, "y plane should be 0");
      static_assert(media::VideoFrame::kUPlane == 1, "u plane should be 1");
      static_assert(media::VideoFrame::kVPlane == 2, "v plane should be 2");
      for (size_t j = 0; j < media::VideoFrame::NumPlanes(frame->format());
           ++j) {
        const uint8* src = frame->visible_data(j);
        const size_t row_bytes = frame->row_bytes(j);
        const size_t src_stride = frame->stride(j);
        for (int k = 0; k < frame->rows(j); ++k) {
          memcpy(dst, src, row_bytes);
          dst += row_bytes;
          src += src_stride;
        }
      }
      buffers_[i].in_use = true;
      host()->SendUnsolicitedReply(
          pp_resource(), PpapiPluginMsg_VideoCapture_OnBufferReady(i));
      return;
    }
  }
}

void PepperVideoCaptureHost::AllocBuffers(const gfx::Size& resolution,
                                          int frame_rate) {
  PP_VideoCaptureDeviceInfo_Dev info = {
      static_cast<uint32_t>(resolution.width()),
      static_cast<uint32_t>(resolution.height()),
      static_cast<uint32_t>(frame_rate)};
  ReleaseBuffers();

  const size_t size = media::VideoFrame::AllocationSize(
      media::VideoFrame::I420, gfx::Size(info.width, info.height));

  ppapi::proxy::ResourceMessageReplyParams params(pp_resource(), 0);

  // Allocate buffers. We keep a reference to them, that is released in
  // ReleaseBuffers. In the mean time, we prepare the resource and handle here
  // for sending below.
  std::vector<HostResource> buffer_host_resources;
  buffers_.reserve(buffer_count_hint_);
  ppapi::ResourceTracker* tracker = HostGlobals::Get()->GetResourceTracker();
  ppapi::proxy::HostDispatcher* dispatcher =
      ppapi::proxy::HostDispatcher::GetForInstance(pp_instance());
  for (size_t i = 0; i < buffer_count_hint_; ++i) {
    PP_Resource res = PPB_Buffer_Impl::Create(pp_instance(), size);
    if (!res)
      break;

    EnterResourceNoLock<PPB_Buffer_API> enter(res, true);
    DCHECK(enter.succeeded());

    BufferInfo buf;
    buf.buffer = static_cast<PPB_Buffer_Impl*>(enter.object());
    buf.data = buf.buffer->Map();
    if (!buf.data) {
      tracker->ReleaseResource(res);
      break;
    }
    buffers_.push_back(buf);

    // Add to HostResource array to be sent.
    {
      HostResource host_resource;
      host_resource.SetHostResource(pp_instance(), res);
      buffer_host_resources.push_back(host_resource);

      // Add a reference for the plugin, which is resposible for releasing it.
      tracker->AddRefResource(res);
    }

    // Add the serialized shared memory handle to params. FileDescriptor is
    // treated in special case.
    {
      EnterResourceNoLock<PPB_Buffer_API> enter(res, true);
      DCHECK(enter.succeeded());
      base::SharedMemory* shm;
      int32_t result = enter.object()->GetSharedMemory(&shm);
      DCHECK(result == PP_OK);
      params.AppendHandle(ppapi::proxy::SerializedHandle(
          dispatcher->ShareSharedMemoryHandleWithRemote(shm->handle()), size));
    }
  }

  if (buffers_.empty()) {
    // We couldn't allocate/map buffers at all. Send an error and stop the
    // capture.
    SetStatus(PP_VIDEO_CAPTURE_STATUS_STOPPING, true);
    platform_video_capture_->StopCapture();
    PostErrorReply();
    return;
  }

  host()->Send(
      new PpapiPluginMsg_ResourceReply(params,
                                       PpapiPluginMsg_VideoCapture_OnDeviceInfo(
                                           info, buffer_host_resources, size)));
}

int32_t PepperVideoCaptureHost::OnOpen(
    ppapi::host::HostMessageContext* context,
    const std::string& device_id,
    const PP_VideoCaptureDeviceInfo_Dev& requested_info,
    uint32_t buffer_count) {
  if (platform_video_capture_.get())
    return PP_ERROR_FAILED;

  SetRequestedInfo(requested_info, buffer_count);

  GURL document_url = renderer_ppapi_host_->GetDocumentURL(pp_instance());
  if (!document_url.is_valid())
    return PP_ERROR_FAILED;

  platform_video_capture_.reset(new PepperPlatformVideoCapture(
      renderer_ppapi_host_->GetRenderFrameForInstance(pp_instance())->
          GetRoutingID(),
      device_id,
      document_url,
      this));

  open_reply_context_ = context->MakeReplyMessageContext();

  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperVideoCaptureHost::OnStartCapture(
    ppapi::host::HostMessageContext* context) {
  if (!SetStatus(PP_VIDEO_CAPTURE_STATUS_STARTING, false) ||
      !platform_video_capture_.get())
    return PP_ERROR_FAILED;

  DCHECK(buffers_.empty());

  // It's safe to call this regardless it's capturing or not, because
  // PepperPlatformVideoCapture maintains the state.
  platform_video_capture_->StartCapture(video_capture_params_);
  return PP_OK;
}

int32_t PepperVideoCaptureHost::OnReuseBuffer(
    ppapi::host::HostMessageContext* context,
    uint32_t buffer) {
  if (buffer >= buffers_.size() || !buffers_[buffer].in_use)
    return PP_ERROR_BADARGUMENT;
  buffers_[buffer].in_use = false;
  return PP_OK;
}

int32_t PepperVideoCaptureHost::OnStopCapture(
    ppapi::host::HostMessageContext* context) {
  return StopCapture();
}

int32_t PepperVideoCaptureHost::OnClose(
    ppapi::host::HostMessageContext* context) {
  return Close();
}

int32_t PepperVideoCaptureHost::StopCapture() {
  if (!SetStatus(PP_VIDEO_CAPTURE_STATUS_STOPPING, false))
    return PP_ERROR_FAILED;

  DCHECK(platform_video_capture_.get());

  ReleaseBuffers();
  // It's safe to call this regardless it's capturing or not, because
  // PepperPlatformVideoCapture maintains the state.
  platform_video_capture_->StopCapture();
  return PP_OK;
}

int32_t PepperVideoCaptureHost::Close() {
  if (!platform_video_capture_.get())
    return PP_OK;

  StopCapture();
  DCHECK(buffers_.empty());
  DetachPlatformVideoCapture();
  return PP_OK;
}

void PepperVideoCaptureHost::ReleaseBuffers() {
  ppapi::ResourceTracker* tracker = HostGlobals::Get()->GetResourceTracker();
  for (size_t i = 0; i < buffers_.size(); ++i) {
    buffers_[i].buffer->Unmap();
    tracker->ReleaseResource(buffers_[i].buffer->pp_resource());
  }
  buffers_.clear();
}

void PepperVideoCaptureHost::SendStatus() {
  host()->SendUnsolicitedReply(pp_resource(),
                               PpapiPluginMsg_VideoCapture_OnStatus(status_));
}

void PepperVideoCaptureHost::SetRequestedInfo(
    const PP_VideoCaptureDeviceInfo_Dev& device_info,
    uint32_t buffer_count) {
  // Clamp the buffer count to between 1 and |kMaxBuffers|.
  buffer_count_hint_ = std::min(std::max(buffer_count, 1U), kMaxBuffers);
  // Clamp the frame rate to between 1 and |kMaxFramesPerSecond - 1|.
  int frames_per_second =
      std::min(std::max(device_info.frames_per_second, 1U),
               static_cast<uint32_t>(media::limits::kMaxFramesPerSecond - 1));

  video_capture_params_.requested_format = media::VideoCaptureFormat(
      gfx::Size(device_info.width, device_info.height),
      frames_per_second,
      media::PIXEL_FORMAT_I420);
}

void PepperVideoCaptureHost::DetachPlatformVideoCapture() {
  if (platform_video_capture_) {
    platform_video_capture_->DetachEventHandler();
    platform_video_capture_.reset();
  }
}

bool PepperVideoCaptureHost::SetStatus(PP_VideoCaptureStatus_Dev status,
                                       bool forced) {
  if (!forced) {
    switch (status) {
      case PP_VIDEO_CAPTURE_STATUS_STOPPED:
        if (status_ != PP_VIDEO_CAPTURE_STATUS_STOPPING)
          return false;
        break;
      case PP_VIDEO_CAPTURE_STATUS_STARTING:
        if (status_ != PP_VIDEO_CAPTURE_STATUS_STOPPED)
          return false;
        break;
      case PP_VIDEO_CAPTURE_STATUS_STARTED:
        switch (status_) {
          case PP_VIDEO_CAPTURE_STATUS_STARTING:
          case PP_VIDEO_CAPTURE_STATUS_PAUSED:
            break;
          default:
            return false;
        }
        break;
      case PP_VIDEO_CAPTURE_STATUS_PAUSED:
        switch (status_) {
          case PP_VIDEO_CAPTURE_STATUS_STARTING:
          case PP_VIDEO_CAPTURE_STATUS_STARTED:
            break;
          default:
            return false;
        }
        break;
      case PP_VIDEO_CAPTURE_STATUS_STOPPING:
        switch (status_) {
          case PP_VIDEO_CAPTURE_STATUS_STARTING:
          case PP_VIDEO_CAPTURE_STATUS_STARTED:
          case PP_VIDEO_CAPTURE_STATUS_PAUSED:
            break;
          default:
            return false;
        }
        break;
    }
  }

  status_ = status;
  return true;
}

PepperVideoCaptureHost::BufferInfo::BufferInfo()
    : in_use(false), data(NULL), buffer() {
}

PepperVideoCaptureHost::BufferInfo::~BufferInfo() {
}

}  // namespace content
