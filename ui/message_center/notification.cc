// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/notification.h"

#include "base/logging.h"
#include "ui/message_center/notification_delegate.h"
#include "ui/message_center/notification_types.h"

namespace {
unsigned g_next_serial_number_ = 0;
}

namespace message_center {

NotificationItem::NotificationItem(const base::string16& title,
                                   const base::string16& message)
 : title(title),
   message(message) {
}

ButtonInfo::ButtonInfo(const base::string16& title)
 : title(title) {
}

RichNotificationData::RichNotificationData()
    : priority(DEFAULT_PRIORITY),
      never_timeout(false),
      timestamp(base::Time::Now()),
      progress(0),
      should_make_spoken_feedback_for_popup_updates(true),
      clickable(true),
      silent(false) {}

RichNotificationData::RichNotificationData(const RichNotificationData& other)
    : priority(other.priority),
      never_timeout(other.never_timeout),
      timestamp(other.timestamp),
      context_message(other.context_message),
      image(other.image),
      small_image(other.small_image),
      items(other.items),
      progress(other.progress),
      buttons(other.buttons),
      should_make_spoken_feedback_for_popup_updates(
          other.should_make_spoken_feedback_for_popup_updates),
      clickable(other.clickable),
      vibration_pattern(other.vibration_pattern),
      silent(other.silent) {}

RichNotificationData::~RichNotificationData() {}

Notification::Notification(NotificationType type,
                           const std::string& id,
                           const base::string16& title,
                           const base::string16& message,
                           const gfx::Image& icon,
                           const base::string16& display_source,
                           const NotifierId& notifier_id,
                           const RichNotificationData& optional_fields,
                           NotificationDelegate* delegate)
    : type_(type),
      id_(id),
      title_(title),
      message_(message),
      icon_(icon),
      display_source_(display_source),
      notifier_id_(notifier_id),
      serial_number_(g_next_serial_number_++),
      optional_fields_(optional_fields),
      shown_as_popup_(false),
      is_read_(false),
      delegate_(delegate) {}

Notification::Notification(const std::string& id, const Notification& other)
    : type_(other.type_),
      id_(id),
      title_(other.title_),
      message_(other.message_),
      icon_(other.icon_),
      display_source_(other.display_source_),
      notifier_id_(other.notifier_id_),
      serial_number_(other.serial_number_),
      optional_fields_(other.optional_fields_),
      shown_as_popup_(other.shown_as_popup_),
      is_read_(other.is_read_),
      delegate_(other.delegate_) {
}

Notification::Notification(const Notification& other)
    : type_(other.type_),
      id_(other.id_),
      title_(other.title_),
      message_(other.message_),
      icon_(other.icon_),
      display_source_(other.display_source_),
      notifier_id_(other.notifier_id_),
      serial_number_(other.serial_number_),
      optional_fields_(other.optional_fields_),
      shown_as_popup_(other.shown_as_popup_),
      is_read_(other.is_read_),
      delegate_(other.delegate_) {}

Notification& Notification::operator=(const Notification& other) {
  type_ = other.type_;
  id_ = other.id_;
  title_ = other.title_;
  message_ = other.message_;
  icon_ = other.icon_;
  display_source_ = other.display_source_;
  notifier_id_ = other.notifier_id_;
  serial_number_ = other.serial_number_;
  optional_fields_ = other.optional_fields_;
  shown_as_popup_ = other.shown_as_popup_;
  is_read_ = other.is_read_;
  delegate_ = other.delegate_;

  return *this;
}

Notification::~Notification() {}

bool Notification::IsRead() const {
  return is_read_ || optional_fields_.priority == MIN_PRIORITY;
}

void Notification::CopyState(Notification* base) {
  shown_as_popup_ = base->shown_as_popup();
  is_read_ = base->is_read_;
  if (!delegate_.get())
    delegate_ = base->delegate();
  optional_fields_.never_timeout = base->never_timeout();
}

void Notification::SetButtonIcon(size_t index, const gfx::Image& icon) {
  if (index >= optional_fields_.buttons.size())
    return;
  optional_fields_.buttons[index].icon = icon;
}

void Notification::SetSystemPriority() {
  optional_fields_.priority = SYSTEM_PRIORITY;
  optional_fields_.never_timeout = true;
}

// static
scoped_ptr<Notification> Notification::CreateSystemNotification(
    const std::string& notification_id,
    const base::string16& title,
    const base::string16& message,
    const gfx::Image& icon,
    const std::string& system_component_id,
    const base::Closure& click_callback) {
  scoped_ptr<Notification> notification(
      new Notification(
          NOTIFICATION_TYPE_SIMPLE,
          notification_id,
          title,
          message,
          icon,
          base::string16()  /* display_source */,
          NotifierId(NotifierId::SYSTEM_COMPONENT, system_component_id),
          RichNotificationData(),
          new HandleNotificationClickedDelegate(click_callback)));
  notification->SetSystemPriority();
  return notification.Pass();
}

}  // namespace message_center
