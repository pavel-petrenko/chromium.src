// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <string>

#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/message_center_notification_manager.h"
#include "chrome/browser/notifications/notification.h"
#include "chrome/browser/notifications/notification_ui_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/test_switches.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/message_center_types.h"

class TestAddObserver : public message_center::MessageCenterObserver {
 public:
  explicit TestAddObserver(message_center::MessageCenter* message_center)
      : message_center_(message_center) {
    message_center_->AddObserver(this);
  }

  ~TestAddObserver() override { message_center_->RemoveObserver(this); }

  void OnNotificationAdded(const std::string& id) override {
    std::string log = logs_[id];
    if (!log.empty())
      log += "_";
    logs_[id] = log + "add-" + id;
  }

  void OnNotificationUpdated(const std::string& id) override {
    std::string log = logs_[id];
    if (!log.empty())
      log += "_";
    logs_[id] = log + "update-" + id;
  }

  const std::string log(const std::string& id) { return logs_[id]; }
  void reset_logs() { logs_.clear(); }

 private:
  std::map<std::string, std::string> logs_;
  message_center::MessageCenter* message_center_;
};

class MessageCenterNotificationsTest : public InProcessBrowserTest {
 public:
  MessageCenterNotificationsTest() {}

  MessageCenterNotificationManager* manager() {
    return static_cast<MessageCenterNotificationManager*>(
        g_browser_process->notification_ui_manager());
  }

  message_center::MessageCenter* message_center() {
    return g_browser_process->message_center();
  }

  Profile* profile() { return browser()->profile(); }

  class TestDelegate : public NotificationDelegate {
   public:
    explicit TestDelegate(const std::string& id) : id_(id) {}

    void Display() override { log_ += "Display_"; }
    void Close(bool by_user) override {
      log_ += "Close_";
      log_ += (by_user ? "by_user_" : "programmatically_");
    }
    void Click() override { log_ += "Click_"; }
    void ButtonClick(int button_index) override {
      log_ += "ButtonClick_";
      log_ += base::IntToString(button_index) + "_";
    }
    std::string id() const override { return id_; }

    const std::string& log() { return log_; }

   private:
    ~TestDelegate() override {}
    std::string id_;
    std::string log_;

    DISALLOW_COPY_AND_ASSIGN(TestDelegate);
  };

  Notification CreateTestNotification(const std::string& delegate_id,
                                      TestDelegate** delegate = NULL) {
    TestDelegate* new_delegate = new TestDelegate(delegate_id);
    if (delegate) {
      *delegate = new_delegate;
      new_delegate->AddRef();
    }

    return Notification(GURL("chrome-test://testing/"),
                        base::ASCIIToUTF16("title"),
                        base::ASCIIToUTF16("message"),
                        gfx::Image(),
                        base::UTF8ToUTF16("chrome-test://testing/"),
                        "REPLACE-ME",
                        new_delegate);
  }

  Notification CreateRichTestNotification(const std::string& id,
                                          TestDelegate** delegate = NULL) {
    TestDelegate* new_delegate = new TestDelegate(id);
    if (delegate) {
      *delegate = new_delegate;
      new_delegate->AddRef();
    }

    message_center::RichNotificationData data;

    return Notification(
        message_center::NOTIFICATION_TYPE_BASE_FORMAT,
        base::ASCIIToUTF16("title"), base::ASCIIToUTF16("message"),
        gfx::Image(),
        message_center::NotifierId(message_center::NotifierId::APPLICATION,
                                   "extension_id"),
        base::UTF8ToUTF16("chrome-test://testing/"),
        GURL("chrome-test://testing/"), "REPLACE-ME", data, new_delegate);
  }
};

// TODO(rsesek): Implement Message Center on Mac and get these tests passing
// for real. http://crbug.com/179904
#if !defined(OS_MACOSX)

IN_PROC_BROWSER_TEST_F(MessageCenterNotificationsTest, RetrieveBaseParts) {
  EXPECT_TRUE(manager());
  EXPECT_TRUE(message_center());
}

IN_PROC_BROWSER_TEST_F(MessageCenterNotificationsTest, BasicAddCancel) {
#if defined(OS_WIN) && defined(USE_ASH)
  // Disable this test in Metro+Ash for now (http://crbug.com/262796).
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAshBrowserTests))
    return;
#endif

  // Someone may create system notifications like "you're in multi-profile
  // mode..." or something which may change the expectation.
  // TODO(mukai): move this to SetUpOnMainThread() after fixing the side-effect
  // of canceling animation which prevents some Displayed() event.
  manager()->CancelAll();
  manager()->Add(CreateTestNotification("hey"), profile());
  EXPECT_EQ(1u, message_center()->NotificationCount());
  manager()->CancelById("hey", NotificationUIManager::GetProfileID(profile()));
  EXPECT_EQ(0u, message_center()->NotificationCount());
}

IN_PROC_BROWSER_TEST_F(MessageCenterNotificationsTest, BasicDelegate) {
#if defined(OS_WIN) && defined(USE_ASH)
  // Disable this test in Metro+Ash for now (http://crbug.com/262796).
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAshBrowserTests))
    return;
#endif

  TestDelegate* delegate;
  manager()->Add(CreateTestNotification("hey", &delegate), profile());
  // Verify that delegate accumulated correct log of events.
  EXPECT_EQ("Display_", delegate->log());
  manager()->CancelById("hey", NotificationUIManager::GetProfileID(profile()));
  // Verify that delegate accumulated correct log of events.
  EXPECT_EQ("Display_Close_programmatically_", delegate->log());
  delegate->Release();
}

IN_PROC_BROWSER_TEST_F(MessageCenterNotificationsTest, ButtonClickedDelegate) {
#if defined(OS_WIN) && defined(USE_ASH)
  // Disable this test in Metro+Ash for now (http://crbug.com/262796).
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAshBrowserTests))
    return;
#endif

  TestDelegate* delegate;
  manager()->Add(CreateTestNotification("n", &delegate), profile());
  const std::string notification_id =
      manager()->GetMessageCenterNotificationIdForTest("n", profile());
  message_center()->ClickOnNotificationButton(notification_id, 1);
  // Verify that delegate accumulated correct log of events.
  EXPECT_EQ("Display_ButtonClick_1_", delegate->log());
  delegate->Release();
}

IN_PROC_BROWSER_TEST_F(MessageCenterNotificationsTest,
                       UpdateExistingNotification) {
#if defined(OS_WIN) && defined(USE_ASH)
  // Disable this test in Metro+Ash for now (http://crbug.com/262796).
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAshBrowserTests))
    return;
#endif

  TestDelegate* delegate;
  manager()->Add(CreateTestNotification("n", &delegate), profile());
  TestDelegate* delegate2;
  manager()->Add(CreateRichTestNotification("n", &delegate2), profile());

  manager()->CancelById("n", NotificationUIManager::GetProfileID(profile()));
  EXPECT_EQ("Display_", delegate->log());
  EXPECT_EQ("Close_programmatically_", delegate2->log());

  delegate->Release();
  delegate2->Release();
}

IN_PROC_BROWSER_TEST_F(MessageCenterNotificationsTest, QueueWhenCenterVisible) {
#if defined(OS_WIN) && defined(USE_ASH)
  // Disable this test in Metro+Ash for now (http://crbug.com/262796).
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAshBrowserTests))
    return;
#endif

  TestAddObserver observer(message_center());

  TestDelegate* delegate;
  TestDelegate* delegate2;

  manager()->Add(CreateTestNotification("n", &delegate), profile());
  const std::string id_n =
      manager()->GetMessageCenterNotificationIdForTest("n", profile());
  message_center()->SetVisibility(message_center::VISIBILITY_MESSAGE_CENTER);
  manager()->Add(CreateTestNotification("n2", &delegate2), profile());
  const std::string id_n2 =
      manager()->GetMessageCenterNotificationIdForTest("n2", profile());

  // 'update-n' should happen since SetVisibility updates is_read status of n.
  // TODO(mukai): fix event handling to happen update-n just once.
  EXPECT_EQ(base::StringPrintf("add-%s_update-%s_update-%s",
                               id_n.c_str(),
                               id_n.c_str(),
                               id_n.c_str()),
            observer.log(id_n));

  message_center()->SetVisibility(message_center::VISIBILITY_TRANSIENT);

  EXPECT_EQ(base::StringPrintf("add-%s", id_n2.c_str()), observer.log(id_n2));

  delegate->Release();
  delegate2->Release();
}

IN_PROC_BROWSER_TEST_F(MessageCenterNotificationsTest,
                       UpdateNonProgressNotificationWhenCenterVisible) {
#if defined(OS_WIN) && defined(USE_ASH)
  // Disable this test in Metro+Ash for now (http://crbug.com/262796).
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAshBrowserTests))
    return;
#endif

  TestAddObserver observer(message_center());

  TestDelegate* delegate;

  // Add a non-progress notification and update it while the message center
  // is visible.
  Notification notification = CreateTestNotification("n", &delegate);
  manager()->Add(notification, profile());
  const std::string notification_id =
      manager()->GetMessageCenterNotificationIdForTest("n", profile());
  message_center()->ClickOnNotification(notification_id);
  message_center()->SetVisibility(message_center::VISIBILITY_MESSAGE_CENTER);
  observer.reset_logs();
  notification.set_title(base::ASCIIToUTF16("title2"));
  manager()->Update(notification, profile());

  // Expect that the notification update is not done.
  EXPECT_EQ("", observer.log(notification_id));

  message_center()->SetVisibility(message_center::VISIBILITY_TRANSIENT);
  EXPECT_EQ(base::StringPrintf("update-%s", notification_id.c_str()),
            observer.log(notification_id));

  delegate->Release();
}

IN_PROC_BROWSER_TEST_F(
    MessageCenterNotificationsTest,
    UpdateNonProgressToProgressNotificationWhenCenterVisible) {
#if defined(OS_WIN) && defined(USE_ASH)
  // Disable this test in Metro+Ash for now (http://crbug.com/262796).
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAshBrowserTests))
    return;
#endif

  TestAddObserver observer(message_center());

  TestDelegate* delegate;

  // Add a non-progress notification and change the type to progress while the
  // message center is visible.
  Notification notification = CreateTestNotification("n", &delegate);
  manager()->Add(notification, profile());
  const std::string notification_id =
      manager()->GetMessageCenterNotificationIdForTest("n", profile());
  message_center()->ClickOnNotification(notification_id);
  message_center()->SetVisibility(message_center::VISIBILITY_MESSAGE_CENTER);
  observer.reset_logs();
  notification.set_type(message_center::NOTIFICATION_TYPE_PROGRESS);
  manager()->Update(notification, profile());

  // Expect that the notification update is not done.
  EXPECT_EQ("", observer.log(notification_id));

  message_center()->SetVisibility(message_center::VISIBILITY_TRANSIENT);
  EXPECT_EQ(base::StringPrintf("update-%s", notification_id.c_str()),
            observer.log(notification_id));

  delegate->Release();
}

IN_PROC_BROWSER_TEST_F(MessageCenterNotificationsTest,
                       UpdateProgressNotificationWhenCenterVisible) {
#if defined(OS_WIN) && defined(USE_ASH)
  // Disable this test in Metro+Ash for now (http://crbug.com/262796).
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAshBrowserTests))
    return;
#endif

  TestAddObserver observer(message_center());

  TestDelegate* delegate;

  // Add a progress notification and update it while the message center
  // is visible.
  Notification notification = CreateTestNotification("n", &delegate);
  notification.set_type(message_center::NOTIFICATION_TYPE_PROGRESS);
  manager()->Add(notification, profile());
  const std::string notification_id =
      manager()->GetMessageCenterNotificationIdForTest("n", profile());
  message_center()->ClickOnNotification(notification_id);
  message_center()->SetVisibility(message_center::VISIBILITY_MESSAGE_CENTER);
  observer.reset_logs();
  notification.set_progress(50);
  manager()->Update(notification, profile());

  // Expect that the progress notification update is performed.
  EXPECT_EQ(base::StringPrintf("update-%s", notification_id.c_str()),
            observer.log(notification_id));

  delegate->Release();
}

#endif  // !defined(OS_MACOSX)
