// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_VIDEO_VIDEO_DECODE_ACCELERATOR_H_
#define MEDIA_VIDEO_VIDEO_DECODE_ACCELERATOR_H_

#include <vector>

#include "base/basictypes.h"
#include "base/callback_old.h"
#include "media/base/bitstream_buffer.h"
#include "media/video/picture.h"
#include "ui/gfx/size.h"

namespace media {

// Video decoder interface.
// This interface is extended by the various components that ultimately
// implement the backend of PPB_VideoDecode_Dev.
//
// No thread-safety guarantees are implied by the use of RefCountedThreadSafe
// below.
class MEDIA_EXPORT VideoDecodeAccelerator
    : public base::RefCountedThreadSafe<VideoDecodeAccelerator> {
 public:
  // Video stream profile.  This *must* match PP_VideoDecoder_Profile.
  enum Profile {
    // Keep the values in this enum unique, as they imply format (h.264 vs. VP8,
    // for example), and keep the values for a particular format grouped
    // together for clarity.
    H264PROFILE_MIN = 0,
    H264PROFILE_BASELINE = H264PROFILE_MIN,
    H264PROFILE_MAIN,
    H264PROFILE_EXTENDED,
    H264PROFILE_HIGH,
    H264PROFILE_HIGH10PROFILE,
    H264PROFILE_HIGH422PROFILE,
    H264PROFILE_HIGH444PREDICTIVEPROFILE,
    H264PROFILE_SCALABLEBASELINE,
    H264PROFILE_SCALABLEHIGH,
    H264PROFILE_STEREOHIGH,
    H264PROFILE_MULTIVIEWHIGH,
    H264PROFILE_MAX = H264PROFILE_MULTIVIEWHIGH,
  };

  // Enumeration of potential errors generated by the API.
  // Note: Keep these in sync with PP_VideoDecodeError_Dev.
  enum Error {
    // An operation was attempted during an incompatible decoder state.
    ILLEGAL_STATE = 1,
    // Invalid argument was passed to an API method.
    INVALID_ARGUMENT,
    // Encoded input is unreadable.
    UNREADABLE_INPUT,
    // A failure occurred at the browser layer or one of its dependencies.
    // Examples of such failures include GPU hardware failures, GPU driver
    // failures, GPU library failures, browser programming errors, and so on.
    PLATFORM_FAILURE,
  };

  // Interface for collaborating with picture interface to provide memory for
  // output picture and blitting them.
  // This interface is extended by the various layers that relay messages back
  // to the plugin, through the PPP_VideoDecode_Dev interface the plugin
  // implements.
  class Client {
   public:
    virtual ~Client() {}

    // Callback to notify client that decoder has been initialized.
    virtual void NotifyInitializeDone() = 0;

    // Callback to tell client how many and what size of buffers to provide.
    virtual void ProvidePictureBuffers(
        uint32 requested_num_of_buffers, const gfx::Size& dimensions) = 0;

    // Callback to dismiss picture buffer that was assigned earlier.
    virtual void DismissPictureBuffer(int32 picture_buffer_id) = 0;

    // Callback to deliver decoded pictures ready to be displayed.
    virtual void PictureReady(const Picture& picture) = 0;

    // Callback to notify that decoder has decoded end of stream marker and has
    // outputted all displayable pictures.
    virtual void NotifyEndOfStream() = 0;

    // Callback to notify that decoded has decoded the end of the current
    // bitstream buffer.
    virtual void NotifyEndOfBitstreamBuffer(int32 bitstream_buffer_id) = 0;

    // Flush completion callback.
    virtual void NotifyFlushDone() = 0;

    // Reset completion callback.
    virtual void NotifyResetDone() = 0;

    // Callback to notify about decoding errors.
    virtual void NotifyError(Error error) = 0;
  };

  // Video decoder functions.

  // Initializes the video decoder with specific configuration.
  // Parameters:
  //  |profile| is the video stream's format profile.
  //
  // Returns true when command successfully accepted. Otherwise false.
  virtual bool Initialize(Profile profile) = 0;

  // Decodes given bitstream buffer. Once decoder is done with processing
  // |bitstream_buffer| it will call NotifyEndOfBitstreamBuffer() with the
  // bitstream buffer id.
  // Parameters:
  //  |bitstream_buffer| is the input bitstream that is sent for decoding.
  virtual void Decode(const BitstreamBuffer& bitstream_buffer) = 0;

  // Assigns a set of texture-backed picture buffers to the video decoder.
  //
  // Ownership of each picture buffer remains with the client, but the client
  // is not allowed to deallocate the buffer before the DismissPictureBuffer
  // callback has been initiated for a given buffer.
  //
  // Parameters:
  //  |buffers| contains the allocated picture buffers for the output.
  virtual void AssignPictureBuffers(
      const std::vector<PictureBuffer>& buffers) = 0;

  // Sends picture buffers to be reused by the decoder. This needs to be called
  // for each buffer that has been processed so that decoder may know onto which
  // picture buffers it can write the output to.
  //
  // Parameters:
  //  |picture_buffer_id| id of the picture buffer that is to be reused.
  virtual void ReusePictureBuffer(int32 picture_buffer_id) = 0;

  // Flushes the decoder: all pending inputs will be decoded and pictures handed
  // back to the client, followed by NotifyFlushDone() being called on the
  // client.  Can be used to implement "end of stream" notification.
  virtual void Flush() = 0;

  // Resets the decoder: all pending inputs are dropped immediately and the
  // decoder returned to a state ready for further Decode()s, followed by
  // NotifyResetDone() being called on the client.  Can be used to implement
  // "seek".
  virtual void Reset() = 0;

  // Destroys the decoder: all pending inputs are dropped immediately and the
  // component is freed.  This call may asynchornously free system resources,
  // but its client-visible effects are synchronous.  After this method returns
  // no more callbacks will be made on the client.
  virtual void Destroy() = 0;

 protected:
  friend class base::RefCountedThreadSafe<VideoDecodeAccelerator>;
  virtual ~VideoDecodeAccelerator();
};

}  // namespace media

#endif  // MEDIA_VIDEO_VIDEO_DECODE_ACCELERATOR_H_
