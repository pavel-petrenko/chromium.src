// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CHROME_BROWSER_MAIN_CHROMEOS_H_
#define CHROME_BROWSER_CHROMEOS_CHROME_BROWSER_MAIN_CHROMEOS_H_

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/chrome_browser_main_linux.h"
#include "chrome/browser/chromeos/version_loader.h"
#include "chrome/common/cancelable_task_tracker.h"

namespace contacts {
class ContactManager;
}

namespace chromeos {

class BrightnessObserver;
class MagnificationManager;
class OutputObserver;
class PowerButtonObserver;
class PowerStateOverride;
class PrimaryDisplaySwitchObserver;
class RemovableDeviceNotificationsCros;
class ResumeObserver;
class ScreenDimmingObserver;
class ScreenLockObserver;
class SessionManagerObserver;
class UserActivityNotifier;
class VideoActivityNotifier;

namespace default_app_order {
class ExternalLoader;
}

namespace internal {
class DBusServices;
}

class ChromeBrowserMainPartsChromeos : public ChromeBrowserMainPartsLinux {
 public:
  explicit ChromeBrowserMainPartsChromeos(
      const content::MainFunctionParams& parameters);
  virtual ~ChromeBrowserMainPartsChromeos();

  // ChromeBrowserMainParts overrides.
  virtual void PreEarlyInitialization() OVERRIDE;
  virtual void PreMainMessageLoopStart() OVERRIDE;
  virtual void PostMainMessageLoopStart() OVERRIDE;
  virtual void PreMainMessageLoopRun() OVERRIDE;

  // Stages called from PreMainMessageLoopRun.
  virtual void PreProfileInit() OVERRIDE;
  virtual void PostProfileInit() OVERRIDE;
  virtual void PreBrowserStart() OVERRIDE;
  virtual void PostBrowserStart() OVERRIDE;

  virtual void PostMainMessageLoopRun() OVERRIDE;

  virtual void SetupPlatformFieldTrials() OVERRIDE;

 private:
  // Set up field trial for low memory headroom settings.
  void SetupLowMemoryHeadroomFieldTrial();

  scoped_ptr<contacts::ContactManager> contact_manager_;
  scoped_ptr<BrightnessObserver> brightness_observer_;
  scoped_ptr<default_app_order::ExternalLoader> app_order_loader_;
  scoped_ptr<OutputObserver> output_observer_;
  scoped_ptr<ResumeObserver> resume_observer_;
  scoped_ptr<ScreenLockObserver> screen_lock_observer_;
  scoped_ptr<PowerButtonObserver> power_button_observer_;
  scoped_refptr<PowerStateOverride> power_state_override_;
  scoped_ptr<PrimaryDisplaySwitchObserver> primary_display_switch_observer_;
  scoped_ptr<UserActivityNotifier> user_activity_notifier_;
  scoped_ptr<VideoActivityNotifier> video_activity_notifier_;
  scoped_ptr<ScreenDimmingObserver> screen_dimming_observer_;
  scoped_refptr<RemovableDeviceNotificationsCros>
      removable_device_notifications_;

  scoped_ptr<internal::DBusServices> dbus_services_;

  VersionLoader cros_version_loader_;
  CancelableTaskTracker tracker_;

  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserMainPartsChromeos);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_CHROME_BROWSER_MAIN_CHROMEOS_H_
