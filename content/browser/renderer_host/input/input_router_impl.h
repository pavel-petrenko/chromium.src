// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_INPUT_INPUT_ROUTER_IMPL_H_
#define CONTENT_BROWSER_RENDERER_HOST_INPUT_INPUT_ROUTER_IMPL_H_

#include <queue>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/time/time.h"
#include "content/browser/renderer_host/input/gesture_event_queue.h"
#include "content/browser/renderer_host/input/input_router.h"
#include "content/browser/renderer_host/input/touch_action_filter.h"
#include "content/browser/renderer_host/input/touch_event_queue.h"
#include "content/browser/renderer_host/input/touchpad_tap_suppression_controller.h"
#include "content/common/input/input_event_stream_validator.h"
#include "content/public/browser/native_web_keyboard_event.h"

namespace IPC {
class Sender;
}

namespace ui {
class LatencyInfo;
}

namespace content {

class InputAckHandler;
class InputRouterClient;
class OverscrollController;
struct DidOverscrollParams;
struct InputEventAck;

// A default implementation for browser input event routing.
class CONTENT_EXPORT InputRouterImpl
    : public NON_EXPORTED_BASE(InputRouter),
      public NON_EXPORTED_BASE(GestureEventQueueClient),
      public NON_EXPORTED_BASE(TouchEventQueueClient),
      public NON_EXPORTED_BASE(TouchpadTapSuppressionControllerClient) {
 public:
  struct CONTENT_EXPORT Config {
    Config();
    GestureEventQueue::Config gesture_config;
    TouchEventQueue::Config touch_config;
  };

  InputRouterImpl(IPC::Sender* sender,
                  InputRouterClient* client,
                  InputAckHandler* ack_handler,
                  int routing_id,
                  const Config& config);
  ~InputRouterImpl() override;

  // InputRouter
  bool SendInput(scoped_ptr<IPC::Message> message) override;
  void SendMouseEvent(const MouseEventWithLatencyInfo& mouse_event) override;
  void SendWheelEvent(
      const MouseWheelEventWithLatencyInfo& wheel_event) override;
  void SendKeyboardEvent(const NativeWebKeyboardEvent& key_event,
                         const ui::LatencyInfo& latency_info,
                         bool is_keyboard_shortcut) override;
  void SendGestureEvent(
      const GestureEventWithLatencyInfo& gesture_event) override;
  void SendTouchEvent(const TouchEventWithLatencyInfo& touch_event) override;
  const NativeWebKeyboardEvent* GetLastKeyboardEvent() const override;
  void NotifySiteIsMobileOptimized(bool is_mobile_optimized) override;
  void RequestNotificationWhenFlushed() override;
  bool HasPendingEvents() const override;

  // IPC::Listener
  bool OnMessageReceived(const IPC::Message& message) override;

private:
  friend class InputRouterImplTest;

  // TouchpadTapSuppressionControllerClient
  void SendMouseEventImmediately(
      const MouseEventWithLatencyInfo& mouse_event) override;

  // TouchEventQueueClient
  void SendTouchEventImmediately(
      const TouchEventWithLatencyInfo& touch_event) override;
  void OnTouchEventAck(const TouchEventWithLatencyInfo& event,
                       InputEventAckState ack_result) override;

  // GetureEventFilterClient
  void SendGestureEventImmediately(
      const GestureEventWithLatencyInfo& gesture_event) override;
  void OnGestureEventAck(const GestureEventWithLatencyInfo& event,
                         InputEventAckState ack_result) override;

  bool SendMoveCaret(scoped_ptr<IPC::Message> message);
  bool SendSelectMessage(scoped_ptr<IPC::Message> message);
  bool Send(IPC::Message* message);

  // Filters and forwards |input_event| to the appropriate handler.
  void FilterAndSendWebInputEvent(const blink::WebInputEvent& input_event,
                                  const ui::LatencyInfo& latency_info,
                                  bool is_keyboard_shortcut);

  // Utility routine for filtering and forwarding |input_event| to the
  // appropriate handler. |input_event| will be offered to the overscroll
  // controller, client and renderer, in that order.
  void OfferToHandlers(const blink::WebInputEvent& input_event,
                       const ui::LatencyInfo& latency_info,
                       bool is_keyboard_shortcut);

  // Returns true if |input_event| was consumed by the overscroll controller.
  bool OfferToOverscrollController(const blink::WebInputEvent& input_event,
                                   const ui::LatencyInfo& latency_info);

  // Returns true if |input_event| was consumed by the client.
  bool OfferToClient(const blink::WebInputEvent& input_event,
                     const ui::LatencyInfo& latency_info);

  // Returns true if |input_event| was successfully sent to the renderer
  // as an async IPC Message.
  bool OfferToRenderer(const blink::WebInputEvent& input_event,
                       const ui::LatencyInfo& latency_info,
                       bool is_keyboard_shortcut);

  // IPC message handlers
  void OnInputEventAck(const InputEventAck& ack);
  void OnDidOverscroll(const DidOverscrollParams& params);
  void OnMsgMoveCaretAck();
  void OnSelectMessageAck();
  void OnHasTouchEventHandlers(bool has_handlers);
  void OnSetTouchAction(TouchAction touch_action);
  void OnDidStopFlinging();

  // Indicates the source of an ack provided to |ProcessInputEventAck()|.
  // The source is tracked by |current_ack_source_|, which aids in ack routing.
  enum AckSource {
    RENDERER,
    CLIENT,
    IGNORING_DISPOSITION,
    ACK_SOURCE_NONE
  };
  // Note: This function may result in |this| being deleted, and as such
  // should be the last method called in any internal chain of event handling.
  void ProcessInputEventAck(blink::WebInputEvent::Type event_type,
                            InputEventAckState ack_result,
                            const ui::LatencyInfo& latency_info,
                            uint32 unique_touch_event_id,
                            AckSource ack_source);

  // Dispatches the ack'ed event to |ack_handler_|.
  void ProcessKeyboardAck(blink::WebInputEvent::Type type,
                          InputEventAckState ack_result);

  // Forwards a valid |next_mouse_move_| if |type| is MouseMove.
  void ProcessMouseAck(blink::WebInputEvent::Type type,
                       InputEventAckState ack_result);

  // Dispatches the ack'ed event to |ack_handler_|, forwarding queued events
  // from |coalesced_mouse_wheel_events_|.
  void ProcessWheelAck(InputEventAckState ack_result,
                       const ui::LatencyInfo& latency);

  // Forwards the event ack to |gesture_event_queue|, potentially triggering
  // dispatch of queued gesture events.
  void ProcessGestureAck(blink::WebInputEvent::Type type,
                         InputEventAckState ack_result,
                         const ui::LatencyInfo& latency);

  // Forwards the event ack to |touch_event_queue_|, potentially triggering
  // dispatch of queued touch events, or the creation of gesture events.
  void ProcessTouchAck(InputEventAckState ack_result,
                       const ui::LatencyInfo& latency,
                       uint32 unique_touch_event_id);

  // Called when a touch timeout-affecting bit has changed, in turn toggling the
  // touch ack timeout feature of the |touch_event_queue_| as appropriate. Input
  // to that determination includes current view properties and the allowed
  // touch action. Note that this will only affect platforms that have a
  // non-zero touch timeout configuration.
  void UpdateTouchAckTimeoutEnabled();

  // If a flush has been requested, signals a completed flush to the client if
  // all events have been dispatched (i.e., |HasPendingEvents()| is false).
  void SignalFlushedIfNecessary();

  int routing_id() const { return routing_id_; }


  IPC::Sender* sender_;
  InputRouterClient* client_;
  InputAckHandler* ack_handler_;
  int routing_id_;

  // (Similar to |mouse_move_pending_|.) True while waiting for SelectRange_ACK
  // or MoveRangeSelectionExtent_ACK.
  bool select_message_pending_;

  // Queue of pending select messages to send after receving the next select
  // message ack.
  std::deque<IPC::Message*> pending_select_messages_;

  // (Similar to |mouse_move_pending_|.) True while waiting for MoveCaret_ACK.
  bool move_caret_pending_;

  // (Similar to |next_mouse_move_|.) The next MoveCaret to send, if any.
  scoped_ptr<IPC::Message> next_move_caret_;

  // True if a mouse move event was sent to the render view and we are waiting
  // for a corresponding InputHostMsg_HandleInputEvent_ACK message.
  bool mouse_move_pending_;

  // The next mouse move event to send (only non-null while mouse_move_pending_
  // is true).
  scoped_ptr<MouseEventWithLatencyInfo> next_mouse_move_;

  // (Similar to |mouse_move_pending_|.) True if a mouse wheel event was sent
  // and we are waiting for a corresponding ack.
  bool mouse_wheel_pending_;
  MouseWheelEventWithLatencyInfo current_wheel_event_;

  // (Similar to |next_mouse_move_|.) The next mouse wheel events to send.
  // Unlike mouse moves, mouse wheel events received while one is pending are
  // coalesced (by accumulating deltas) if they match the previous event in
  // modifiers. On the Mac, in particular, mouse wheel events are received at a
  // high rate; not waiting for the ack results in jankiness, and using the same
  // mechanism as for mouse moves (just dropping old events when multiple ones
  // would be queued) results in very slow scrolling.
  typedef std::deque<MouseWheelEventWithLatencyInfo> WheelEventQueue;
  WheelEventQueue coalesced_mouse_wheel_events_;

  // A queue of keyboard events. We can't trust data from the renderer so we
  // stuff key events into a queue and pop them out on ACK, feeding our copy
  // back to whatever unhandled handler instead of the returned version.
  typedef std::deque<NativeWebKeyboardEvent> KeyQueue;
  KeyQueue key_queue_;

  // The time when an input event was sent to the client.
  base::TimeTicks input_event_start_time_;

  // The source of the ack within the scope of |ProcessInputEventAck()|.
  // Defaults to ACK_SOURCE_NONE.
  AckSource current_ack_source_;

  // Whether a call to |Flush()| has yet been accompanied by a |DidFlush()| call
  // to the client_ after all events have been dispatched/acked.
  bool flush_requested_;

  // Whether there are any active flings in the renderer. As the fling
  // end notification is asynchronous, we use a count rather than a boolean
  // to avoid races in bookkeeping when starting a new fling.
  int active_renderer_fling_count_;

  TouchEventQueue touch_event_queue_;
  GestureEventQueue gesture_event_queue_;
  TouchActionFilter touch_action_filter_;
  InputEventStreamValidator input_stream_validator_;
  InputEventStreamValidator output_stream_validator_;

  DISALLOW_COPY_AND_ASSIGN(InputRouterImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_INPUT_INPUT_ROUTER_IMPL_H_
