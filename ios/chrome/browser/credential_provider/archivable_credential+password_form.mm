// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/credential_provider/archivable_credential+password_form.h"

#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/android_affiliation/affiliation_utils.h"
#include "components/password_manager/core/browser/password_ui_utils.h"
#import "ios/chrome/browser/credential_provider/credential_provider_util.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using base::SysUTF8ToNSString;
using base::SysUTF16ToNSString;

}  // namespace

@implementation ArchivableCredential (PasswordForm)

- (instancetype)initWithPasswordForm:(const autofill::PasswordForm&)passwordForm
                             favicon:(NSString*)favicon
                validationIdentifier:(NSString*)validationIdentifier {
  if (passwordForm.blocked_by_user) {
    return nil;
  }
  std::string site_name =
      password_manager::GetShownOrigin(url::Origin::Create(passwordForm.url));
  NSString* keychainIdentifier =
      SysUTF8ToNSString(passwordForm.encrypted_password);

  NSString* serviceIdentifier = SysUTF8ToNSString(passwordForm.url.spec());
  NSString* serviceName = SysUTF8ToNSString(site_name);

  if (password_manager::IsValidAndroidFacetURI(passwordForm.signon_realm)) {
    NSString* webRealm = SysUTF8ToNSString(passwordForm.affiliated_web_realm);
    url::Origin origin =
        url::Origin::Create(GURL(passwordForm.affiliated_web_realm));
    std::string shownOrigin = password_manager::GetShownOrigin(origin);

    // Set serviceIdentifier:
    if (webRealm.length) {
      // Prefer webRealm.
      serviceIdentifier = webRealm;
    } else if (!serviceIdentifier.length) {
      // Fallback to signon_realm.
      serviceIdentifier = SysUTF8ToNSString(passwordForm.signon_realm);
    }

    // Set serviceName:
    if (!shownOrigin.empty()) {
      // Prefer shownOrigin to match non Android credentials.
      serviceName = SysUTF8ToNSString(shownOrigin);
    } else if (!passwordForm.app_display_name.empty()) {
      serviceName = SysUTF8ToNSString(passwordForm.app_display_name);
    } else if (!serviceName.length) {
      // Fallback to serviceIdentifier.
      serviceName = serviceIdentifier;
    }
  }

  DCHECK(serviceIdentifier.length);

  return [self initWithFavicon:favicon
            keychainIdentifier:keychainIdentifier
                          rank:passwordForm.times_used
              recordIdentifier:RecordIdentifierForPasswordForm(passwordForm)
             serviceIdentifier:serviceIdentifier
                   serviceName:serviceName
                          user:SysUTF16ToNSString(passwordForm.username_value)
          validationIdentifier:validationIdentifier];
}

@end
