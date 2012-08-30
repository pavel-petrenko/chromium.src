// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/input_event.h"

#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/point.h"
#include "ppapi/cpp/touch_point.h"
#include "ppapi/cpp/var.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_InputEvent_1_0>() {
  return PPB_INPUT_EVENT_INTERFACE_1_0;
}

template <> const char* interface_name<PPB_KeyboardInputEvent_1_0>() {
  return PPB_KEYBOARD_INPUT_EVENT_INTERFACE_1_0;
}

template <> const char* interface_name<PPB_MouseInputEvent_1_1>() {
  return PPB_MOUSE_INPUT_EVENT_INTERFACE_1_1;
}

template <> const char* interface_name<PPB_WheelInputEvent_1_0>() {
  return PPB_WHEEL_INPUT_EVENT_INTERFACE_1_0;
}

template <> const char* interface_name<PPB_TouchInputEvent_1_0>() {
  return PPB_TOUCH_INPUT_EVENT_INTERFACE_1_0;
}

}  // namespace

// InputEvent ------------------------------------------------------------------

InputEvent::InputEvent() : Resource() {
}

InputEvent::InputEvent(PP_Resource input_event_resource) : Resource() {
  // Type check the input event before setting it.
  if (!has_interface<PPB_InputEvent_1_0>())
    return;
  if (get_interface<PPB_InputEvent_1_0>()->IsInputEvent(input_event_resource)) {
    Module::Get()->core()->AddRefResource(input_event_resource);
    PassRefFromConstructor(input_event_resource);
  }
}

InputEvent::~InputEvent() {
}

PP_InputEvent_Type InputEvent::GetType() const {
  if (!has_interface<PPB_InputEvent_1_0>())
    return PP_INPUTEVENT_TYPE_UNDEFINED;
  return get_interface<PPB_InputEvent_1_0>()->GetType(pp_resource());
}

PP_TimeTicks InputEvent::GetTimeStamp() const {
  if (!has_interface<PPB_InputEvent_1_0>())
    return 0.0f;
  return get_interface<PPB_InputEvent_1_0>()->GetTimeStamp(pp_resource());
}

uint32_t InputEvent::GetModifiers() const {
  if (!has_interface<PPB_InputEvent_1_0>())
    return 0;
  return get_interface<PPB_InputEvent_1_0>()->GetModifiers(pp_resource());
}

// MouseInputEvent -------------------------------------------------------------

MouseInputEvent::MouseInputEvent() : InputEvent() {
}

MouseInputEvent::MouseInputEvent(const InputEvent& event) : InputEvent() {
  // Type check the input event before setting it.
  if (!has_interface<PPB_MouseInputEvent_1_1>())
    return;
  if (get_interface<PPB_MouseInputEvent_1_1>()->IsMouseInputEvent(
          event.pp_resource())) {
    Module::Get()->core()->AddRefResource(event.pp_resource());
    PassRefFromConstructor(event.pp_resource());
  }
}

MouseInputEvent::MouseInputEvent(const InstanceHandle& instance,
                                 PP_InputEvent_Type type,
                                 PP_TimeTicks time_stamp,
                                 uint32_t modifiers,
                                 PP_InputEvent_MouseButton mouse_button,
                                 const Point& mouse_position,
                                 int32_t click_count,
                                 const Point& mouse_movement) {
  // Type check the input event before setting it.
  if (!has_interface<PPB_MouseInputEvent_1_1>())
    return;
  PassRefFromConstructor(get_interface<PPB_MouseInputEvent_1_1>()->Create(
      instance.pp_instance(), type, time_stamp, modifiers, mouse_button,
      &mouse_position.pp_point(), click_count, &mouse_movement.pp_point()));
}

PP_InputEvent_MouseButton MouseInputEvent::GetButton() const {
  if (!has_interface<PPB_MouseInputEvent_1_1>())
    return PP_INPUTEVENT_MOUSEBUTTON_NONE;
  return get_interface<PPB_MouseInputEvent_1_1>()->GetButton(pp_resource());
}

Point MouseInputEvent::GetPosition() const {
  if (!has_interface<PPB_MouseInputEvent_1_1>())
    return Point();
  return get_interface<PPB_MouseInputEvent_1_1>()->GetPosition(pp_resource());
}

int32_t MouseInputEvent::GetClickCount() const {
  if (!has_interface<PPB_MouseInputEvent_1_1>())
    return 0;
  return get_interface<PPB_MouseInputEvent_1_1>()->GetClickCount(pp_resource());
}

Point MouseInputEvent::GetMovement() const {
  if (!has_interface<PPB_MouseInputEvent_1_1>())
    return Point();
  return get_interface<PPB_MouseInputEvent_1_1>()->GetMovement(pp_resource());
}

// WheelInputEvent -------------------------------------------------------------

WheelInputEvent::WheelInputEvent() : InputEvent() {
}

WheelInputEvent::WheelInputEvent(const InputEvent& event) : InputEvent() {
  // Type check the input event before setting it.
  if (!has_interface<PPB_WheelInputEvent_1_0>())
    return;
  if (get_interface<PPB_WheelInputEvent_1_0>()->IsWheelInputEvent(
          event.pp_resource())) {
    Module::Get()->core()->AddRefResource(event.pp_resource());
    PassRefFromConstructor(event.pp_resource());
  }
}

WheelInputEvent::WheelInputEvent(const InstanceHandle& instance,
                                 PP_TimeTicks time_stamp,
                                 uint32_t modifiers,
                                 const FloatPoint& wheel_delta,
                                 const FloatPoint& wheel_ticks,
                                 bool scroll_by_page) {
  // Type check the input event before setting it.
  if (!has_interface<PPB_WheelInputEvent_1_0>())
    return;
  PassRefFromConstructor(get_interface<PPB_WheelInputEvent_1_0>()->Create(
      instance.pp_instance(), time_stamp, modifiers,
      &wheel_delta.pp_float_point(), &wheel_ticks.pp_float_point(),
      PP_FromBool(scroll_by_page)));
}

FloatPoint WheelInputEvent::GetDelta() const {
  if (!has_interface<PPB_WheelInputEvent_1_0>())
    return FloatPoint();
  return get_interface<PPB_WheelInputEvent_1_0>()->GetDelta(pp_resource());
}

FloatPoint WheelInputEvent::GetTicks() const {
  if (!has_interface<PPB_WheelInputEvent_1_0>())
    return FloatPoint();
  return get_interface<PPB_WheelInputEvent_1_0>()->GetTicks(pp_resource());
}

bool WheelInputEvent::GetScrollByPage() const {
  if (!has_interface<PPB_WheelInputEvent_1_0>())
    return false;
  return PP_ToBool(
      get_interface<PPB_WheelInputEvent_1_0>()->GetScrollByPage(pp_resource()));
}

// KeyboardInputEvent ----------------------------------------------------------

KeyboardInputEvent::KeyboardInputEvent() : InputEvent() {
}

KeyboardInputEvent::KeyboardInputEvent(const InputEvent& event) : InputEvent() {
  // Type check the input event before setting it.
  if (!has_interface<PPB_KeyboardInputEvent_1_0>())
    return;
  if (get_interface<PPB_KeyboardInputEvent_1_0>()->IsKeyboardInputEvent(
          event.pp_resource())) {
    Module::Get()->core()->AddRefResource(event.pp_resource());
    PassRefFromConstructor(event.pp_resource());
  }
}

KeyboardInputEvent::KeyboardInputEvent(const InstanceHandle& instance,
                                       PP_InputEvent_Type type,
                                       PP_TimeTicks time_stamp,
                                       uint32_t modifiers,
                                       uint32_t key_code,
                                       const Var& character_text) {
  // Type check the input event before setting it.
  if (!has_interface<PPB_KeyboardInputEvent_1_0>())
    return;
  PassRefFromConstructor(get_interface<PPB_KeyboardInputEvent_1_0>()->Create(
      instance.pp_instance(), type, time_stamp, modifiers, key_code,
      character_text.pp_var()));
}

uint32_t KeyboardInputEvent::GetKeyCode() const {
  if (!has_interface<PPB_KeyboardInputEvent_1_0>())
    return 0;
  return get_interface<PPB_KeyboardInputEvent_1_0>()->GetKeyCode(pp_resource());
}

Var KeyboardInputEvent::GetCharacterText() const {
  if (!has_interface<PPB_KeyboardInputEvent_1_0>())
    return Var();
  return Var(PASS_REF,
             get_interface<PPB_KeyboardInputEvent_1_0>()->GetCharacterText(
                 pp_resource()));
}

// TouchInputEvent ------------------------------------------------------------
TouchInputEvent::TouchInputEvent() : InputEvent() {
}

TouchInputEvent::TouchInputEvent(const InputEvent& event) : InputEvent() {
  if (!has_interface<PPB_TouchInputEvent_1_0>())
    return;
  // Type check the input event before setting it.
  if (get_interface<PPB_TouchInputEvent_1_0>()->IsTouchInputEvent(
      event.pp_resource())) {
    Module::Get()->core()->AddRefResource(event.pp_resource());
    PassRefFromConstructor(event.pp_resource());
  }
}

void TouchInputEvent::AddTouchPoint(PP_TouchListType list,
                                    PP_TouchPoint point) {
  if (!has_interface<PPB_TouchInputEvent_1_0>())
    return;
  get_interface<PPB_TouchInputEvent_1_0>()->AddTouchPoint(pp_resource(), list,
                                                          &point);
}

uint32_t TouchInputEvent::GetTouchCount(PP_TouchListType list) const {
  if (!has_interface<PPB_TouchInputEvent_1_0>())
    return 0;
  return get_interface<PPB_TouchInputEvent_1_0>()->GetTouchCount(pp_resource(),
                                                                 list);
}

TouchPoint TouchInputEvent::GetTouchById(PP_TouchListType list,
                                             uint32_t id) const {
  if (!has_interface<PPB_TouchInputEvent_1_0>())
    return TouchPoint();
  return TouchPoint(get_interface<PPB_TouchInputEvent_1_0>()->
                        GetTouchById(pp_resource(), list, id));
}

TouchPoint TouchInputEvent::GetTouchByIndex(PP_TouchListType list,
                                                uint32_t index) const {
  if (!has_interface<PPB_TouchInputEvent_1_0>())
    return TouchPoint();
  return TouchPoint(get_interface<PPB_TouchInputEvent_1_0>()->
                        GetTouchByIndex(pp_resource(), list, index));
}

}  // namespace pp
