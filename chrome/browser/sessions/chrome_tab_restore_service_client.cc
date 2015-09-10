// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sessions/chrome_tab_restore_service_client.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_service.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/common/url_constants.h"

#if defined(ENABLE_EXTENSIONS)
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/extensions/extension_metrics.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_set.h"
#endif

namespace {

void RecordAppLaunch(Profile* profile, const GURL& url) {
#if defined(ENABLE_EXTENSIONS)
  const extensions::Extension* extension =
      extensions::ExtensionRegistry::Get(profile)
          ->enabled_extensions()
          .GetAppByURL(url);
  if (!extension)
    return;

  extensions::RecordAppLaunchType(
      extension_misc::APP_LAUNCH_NTP_RECENTLY_CLOSED, extension->GetType());
#endif  // defined(ENABLE_EXTENSIONS)
}

}  // namespace

ChromeTabRestoreServiceClient::ChromeTabRestoreServiceClient(Profile* profile)
    : profile_(profile) {}

ChromeTabRestoreServiceClient::~ChromeTabRestoreServiceClient() {}

base::FilePath ChromeTabRestoreServiceClient::GetPathToSaveTo() {
  return profile_->GetPath();
}

GURL ChromeTabRestoreServiceClient::GetNewTabURL() {
  return GURL(chrome::kChromeUINewTabURL);
}

bool ChromeTabRestoreServiceClient::HasLastSession() {
#if defined(ENABLE_SESSION_SERVICE)
  SessionService* session_service =
      SessionServiceFactory::GetForProfile(profile_);
  Profile::ExitType exit_type = profile_->GetLastSessionExitType();
  // The previous session crashed and wasn't restored, or was a forced
  // shutdown. Both of which won't have notified us of the browser close so
  // that we need to load the windows from session service (which will have
  // saved them).
  return (!profile_->restored_last_session() && session_service &&
          (exit_type == Profile::EXIT_CRASHED ||
           exit_type == Profile::EXIT_SESSION_ENDED));
#else
  return false;
#endif
}

void ChromeTabRestoreServiceClient::GetLastSession(
    const sessions::GetLastSessionCallback& callback,
    base::CancelableTaskTracker* tracker) {
  DCHECK(HasLastSession());
#if defined(ENABLE_SESSION_SERVICE)
  SessionServiceFactory::GetForProfile(profile_)
      ->GetLastSession(callback, tracker);
#endif
}

void ChromeTabRestoreServiceClient::OnTabRestored(const GURL& url) {
  RecordAppLaunch(profile_, url);
}
