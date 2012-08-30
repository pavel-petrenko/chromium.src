// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_RECTANGLE_UPDATE_DECODER_H_
#define REMOTING_CLIENT_RECTANGLE_UPDATE_DECODER_H_

#include <list>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "remoting/codec/video_decoder.h"
#include "remoting/client/chromoting_stats.h"
#include "remoting/client/frame_consumer_proxy.h"
#include "remoting/client/frame_producer.h"
#include "remoting/protocol/video_stub.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace pp {
class ImageData;
};

namespace remoting {

class ChromotingStats;
class VideoPacket;

namespace protocol {
class SessionConfig;
}  // namespace protocol

// TODO(ajwong): Re-examine this API, especially with regards to how error
// conditions on each step are reported.  Should they be CHECKs? Logs? Other?
// TODO(sergeyu): Rename this class.
class RectangleUpdateDecoder
    : public base::RefCountedThreadSafe<RectangleUpdateDecoder>,
      public FrameProducer,
      public protocol::VideoStub {
 public:
  // Creates an update decoder on |main_task_runner_| and |decode_task_runner_|,
  // outputting to |consumer|. The |main_task_runner_| is responsible for
  // receiving and queueing packets. The |decode_task_runner_| is responsible
  // for decoding the video packets.
  // TODO(wez): Replace the ref-counted proxy with an owned FrameConsumer.
  RectangleUpdateDecoder(
      scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> decode_task_runner,
      scoped_refptr<FrameConsumerProxy> consumer);

  // Initializes decoder with the information from the protocol config.
  void Initialize(const protocol::SessionConfig& config);

  // Removes all video packets in the queue.
  void DropAllPackets();

  // FrameProducer implementation.  These methods may be called before we are
  // Initialize()d, or we know the source screen size.
  virtual void DrawBuffer(pp::ImageData* buffer) OVERRIDE;
  virtual void InvalidateRegion(const SkRegion& region) OVERRIDE;
  virtual void RequestReturnBuffers(const base::Closure& done) OVERRIDE;
  virtual void SetOutputSizeAndClip(const SkISize& view_size,
                                    const SkIRect& clip_area) OVERRIDE;

  // VideoStub implementation.
  virtual void ProcessVideoPacket(scoped_ptr<VideoPacket> packet,
                                  const base::Closure& done) OVERRIDE;
  virtual int GetPendingVideoPackets() OVERRIDE;

  // Return the stats recorded by this client.
  ChromotingStats* GetStats();

 private:
  struct QueuedVideoPacket {
    QueuedVideoPacket(scoped_ptr<VideoPacket> packet,
                      const base::Closure& done);
    ~QueuedVideoPacket();
    VideoPacket* packet;
    base::Closure done;
  };

  friend class base::RefCountedThreadSafe<RectangleUpdateDecoder>;
  virtual ~RectangleUpdateDecoder();

  // Paints the invalidated region to the next available buffer and returns it
  // to the consumer.
  void SchedulePaint();
  void DoPaint();

  // If a packet is not being processed, dispatches a single message from the
  // |received_packets_| queue.
  void ProcessNextPacket();

  // Decodes the contents of |packet|. DecodePacket may keep a reference to
  // |packet| so the |packet| must remain alive and valid until |done| is
  // executed.
  void DecodePacket(scoped_ptr<VideoPacket> packet, const base::Closure& done);

  // Callback method when a VideoPacket is processed.
  // If |last_packet| is true then |decode_start| contains the timestamp when
  // the packet will start to be processed.
  void OnPacketDone(bool last_packet, base::Time decode_start);

  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> decode_task_runner_;
  scoped_refptr<FrameConsumerProxy> consumer_;
  scoped_ptr<VideoDecoder> decoder_;

  // Remote screen size in pixels.
  SkISize source_size_;

  // Vertical and horizontal DPI of the remote screen.
  SkIPoint source_dpi_;

  // The current dimensions of the frame consumer view.
  SkISize view_size_;
  SkIRect clip_area_;

  // The drawing buffers supplied by the frame consumer.
  std::list<pp::ImageData*> buffers_;

  // Flag used to coalesce runs of SchedulePaint()s into a single DoPaint().
  bool paint_scheduled_;

  // Contains all video packets that have been received, but have not yet been
  // processed.
  //
  // Used to serialize sending of messages to the client.
  // TODO(sergeyu): Simplify this code and remove this list.
  std::list<QueuedVideoPacket> received_packets_;

  // True if a message is being processed. Can be used to determine if it is
  // safe to dispatch another message.
  bool packet_being_processed_;

  ChromotingStats stats_;

  // Keep track of the most recent sequence number bounced back from the host.
  int64 latest_sequence_number_;
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_RECTANGLE_UPDATE_DECODER_H_
