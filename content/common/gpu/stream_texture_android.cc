// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/gpu/stream_texture_android.h"

#include "base/bind.h"
#include "content/common/android/surface_texture_peer.h"
#include "content/common/gpu/gpu_channel.h"
#include "content/common/gpu/gpu_messages.h"
#include "gpu/command_buffer/service/context_group.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "gpu/command_buffer/service/texture_manager.h"
#include "ui/gfx/size.h"

namespace content {

using gpu::gles2::ContextGroup;
using gpu::gles2::GLES2Decoder;
using gpu::gles2::TextureManager;
using gpu::gles2::TextureRef;

// static
int32 StreamTexture::Create(
    GpuCommandBufferStub* owner_stub,
    uint32 client_texture_id) {
  GpuChannel* channel = owner_stub->channel();
  int32 route_id = channel->GenerateRouteID();

  GLES2Decoder* decoder = owner_stub->decoder();
  TextureManager* texture_manager =
      decoder->GetContextGroup()->texture_manager();
  TextureRef* texture = texture_manager->GetTexture(client_texture_id);

  if (texture && (!texture->texture()->target() ||
                  texture->texture()->target() == GL_TEXTURE_EXTERNAL_OES)) {

    // TODO: Ideally a valid image id was returned to the client so that
    // it could then call glBindTexImage2D() for doing the following.
    scoped_refptr<gfx::GLImage> gl_image(
        new StreamTexture(owner_stub, route_id, texture->service_id()));
    gfx::Size size = gl_image->GetSize();
    texture_manager->SetTarget(texture, GL_TEXTURE_EXTERNAL_OES);
    texture_manager->SetLevelInfo(texture,
                                  GL_TEXTURE_EXTERNAL_OES,
                                  0,
                                  GL_RGBA,
                                  size.width(),
                                  size.height(),
                                  1,
                                  0,
                                  GL_RGBA,
                                  GL_UNSIGNED_BYTE,
                                  true);
    texture_manager->SetLevelImage(
        texture, GL_TEXTURE_EXTERNAL_OES, 0, gl_image);
    return route_id;
  }

  return 0;
}

StreamTexture::StreamTexture(GpuCommandBufferStub* owner_stub,
                             int32 route_id,
                             uint32 texture_id)
    : surface_texture_(new gfx::SurfaceTexture(texture_id)),
      size_(0, 0),
      has_updated_(false),
      owner_stub_(owner_stub),
      route_id_(route_id),
      has_listener_(false),
      weak_factory_(this) {
  owner_stub->AddDestructionObserver(this);
  memset(current_matrix_, 0, sizeof(current_matrix_));
  owner_stub->channel()->AddRoute(route_id, this);
}

StreamTexture::~StreamTexture() {
  if (owner_stub_) {
    owner_stub_->RemoveDestructionObserver(this);
    owner_stub_->channel()->RemoveRoute(route_id_);
  }
}

void StreamTexture::OnWillDestroyStub() {
  owner_stub_->RemoveDestructionObserver(this);
  owner_stub_->channel()->RemoveRoute(route_id_);
  owner_stub_ = NULL;

  // If the owner goes away, there is no need to keep the SurfaceTexture around.
  // The GL texture will keep working regardless with the currently bound frame.
  surface_texture_ = NULL;
}

void StreamTexture::Destroy() {
  NOTREACHED();
}

void StreamTexture::WillUseTexImage() {
  if (!owner_stub_)
    return;

  // TODO(sievers): Update also when used in a different context.
  //                Also see crbug.com/309162.
  if (surface_texture_.get()) {
    surface_texture_->UpdateTexImage();
  }

  if (has_listener_) {
    float mtx[16];
    surface_texture_->GetTransformMatrix(mtx);

    // Only query the matrix once we have bound a valid frame.
    if (has_updated_ && memcmp(current_matrix_, mtx, sizeof(mtx)) != 0) {
      memcpy(current_matrix_, mtx, sizeof(mtx));

      GpuStreamTextureMsg_MatrixChanged_Params params;
      memcpy(&params.m00, mtx, sizeof(mtx));
      owner_stub_->channel()->Send(
          new GpuStreamTextureMsg_MatrixChanged(route_id_, params));
    }
  }
}

void StreamTexture::OnFrameAvailable() {
  has_updated_ = true;
  DCHECK(has_listener_);
  if (owner_stub_) {
    owner_stub_->channel()->Send(
        new GpuStreamTextureMsg_FrameAvailable(route_id_));
  }
}

gfx::Size StreamTexture::GetSize() {
  return size_;
}

bool StreamTexture::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(StreamTexture, message)
    IPC_MESSAGE_HANDLER(GpuStreamTextureMsg_StartListening, OnStartListening)
    IPC_MESSAGE_HANDLER(GpuStreamTextureMsg_EstablishPeer, OnEstablishPeer)
    IPC_MESSAGE_HANDLER(GpuStreamTextureMsg_SetSize, OnSetSize)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  DCHECK(handled);
  return handled;
}

void StreamTexture::OnStartListening() {
  DCHECK(!has_listener_);
  has_listener_ = true;
  surface_texture_->SetFrameAvailableCallback(base::Bind(
      &StreamTexture::OnFrameAvailable, weak_factory_.GetWeakPtr()));
}

void StreamTexture::OnEstablishPeer(int32 primary_id, int32 secondary_id) {
  if (!owner_stub_)
    return;

  base::ProcessHandle process = owner_stub_->channel()->renderer_pid();

  SurfaceTexturePeer::GetInstance()->EstablishSurfaceTexturePeer(
      process, surface_texture_, primary_id, secondary_id);
}

}  // namespace content
