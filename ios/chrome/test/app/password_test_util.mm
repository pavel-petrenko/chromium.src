// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/test/app/password_test_util.h"

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/settings/password/legacy_password_details_table_view_controller+testing.h"
#import "ios/chrome/browser/ui/settings/password/passwords_table_view_controller.h"
#import "ios/chrome/browser/ui/settings/settings_navigation_controller.h"
#import "ios/chrome/browser/ui/util/top_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation MockReauthenticationModule

@synthesize localizedReasonForAuthentication =
    _localizedReasonForAuthentication;
@synthesize expectedResult = _expectedResult;
@synthesize canAttempt = _canAttempt;

- (void)setExpectedResult:(ReauthenticationResult)expectedResult {
  _canAttempt = YES;
  _expectedResult = expectedResult;
}

- (BOOL)canAttemptReauth {
  return _canAttempt;
}

- (void)attemptReauthWithLocalizedReason:(NSString*)localizedReason
                    canReusePreviousAuth:(BOOL)canReusePreviousAuth
                                 handler:
                                     (void (^)(ReauthenticationResult success))
                                         showCopyPasswordsHandler {
  self.localizedReasonForAuthentication = localizedReason;
  showCopyPasswordsHandler(_expectedResult);
}

@end

namespace chrome_test_util {

// Replace the reauthentication module in
// PasswordDetailsCollectionViewController with a fake one to avoid being
// blocked with a reauth prompt, and return the fake reauthentication module.
MockReauthenticationModule* SetUpAndReturnMockReauthenticationModule() {
  MockReauthenticationModule* mock_reauthentication_module =
      [[MockReauthenticationModule alloc] init];
  // TODO(crbug.com/754642): Stop using TopPresentedViewController();
  SettingsNavigationController* settings_navigation_controller =
      base::mac::ObjCCastStrict<SettingsNavigationController>(
          top_view_controller::TopPresentedViewController());
  LegacyPasswordDetailsTableViewController*
      password_details_table_view_controller =
          base::mac::ObjCCastStrict<LegacyPasswordDetailsTableViewController>(
              settings_navigation_controller.topViewController);
  [password_details_table_view_controller
      setReauthenticationModule:mock_reauthentication_module];
  return mock_reauthentication_module;
}

// Replace the reauthentication module in
// PasswordExporter with a fake one to avoid being
// blocked with a reauth prompt, and return the fake reauthentication module.
MockReauthenticationModule*
SetUpAndReturnMockReauthenticationModuleForExport() {
  MockReauthenticationModule* mock_reauthentication_module =
      [[MockReauthenticationModule alloc] init];
  // TODO(crbug.com/754642): Stop using TopPresentedViewController();
  SettingsNavigationController* settings_navigation_controller =
      base::mac::ObjCCastStrict<SettingsNavigationController>(
          top_view_controller::TopPresentedViewController());
  PasswordsTableViewController* passwords_table_view_controller =
      base::mac::ObjCCastStrict<PasswordsTableViewController>(
          settings_navigation_controller.topViewController);
  passwords_table_view_controller.reauthenticationModule =
      mock_reauthentication_module;
  return mock_reauthentication_module;
}

}  // namespace
