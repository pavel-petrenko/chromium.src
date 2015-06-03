// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/input_method_base.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "ui/base/ime/input_method_delegate.h"
#include "ui/base/ime/input_method_observer.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/base/ime/text_input_focus_manager.h"
#include "ui/base/ui_base_switches_util.h"
#include "ui/events/event.h"

namespace ui {

InputMethodBase::InputMethodBase()
  : delegate_(NULL),
    text_input_client_(NULL),
    system_toplevel_window_focused_(false) {
}

InputMethodBase::~InputMethodBase() {
  FOR_EACH_OBSERVER(InputMethodObserver,
                    observer_list_,
                    OnInputMethodDestroyed(this));
}

void InputMethodBase::SetDelegate(internal::InputMethodDelegate* delegate) {
  delegate_ = delegate;
}

void InputMethodBase::OnFocus() {
  system_toplevel_window_focused_ = true;
}

void InputMethodBase::OnBlur() {
  system_toplevel_window_focused_ = false;
}

void InputMethodBase::SetFocusedTextInputClient(TextInputClient* client) {
  SetFocusedTextInputClientInternal(client);
}

void InputMethodBase::DetachTextInputClient(TextInputClient* client) {
  if (text_input_client_ != client)
    return;
  SetFocusedTextInputClientInternal(NULL);
}

TextInputClient* InputMethodBase::GetTextInputClient() const {
  if (switches::IsTextInputFocusManagerEnabled())
    return TextInputFocusManager::GetInstance()->GetFocusedTextInputClient();

  return system_toplevel_window_focused_ ? text_input_client_ : NULL;
}

void InputMethodBase::OnTextInputTypeChanged(const TextInputClient* client) {
  if (!IsTextInputClientFocused(client))
    return;
  NotifyTextInputStateChanged(client);
}

TextInputType InputMethodBase::GetTextInputType() const {
  TextInputClient* client = GetTextInputClient();
  return client ? client->GetTextInputType() : TEXT_INPUT_TYPE_NONE;
}

TextInputMode InputMethodBase::GetTextInputMode() const {
  TextInputClient* client = GetTextInputClient();
  return client ? client->GetTextInputMode() : TEXT_INPUT_MODE_DEFAULT;
}

int InputMethodBase::GetTextInputFlags() const {
  TextInputClient* client = GetTextInputClient();
  return client ? client->GetTextInputFlags() : 0;
}

bool InputMethodBase::CanComposeInline() const {
  TextInputClient* client = GetTextInputClient();
  return client ? client->CanComposeInline() : true;
}

void InputMethodBase::ShowImeIfNeeded() {
  FOR_EACH_OBSERVER(InputMethodObserver, observer_list_, OnShowImeIfNeeded());
}

void InputMethodBase::AddObserver(InputMethodObserver* observer) {
  observer_list_.AddObserver(observer);
}

void InputMethodBase::RemoveObserver(InputMethodObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

bool InputMethodBase::IsTextInputClientFocused(const TextInputClient* client) {
  return client && (client == GetTextInputClient());
}

bool InputMethodBase::IsTextInputTypeNone() const {
  return GetTextInputType() == TEXT_INPUT_TYPE_NONE;
}

void InputMethodBase::OnInputMethodChanged() const {
  TextInputClient* client = GetTextInputClient();
  if (!IsTextInputTypeNone())
    client->OnInputMethodChanged();
}

bool InputMethodBase::DispatchKeyEventPostIME(
    const ui::KeyEvent& event) const {
  if (!delegate_)
    return false;

  return delegate_->DispatchKeyEventPostIME(event);
}

void InputMethodBase::NotifyTextInputStateChanged(
    const TextInputClient* client) {
  FOR_EACH_OBSERVER(InputMethodObserver,
                    observer_list_,
                    OnTextInputStateChanged(client));
}

void InputMethodBase::NotifyTextInputCaretBoundsChanged(
    const TextInputClient* client) {
  FOR_EACH_OBSERVER(
      InputMethodObserver, observer_list_, OnCaretBoundsChanged(client));
}

void InputMethodBase::SetFocusedTextInputClientInternal(
    TextInputClient* client) {
  if (switches::IsTextInputFocusManagerEnabled())
    return;

  TextInputClient* old = text_input_client_;
  if (old == client)
    return;
  OnWillChangeFocusedClient(old, client);
  text_input_client_ = client;  // NULL allowed.
  OnDidChangeFocusedClient(old, client);
  NotifyTextInputStateChanged(text_input_client_);
}

}  // namespace ui
