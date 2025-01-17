// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/password/password_details/password_details_table_view_controller.h"

#include <memory>

#include "base/ios/ios_util.h"
#include "base/mac/foundation_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/common/password_form.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/ui/commands/snackbar_commands.h"
#import "ios/chrome/browser/ui/settings/cells/settings_image_detail_text_item.h"
#import "ios/chrome/browser/ui/settings/password/password_details/password_details.h"
#import "ios/chrome/browser/ui/settings/password/password_details/password_details_consumer.h"
#import "ios/chrome/browser/ui/settings/password/password_details/password_details_handler.h"
#import "ios/chrome/browser/ui/settings/password/password_details/password_details_table_view_controller_delegate.h"
#import "ios/chrome/browser/ui/table_view/cells/table_view_cells_constants.h"
#import "ios/chrome/browser/ui/table_view/cells/table_view_text_edit_item.h"
#import "ios/chrome/browser/ui/table_view/chrome_table_view_controller_test.h"
#import "ios/chrome/common/ui/reauthentication/reauthentication_module.h"
#include "ios/chrome/grit/ios_chromium_strings.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/chrome/test/app/password_test_util.h"
#include "ios/web/public/test/web_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
constexpr char kExampleCom[] = "http://www.example.com/";
constexpr char kAndroid[] = "android://hash@com.example.my.app";
constexpr char kUsername[] = "test@egmail.com";
constexpr char kPassword[] = "test";
}

@interface PasswordDetailsTableViewController (Test)
- (void)copyPasswordDetails:(id)sender;
@end

// Test class that conforms to PasswordDetailsHanler in order to test the
// presenter methods are called correctly.
@interface FakePasswordDetailsHandler : NSObject <PasswordDetailsHandler>

@property(nonatomic, assign) BOOL deletionCalled;

@property(nonatomic, assign) BOOL editingCalled;

@end

@implementation FakePasswordDetailsHandler

- (void)passwordDetailsTableViewControllerDidDisappear {
}

- (void)showPasscodeDialog {
}

- (void)showPasswordDeleteDialogWithOrigin:(NSString*)origin {
  self.deletionCalled = YES;
}

- (void)showPasswordEditDialogWithOrigin:(NSString*)origin {
  self.editingCalled = YES;
}

@end

// Test class that conforms to PasswordDetailsViewControllerDelegate in order to
// test the delegate methods are called correctly.
@interface FakePasswordDetailsDelegate
    : NSObject <PasswordDetailsTableViewControllerDelegate>

@property(nonatomic, strong) PasswordDetails* password;

@end

@implementation FakePasswordDetailsDelegate

- (void)passwordDetailsViewController:
            (PasswordDetailsTableViewController*)viewController
               didEditPasswordDetails:(PasswordDetails*)password {
  self.password = password;
}

@end

@interface FakeSnackbarImplementation : NSObject <SnackbarCommands>

@property(nonatomic, assign) NSString* snackbarMessage;

@end

@implementation FakeSnackbarImplementation

- (void)showSnackbarMessage:(MDCSnackbarMessage*)message {
}

- (void)showSnackbarMessage:(MDCSnackbarMessage*)message
               bottomOffset:(CGFloat)offset {
}

- (void)showSnackbarWithMessage:(NSString*)messageText
                     buttonText:(NSString*)buttonText
                  messageAction:(void (^)(void))messageAction
               completionAction:(void (^)(BOOL))completionAction {
  self.snackbarMessage = messageText;
}

@end

// Unit tests for PasswordIssuesTableViewController.
class PasswordDetailsTableViewControllerTest
    : public ChromeTableViewControllerTest {
 protected:
  PasswordDetailsTableViewControllerTest() {
    handler_ = [[FakePasswordDetailsHandler alloc] init];
    delegate_ = [[FakePasswordDetailsDelegate alloc] init];
    reauthentication_module_ = [[MockReauthenticationModule alloc] init];
    reauthentication_module_.expectedResult = ReauthenticationResult::kSuccess;
    snack_bar_ = [[FakeSnackbarImplementation alloc] init];
  }

  ChromeTableViewController* InstantiateController() override {
    PasswordDetailsTableViewController* controller =
        [[PasswordDetailsTableViewController alloc]
            initWithStyle:UITableViewStylePlain];
    controller.handler = handler_;
    controller.delegate = delegate_;
    controller.reauthModule = reauthentication_module_;
    controller.commandsHandler = snack_bar_;
    return controller;
  }

  void SetPassword(std::string website = kExampleCom,
                   std::string username = kUsername,
                   std::string password = kPassword,
                   bool isCompromised = false) {
    auto form = autofill::PasswordForm();
    form.signon_realm = website;
    form.username_value = base::ASCIIToUTF16(username);
    form.password_value = base::ASCIIToUTF16(password);
    form.url = GURL(website);
    form.action = GURL(website + "/action");
    form.username_element = base::ASCIIToUTF16("email");
    form.scheme = autofill::PasswordForm::Scheme::kHtml;

    PasswordDetails* passwordDetails =
        [[PasswordDetails alloc] initWithPasswordForm:form];
    passwordDetails.compromised = isCompromised;

    PasswordDetailsTableViewController* passwords_controller =
        static_cast<PasswordDetailsTableViewController*>(controller());
    [passwords_controller setPassword:passwordDetails];
  }

  void SetFederatedPassword() {
    auto form = autofill::PasswordForm();
    form.username_value = base::ASCIIToUTF16("test@egmail.com");
    form.url = GURL(base::ASCIIToUTF16("http://www.example.com/"));
    form.signon_realm = form.url.spec();
    form.federation_origin =
        url::Origin::Create(GURL("http://www.example.com/"));
    PasswordDetails* password =
        [[PasswordDetails alloc] initWithPasswordForm:form];
    PasswordDetailsTableViewController* passwords_controller =
        static_cast<PasswordDetailsTableViewController*>(controller());
    [passwords_controller setPassword:password];
  }

  void SetBlockedOrigin() {
    auto form = autofill::PasswordForm();
    form.url = GURL("http://www.example.com/");
    form.blocked_by_user = true;
    form.signon_realm = form.url.spec();
    PasswordDetails* password =
        [[PasswordDetails alloc] initWithPasswordForm:form];
    PasswordDetailsTableViewController* passwords_controller =
        static_cast<PasswordDetailsTableViewController*>(controller());
    [passwords_controller setPassword:password];
  }

  void CheckEditCellText(NSString* expected_text, int section, int item) {
    TableViewTextEditItem* cell =
        static_cast<TableViewTextEditItem*>(GetTableViewItem(section, item));
    EXPECT_NSEQ(expected_text, cell.textFieldValue);
  }

  void CheckDetailItemTextWithId(int expected_detail_text_id,
                                 int section,
                                 int item) {
    SettingsImageDetailTextItem* cell =
        static_cast<SettingsImageDetailTextItem*>(
            GetTableViewItem(section, item));
    EXPECT_NSEQ(l10n_util::GetNSString(expected_detail_text_id),
                cell.detailText);
  }

  FakePasswordDetailsHandler* handler() { return handler_; }
  FakePasswordDetailsDelegate* delegate() { return delegate_; }
  MockReauthenticationModule* reauth() { return reauthentication_module_; }
  FakeSnackbarImplementation* snack_bar() {
    return (FakeSnackbarImplementation*)snack_bar_;
  }

 private:
  id snack_bar_;
  FakePasswordDetailsHandler* handler_;
  FakePasswordDetailsDelegate* delegate_;
  MockReauthenticationModule* reauthentication_module_;
};

// Tests that password is displayed properly.
TEST_F(PasswordDetailsTableViewControllerTest, TestPassword) {
  SetPassword();
  EXPECT_EQ(1, NumberOfSections());
  EXPECT_EQ(3, NumberOfItemsInSection(0));

  CheckEditCellText(@"http://www.example.com/", 0, 0);
  CheckEditCellText(@"test@egmail.com", 0, 1);
  CheckEditCellText(kMaskedPassword, 0, 2);
}

// Tests that compromised password is displayed properly.
TEST_F(PasswordDetailsTableViewControllerTest, TestCompromisedPassword) {
  SetPassword(kExampleCom, kUsername, kPassword, true);
  EXPECT_EQ(2, NumberOfSections());
  EXPECT_EQ(3, NumberOfItemsInSection(0));
  EXPECT_EQ(2, NumberOfItemsInSection(1));

  CheckEditCellText(@"http://www.example.com/", 0, 0);
  CheckEditCellText(@"test@egmail.com", 0, 1);
  CheckEditCellText(kMaskedPassword, 0, 2);

  CheckTextCellTextWithId(IDS_IOS_CHANGE_COMPROMISED_PASSWORD, 1, 0);
  CheckDetailItemTextWithId(IDS_IOS_CHANGE_COMPROMISED_PASSWORD_DESCRIPTION, 1,
                            1);
}

// Tests that password is shown/hidden.
TEST_F(PasswordDetailsTableViewControllerTest, TestShowHidePassword) {
  SetPassword();
  CheckEditCellText(kMaskedPassword, 0, 2);

  NSIndexPath* indexOfPassword = [NSIndexPath indexPathForRow:2 inSection:0];
  TableViewTextEditCell* textFieldCell =
      base::mac::ObjCCastStrict<TableViewTextEditCell>([controller()
                      tableView:controller().tableView
          cellForRowAtIndexPath:indexOfPassword]);
  EXPECT_TRUE(textFieldCell);
  [textFieldCell.identifyingIconButton
      sendActionsForControlEvents:UIControlEventTouchUpInside];

  CheckEditCellText(@"test", 0, 2);
  EXPECT_NSEQ(
      l10n_util::GetNSString(IDS_IOS_SETTINGS_PASSWORD_REAUTH_REASON_SHOW),
      reauth().localizedReasonForAuthentication);

  [textFieldCell.identifyingIconButton
      sendActionsForControlEvents:UIControlEventTouchUpInside];
  CheckEditCellText(kMaskedPassword, 0, 2);
}

// Tests that passwords was not shown in case reauth failed.
TEST_F(PasswordDetailsTableViewControllerTest, TestShowPasswordReauthFailed) {
  SetPassword();

  CheckEditCellText(kMaskedPassword, 0, 2);

  reauth().expectedResult = ReauthenticationResult::kFailure;
  NSIndexPath* indexOfPassword = [NSIndexPath indexPathForRow:2 inSection:0];
  TableViewTextEditCell* textFieldCell =
      base::mac::ObjCCastStrict<TableViewTextEditCell>([controller()
                      tableView:controller().tableView
          cellForRowAtIndexPath:indexOfPassword]);
  EXPECT_TRUE(textFieldCell);
  [textFieldCell.identifyingIconButton
      sendActionsForControlEvents:UIControlEventTouchUpInside];

  CheckEditCellText(kMaskedPassword, 0, 2);
}

// Tests that password was revealed during editing.
TEST_F(PasswordDetailsTableViewControllerTest, TestPasswordShownDuringEditing) {
  SetPassword();
  CheckEditCellText(kMaskedPassword, 0, 2);

  PasswordDetailsTableViewController* passwordDetails =
      base::mac::ObjCCastStrict<PasswordDetailsTableViewController>(
          controller());
  [passwordDetails editButtonPressed];
  EXPECT_TRUE(passwordDetails.tableView.editing);
  CheckEditCellText(@"test", 0, 2);

  [passwordDetails editButtonPressed];
  EXPECT_FALSE(passwordDetails.tableView.editing);
  CheckEditCellText(kMaskedPassword, 0, 2);
}

// Tests that editing mode was not entered because reauth failed.
TEST_F(PasswordDetailsTableViewControllerTest, TestEditingReauthFailed) {
  SetPassword();
  CheckEditCellText(kMaskedPassword, 0, 2);

  reauth().expectedResult = ReauthenticationResult::kFailure;
  PasswordDetailsTableViewController* passwordDetails =
      base::mac::ObjCCastStrict<PasswordDetailsTableViewController>(
          controller());
  [passwordDetails editButtonPressed];
  EXPECT_FALSE(passwordDetails.tableView.editing);
  CheckEditCellText(kMaskedPassword, 0, 2);
}

// Tests that delete button trigger showing password delete dialog.
TEST_F(PasswordDetailsTableViewControllerTest, TestPasswordDelete) {
  SetPassword();

  EXPECT_FALSE(handler().deletionCalled);
  PasswordDetailsTableViewController* passwordDetails =
      base::mac::ObjCCastStrict<PasswordDetailsTableViewController>(
          controller());
  [passwordDetails editButtonPressed];
  [[UIApplication sharedApplication]
      sendAction:passwordDetails.deleteButton.action
              to:passwordDetails.deleteButton.target
            from:nil
        forEvent:nil];
  EXPECT_TRUE(handler().deletionCalled);
}

// Tests password editing. User confirmed this action.
TEST_F(PasswordDetailsTableViewControllerTest, TestEditPasswordConfirmed) {
  SetPassword();

  PasswordDetailsTableViewController* passwordDetails =
      base::mac::ObjCCastStrict<PasswordDetailsTableViewController>(
          controller());
  [passwordDetails editButtonPressed];
  EXPECT_FALSE(handler().editingCalled);
  EXPECT_FALSE(delegate().password);
  EXPECT_TRUE(passwordDetails.tableView.editing);

  TableViewTextEditItem* cell =
      static_cast<TableViewTextEditItem*>(GetTableViewItem(0, 2));
  cell.textFieldValue = @"new_password";

  [passwordDetails editButtonPressed];
  EXPECT_TRUE(handler().editingCalled);

  [passwordDetails passwordEditingConfirmed];
  EXPECT_TRUE(delegate().password);

  EXPECT_NSEQ(@"new_password", delegate().password.password);
  EXPECT_FALSE(passwordDetails.tableView.editing);
}

// Tests password editing. User cancelled this action.
TEST_F(PasswordDetailsTableViewControllerTest, TestEditPasswordCancel) {
  SetPassword();

  PasswordDetailsTableViewController* passwordDetails =
      base::mac::ObjCCastStrict<PasswordDetailsTableViewController>(
          controller());
  [passwordDetails editButtonPressed];
  EXPECT_FALSE(delegate().password);
  EXPECT_TRUE(passwordDetails.tableView.editing);

  TableViewTextEditItem* cell =
      static_cast<TableViewTextEditItem*>(GetTableViewItem(0, 2));
  cell.textFieldValue = @"new_password";

  [passwordDetails editButtonPressed];
  EXPECT_FALSE(delegate().password);
  EXPECT_TRUE(passwordDetails.tableView.editing);
}

// Tests android compromised credential is displayed without change password
// button.
TEST_F(PasswordDetailsTableViewControllerTest,
       TestAndroidCompromisedCredential) {
  SetPassword(kAndroid, kUsername, kPassword, true);
  EXPECT_EQ(2, NumberOfSections());
  EXPECT_EQ(3, NumberOfItemsInSection(0));
  EXPECT_EQ(1, NumberOfItemsInSection(1));

  CheckEditCellText(@"com.example.my.app", 0, 0);
  CheckEditCellText(@"test@egmail.com", 0, 1);
  CheckEditCellText(kMaskedPassword, 0, 2);

  CheckDetailItemTextWithId(IDS_IOS_CHANGE_COMPROMISED_PASSWORD_DESCRIPTION, 1,
                            0);
}

// Tests federated credential is shown without password value and editing
// doesn't require reauth.
TEST_F(PasswordDetailsTableViewControllerTest, TestFederatedCredential) {
  SetFederatedPassword();
  EXPECT_EQ(1, NumberOfSections());
  EXPECT_EQ(2, NumberOfItemsInSection(0));

  CheckEditCellText(@"http://www.example.com/", 0, 0);
  CheckEditCellText(@"test@egmail.com", 0, 1);

  reauth().expectedResult = ReauthenticationResult::kFailure;
  PasswordDetailsTableViewController* passwordDetails =
      base::mac::ObjCCastStrict<PasswordDetailsTableViewController>(
          controller());
  [passwordDetails editButtonPressed];
  EXPECT_TRUE(passwordDetails.tableView.editing);
}

// Tests blocked website is shown without password and username values and
// editing doesn't require reauth.
TEST_F(PasswordDetailsTableViewControllerTest, TestBlockedOrigin) {
  SetBlockedOrigin();
  EXPECT_EQ(1, NumberOfSections());
  EXPECT_EQ(1, NumberOfItemsInSection(0));

  CheckEditCellText(@"http://www.example.com/", 0, 0);

  reauth().expectedResult = ReauthenticationResult::kFailure;
  PasswordDetailsTableViewController* passwordDetails =
      base::mac::ObjCCastStrict<PasswordDetailsTableViewController>(
          controller());
  [passwordDetails editButtonPressed];
  EXPECT_TRUE(passwordDetails.tableView.editing);
}

// Tests copy website works as intended.
TEST_F(PasswordDetailsTableViewControllerTest, CopySite) {
  SetPassword();

  PasswordDetailsTableViewController* passwordDetails =
      base::mac::ObjCCastStrict<PasswordDetailsTableViewController>(
          controller());

  [passwordDetails tableView:passwordDetails.tableView
      didSelectRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
  UIMenuController* menu = [UIMenuController sharedMenuController];
  EXPECT_EQ(1u, menu.menuItems.count);
  [passwordDetails copyPasswordDetails:menu];

  UIPasteboard* generalPasteboard = [UIPasteboard generalPasteboard];
  EXPECT_NSEQ(@"http://www.example.com/", generalPasteboard.string);
  EXPECT_NSEQ(l10n_util::GetNSString(IDS_IOS_SETTINGS_SITE_WAS_COPIED_MESSAGE),
              snack_bar().snackbarMessage);
}

// Tests copy username works as intended.
TEST_F(PasswordDetailsTableViewControllerTest, CopyUsername) {
  SetPassword();
  PasswordDetailsTableViewController* passwordDetails =
      base::mac::ObjCCastStrict<PasswordDetailsTableViewController>(
          controller());

  [passwordDetails tableView:passwordDetails.tableView
      didSelectRowAtIndexPath:[NSIndexPath indexPathForRow:1 inSection:0]];
  UIMenuController* menu = [UIMenuController sharedMenuController];
  EXPECT_EQ(1u, menu.menuItems.count);
  [passwordDetails copyPasswordDetails:menu];

  UIPasteboard* generalPasteboard = [UIPasteboard generalPasteboard];
  EXPECT_NSEQ(@"test@egmail.com", generalPasteboard.string);
  EXPECT_NSEQ(
      l10n_util::GetNSString(IDS_IOS_SETTINGS_USERNAME_WAS_COPIED_MESSAGE),
      snack_bar().snackbarMessage);
}

// Tests copy password works as intended when reauth was successful.
TEST_F(PasswordDetailsTableViewControllerTest, CopyPasswordSuccess) {
  SetPassword();

  PasswordDetailsTableViewController* passwordDetails =
      base::mac::ObjCCastStrict<PasswordDetailsTableViewController>(
          controller());

  [passwordDetails tableView:passwordDetails.tableView
      didSelectRowAtIndexPath:[NSIndexPath indexPathForRow:2 inSection:0]];

  UIMenuController* menu = [UIMenuController sharedMenuController];
  EXPECT_EQ(1u, menu.menuItems.count);
  [passwordDetails copyPasswordDetails:menu];

  UIPasteboard* generalPasteboard = [UIPasteboard generalPasteboard];
  EXPECT_NSEQ(@"test", generalPasteboard.string);
  EXPECT_NSEQ(
      l10n_util::GetNSString(IDS_IOS_SETTINGS_PASSWORD_REAUTH_REASON_COPY),
      reauth().localizedReasonForAuthentication);
  EXPECT_NSEQ(
      l10n_util::GetNSString(IDS_IOS_SETTINGS_PASSWORD_WAS_COPIED_MESSAGE),
      snack_bar().snackbarMessage);
}

// Tests copy password works as intended.
TEST_F(PasswordDetailsTableViewControllerTest, CopyPasswordFail) {
  SetPassword();

  PasswordDetailsTableViewController* passwordDetails =
      base::mac::ObjCCastStrict<PasswordDetailsTableViewController>(
          controller());

  reauth().expectedResult = ReauthenticationResult::kFailure;
  [passwordDetails tableView:passwordDetails.tableView
      didSelectRowAtIndexPath:[NSIndexPath indexPathForRow:2 inSection:0]];

  UIMenuController* menu = [UIMenuController sharedMenuController];
  EXPECT_EQ(1u, menu.menuItems.count);
  [passwordDetails copyPasswordDetails:menu];

  EXPECT_NSEQ(
      l10n_util::GetNSString(IDS_IOS_SETTINGS_PASSWORD_WAS_NOT_COPIED_MESSAGE),
      snack_bar().snackbarMessage);
}
