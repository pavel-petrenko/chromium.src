// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/message_loop/message_loop.h"
#include "content/child/child_process.h"
#include "content/public/renderer/media_stream_video_sink.h"
#include "content/renderer/media/media_stream.h"
#include "content/renderer/media/mock_media_stream_registry.h"
#include "content/renderer/media/video_source_handler.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/web/WebHeap.h"

namespace content {

static const std::string kTestStreamUrl = "stream_url";
static const std::string kTestVideoTrackId = "video_track_id";
static const std::string kUnknownStreamUrl = "unknown_stream_url";

class VideoSourceHandlerTest : public ::testing::Test,
                               public FrameReaderInterface {
 public:
  VideoSourceHandlerTest() : registry_(new MockMediaStreamRegistry()) {
    handler_.reset(new VideoSourceHandler(registry_.get()));
    registry_->Init(kTestStreamUrl);
    registry_->AddVideoTrack(kTestVideoTrackId);
    EXPECT_FALSE(handler_->GetFirstVideoTrack(kTestStreamUrl).isNull());
  }

  MOCK_METHOD1(GotFrame, void(const scoped_refptr<media::VideoFrame>&));

  void TearDown() override {
    registry_.reset();
    handler_.reset();
    blink::WebHeap::collectAllGarbageForTesting();
  }

  void DeliverFrameForTesting(const scoped_refptr<media::VideoFrame>& frame) {
    handler_->DeliverFrameForTesting(this, frame);
  }

 protected:
  scoped_ptr<VideoSourceHandler> handler_;
  // A ChildProcess and a MessageLoop are both needed to fool the Tracks and
  // Sources inside |registry_| into believing they are on the right threads.
  const ChildProcess child_process_;
  const base::MessageLoop message_loop_;
  scoped_ptr<MockMediaStreamRegistry> registry_;
};

// Open |handler_| and send a VideoFrame to be received at the other side.
TEST_F(VideoSourceHandlerTest, OpenClose) {
  // Unknow url will return false.
  EXPECT_FALSE(handler_->Open(kUnknownStreamUrl, this));
  EXPECT_TRUE(handler_->Open(kTestStreamUrl, this));

  const base::TimeDelta ts = base::TimeDelta::FromMilliseconds(789012);
  const scoped_refptr<media::VideoFrame> captured_frame =
      media::VideoFrame::CreateBlackFrame(gfx::Size(640, 360));
  captured_frame->set_timestamp(ts);

  EXPECT_CALL(*this, GotFrame(captured_frame));
  DeliverFrameForTesting(captured_frame);

  EXPECT_FALSE(handler_->Close(NULL));
  EXPECT_TRUE(handler_->Close(this));
}

TEST_F(VideoSourceHandlerTest, OpenWithoutClose) {
  EXPECT_TRUE(handler_->Open(kTestStreamUrl, this));
}

}  // namespace content
