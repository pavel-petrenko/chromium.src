// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "content/browser/renderer_host/text_input_client_mac.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread.h"
#include "content/browser/gpu/gpu_surface_tracker.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_delegate.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/text_input_client_message_filter.h"
#include "content/common/text_input_client_messages.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_browser_context.h"
#include "ipc/ipc_test_sink.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"

namespace content {

namespace {
const int64 kTaskDelayMs = 200;

class MockRenderWidgetHostDelegate : public RenderWidgetHostDelegate {
 public:
  MockRenderWidgetHostDelegate() {}
  ~MockRenderWidgetHostDelegate() override {}

 private:
  void Cut() override {}
  void Copy() override {}
  void Paste() override {}
  void SelectAll() override {}
};

// This test does not test the WebKit side of the dictionary system (which
// performs the actual data fetching), but rather this just tests that the
// service's signaling system works.
class TextInputClientMacTest : public testing::Test {
 public:
  TextInputClientMacTest()
      : browser_context_(),
        process_factory_(),
        delegate_(),
        thread_("TextInputClientMacTestThread") {
    RenderProcessHost* rph =
        process_factory_.CreateRenderProcessHost(&browser_context_, nullptr);
    int32 routing_id = rph->GetNextRoutingID();
    int32 surface_id = GpuSurfaceTracker::Get()->AddSurfaceForRenderer(
        rph->GetID(), routing_id);
    widget_.reset(new RenderWidgetHostImpl(&delegate_, rph, routing_id,
                                           surface_id, false));
  }

  // Accessor for the TextInputClientMac instance.
  TextInputClientMac* service() {
    return TextInputClientMac::GetInstance();
  }

  // Helper method to post a task on the testing thread's MessageLoop after
  // a short delay.
  void PostTask(const tracked_objects::Location& from_here,
                const base::Closure& task) {
    PostTask(from_here, task, base::TimeDelta::FromMilliseconds(kTaskDelayMs));
  }

  void PostTask(const tracked_objects::Location& from_here,
                const base::Closure& task,
                const base::TimeDelta delay) {
    thread_.message_loop()->PostDelayedTask(from_here, task, delay);
  }

  RenderWidgetHostImpl* widget() { return widget_.get(); }

  IPC::TestSink& ipc_sink() {
    return static_cast<MockRenderProcessHost*>(widget()->GetProcess())->sink();
  }

 private:
  friend class ScopedTestingThread;

  base::MessageLoopForUI message_loop_;
  TestBrowserContext browser_context_;

  // Gets deleted when the last RWH in the "process" gets destroyed.
  MockRenderProcessHostFactory process_factory_;
  MockRenderWidgetHostDelegate delegate_;
  scoped_ptr<RenderWidgetHostImpl> widget_;

  base::Thread thread_;
};

////////////////////////////////////////////////////////////////////////////////

// Helper class that Start()s and Stop()s a thread according to the scope of the
// object.
class ScopedTestingThread {
 public:
  ScopedTestingThread(TextInputClientMacTest* test) : thread_(test->thread_) {
    thread_.Start();
  }
  ~ScopedTestingThread() {
    thread_.Stop();
  }

 private:
  base::Thread& thread_;
};

// Adapter for OnMessageReceived to ignore return type so it can be posted
// to a MessageLoop.
void CallOnMessageReceived(scoped_refptr<TextInputClientMessageFilter> filter,
                           const IPC::Message& message) {
  filter->OnMessageReceived(message);
}

}  // namespace

// Test Cases //////////////////////////////////////////////////////////////////

TEST_F(TextInputClientMacTest, GetCharacterIndex) {
  ScopedTestingThread thread(this);
  const NSUInteger kSuccessValue = 42;

  PostTask(FROM_HERE,
           base::Bind(&TextInputClientMac::SetCharacterIndexAndSignal,
                      base::Unretained(service()), kSuccessValue));
  NSUInteger index = service()->GetCharacterIndexAtPoint(
      widget(), gfx::Point(2, 2));

  EXPECT_EQ(1U, ipc_sink().message_count());
  EXPECT_TRUE(ipc_sink().GetUniqueMessageMatching(
      TextInputClientMsg_CharacterIndexForPoint::ID));
  EXPECT_EQ(kSuccessValue, index);
}

TEST_F(TextInputClientMacTest, TimeoutCharacterIndex) {
  NSUInteger index = service()->GetCharacterIndexAtPoint(
      widget(), gfx::Point(2, 2));
  EXPECT_EQ(1U, ipc_sink().message_count());
  EXPECT_TRUE(ipc_sink().GetUniqueMessageMatching(
      TextInputClientMsg_CharacterIndexForPoint::ID));
  EXPECT_EQ(NSNotFound, index);
}

TEST_F(TextInputClientMacTest, NotFoundCharacterIndex) {
  ScopedTestingThread thread(this);
  const NSUInteger kPreviousValue = 42;
  const size_t kNotFoundValue = static_cast<size_t>(-1);

  // Set an arbitrary value to ensure the index is not |NSNotFound|.
  PostTask(FROM_HERE,
           base::Bind(&TextInputClientMac::SetCharacterIndexAndSignal,
                      base::Unretained(service()), kPreviousValue));

  scoped_refptr<TextInputClientMessageFilter> filter(
      new TextInputClientMessageFilter(widget()->GetProcess()->GetID()));
  scoped_ptr<IPC::Message> message(
      new TextInputClientReplyMsg_GotCharacterIndexForPoint(
          widget()->GetRoutingID(), kNotFoundValue));
  // Set |WTF::notFound| to the index |kTaskDelayMs| after the previous
  // setting.
  PostTask(FROM_HERE,
           base::Bind(&CallOnMessageReceived, filter, *message),
           base::TimeDelta::FromMilliseconds(kTaskDelayMs) * 2);

  NSUInteger index = service()->GetCharacterIndexAtPoint(
      widget(), gfx::Point(2, 2));
  EXPECT_EQ(kPreviousValue, index);
  index = service()->GetCharacterIndexAtPoint(widget(), gfx::Point(2, 2));
  EXPECT_EQ(NSNotFound, index);

  EXPECT_EQ(2U, ipc_sink().message_count());
  for (size_t i = 0; i < ipc_sink().message_count(); ++i) {
    const IPC::Message* ipc_message = ipc_sink().GetMessageAt(i);
    EXPECT_EQ(ipc_message->type(),
              TextInputClientMsg_CharacterIndexForPoint::ID);
  }
}

TEST_F(TextInputClientMacTest, GetRectForRange) {
  ScopedTestingThread thread(this);
  const NSRect kSuccessValue = NSMakeRect(42, 43, 44, 45);

  PostTask(FROM_HERE,
           base::Bind(&TextInputClientMac::SetFirstRectAndSignal,
                      base::Unretained(service()), kSuccessValue));
  NSRect rect = service()->GetFirstRectForRange(widget(), NSMakeRange(0, 32));

  EXPECT_EQ(1U, ipc_sink().message_count());
  EXPECT_TRUE(ipc_sink().GetUniqueMessageMatching(
      TextInputClientMsg_FirstRectForCharacterRange::ID));
  EXPECT_NSEQ(kSuccessValue, rect);
}

TEST_F(TextInputClientMacTest, TimeoutRectForRange) {
  NSRect rect = service()->GetFirstRectForRange(widget(), NSMakeRange(0, 32));
  EXPECT_EQ(1U, ipc_sink().message_count());
  EXPECT_TRUE(ipc_sink().GetUniqueMessageMatching(
      TextInputClientMsg_FirstRectForCharacterRange::ID));
  EXPECT_NSEQ(NSZeroRect, rect);
}

}  // namespace content
