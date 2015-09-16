// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/metrics/histogram.h"
#include "base/prefs/pref_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/first_run/first_run_controller.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/prefs/pref_service_syncable_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/extensions/app_launch_params.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/pref_names.h"
#include "chromeos/chromeos_switches.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/syncable_prefs/pref_service_syncable.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"

namespace chromeos {
namespace first_run {

namespace {

void LaunchDialogForProfile(Profile* profile) {
  ExtensionService* service =
      extensions::ExtensionSystem::Get(profile)->extension_service();
  if (!service)
    return;

  const extensions::Extension* extension =
      service->GetExtensionById(extension_misc::kFirstRunDialogId, false);
  if (!extension)
    return;

  OpenApplication(
      AppLaunchParams(profile, extension, extensions::LAUNCH_CONTAINER_WINDOW,
                      NEW_WINDOW, extensions::SOURCE_CHROME_INTERNAL));
  profile->GetPrefs()->SetBoolean(prefs::kFirstRunTutorialShown, true);
}

// Object of this class waits for session start. Then it launches or not
// launches first-run dialog depending on user prefs and flags. Than object
// deletes itself.
class DialogLauncher : public content::NotificationObserver {
 public:
  explicit DialogLauncher(Profile* profile)
      : profile_(profile) {
    DCHECK(profile);
    registrar_.Add(this,
                   chrome::NOTIFICATION_SESSION_STARTED,
                   content::NotificationService::AllSources());
  }

  ~DialogLauncher() override {}

  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    DCHECK(type == chrome::NOTIFICATION_SESSION_STARTED);
    DCHECK(content::Details<const user_manager::User>(details).ptr() ==
           ProfileHelper::Get()->GetUserByProfile(profile_));
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    bool launched_in_test = command_line->HasSwitch(::switches::kTestType);
    bool launched_in_telemetry =
        command_line->HasSwitch(switches::kOobeSkipPostLogin);
    bool is_user_new = user_manager::UserManager::Get()->IsCurrentUserNew();
    bool first_run_forced = command_line->HasSwitch(switches::kForceFirstRunUI);
    bool first_run_seen =
        profile_->GetPrefs()->GetBoolean(prefs::kFirstRunTutorialShown);
    bool is_pref_synced =
        PrefServiceSyncableFromProfile(profile_)->IsPrioritySyncing();
    bool is_user_ephemeral = user_manager::UserManager::Get()
                                 ->IsCurrentUserNonCryptohomeDataEphemeral();
    if (!launched_in_telemetry &&
        ((is_user_new && !first_run_seen &&
          (is_pref_synced || !is_user_ephemeral) && !launched_in_test) ||
         first_run_forced)) {
      LaunchDialogForProfile(profile_);
    }
    delete this;
  }

 private:
  Profile* profile_;
  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(DialogLauncher);
};

}  // namespace

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(
      prefs::kFirstRunTutorialShown,
      false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);
}

void MaybeLaunchDialogAfterSessionStart() {
  new DialogLauncher(ProfileHelper::Get()->GetProfileByUserUnsafe(
      user_manager::UserManager::Get()->GetActiveUser()));
}

void LaunchTutorial() {
  UMA_HISTOGRAM_BOOLEAN("CrosFirstRun.TutorialLaunched", true);
  FirstRunController::Start();
}

}  // namespace first_run
}  // namespace chromeos
