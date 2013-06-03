// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_POWER_POLICY_CONTROLLER_H_
#define CHROMEOS_DBUS_POWER_POLICY_CONTROLLER_H_

#include <map>
#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_thread_manager_observer.h"
#include "chromeos/dbus/power_manager/policy.pb.h"
#include "chromeos/dbus/power_manager_client.h"

namespace chromeos {

class DBusThreadManager;

// PowerPolicyController is responsible for sending Chrome's assorted power
// management preferences to the Chrome OS power manager.
class CHROMEOS_EXPORT PowerPolicyController
    : public DBusThreadManagerObserver,
      public PowerManagerClient::Observer {
 public:
  // Note: Do not change these values; they are used by preferences.
  enum Action {
    ACTION_SUSPEND      = 0,
    ACTION_STOP_SESSION = 1,
    ACTION_SHUT_DOWN    = 2,
    ACTION_DO_NOTHING   = 3,
  };

  // Values of various power-management-related preferences.
  struct PrefValues {
    PrefValues();

    int ac_screen_dim_delay_ms;
    int ac_screen_off_delay_ms;
    int ac_screen_lock_delay_ms;
    int ac_idle_warning_delay_ms;
    int ac_idle_delay_ms;
    int battery_screen_dim_delay_ms;
    int battery_screen_off_delay_ms;
    int battery_screen_lock_delay_ms;
    int battery_idle_warning_delay_ms;
    int battery_idle_delay_ms;
    Action idle_action;
    Action lid_closed_action;
    bool use_audio_activity;
    bool use_video_activity;
    bool allow_screen_wake_locks;
    bool enable_screen_lock;
    double presentation_idle_delay_factor;
    double user_activity_screen_dim_delay_factor;
  };

  // Returns a string describing |policy|.  Useful for tests.
  static std::string GetPolicyDebugString(
      const power_manager::PowerManagementPolicy& policy);

  // Delay in milliseconds between the screen being turned off and the
  // screen being locked. Used if the |enable_screen_lock| pref is set but
  // |*_screen_lock_delay_ms| are unset or set to higher values than what
  // this constant would imply.
  const static int kScreenLockAfterOffDelayMs;

  PowerPolicyController(DBusThreadManager* manager, PowerManagerClient* client);
  virtual ~PowerPolicyController();

  // Updates |prefs_policy_| with |values| and sends an updated policy.
  void ApplyPrefs(const PrefValues& values);

  // Registers a request to temporarily prevent the screen from getting
  // dimmed or turned off or the system from suspending in response to user
  // inactivity and sends an updated policy.  |reason| is a human-readable
  // description of the reason the lock was created.  Returns a unique ID
  // that can be passed to RemoveWakeLock() later.
  int AddScreenWakeLock(const std::string& reason);
  int AddSystemWakeLock(const std::string& reason);

  // Unregisters a request previously created via AddScreenWakeLock() or
  // AddSystemWakeLock() and sends an updated policy.
  void RemoveWakeLock(int id);

  // DBusThreadManagerObserver implementation:
  virtual void OnDBusThreadManagerDestroying(DBusThreadManager* manager)
      OVERRIDE;

  // PowerManagerClient::Observer implementation:
  virtual void PowerManagerRestarted() OVERRIDE;

 private:
  typedef std::map<int, std::string> WakeLockMap;

  // Sends a policy based on |prefs_policy_| to the power manager.
  void SendCurrentPolicy();

  // Sends an empty policy to the power manager to reset its configuration.
  void SendEmptyPolicy();

  DBusThreadManager* manager_;  // not owned
  PowerManagerClient* client_;  // not owned

  // Policy derived from values passed to ApplyPrefs().
  power_manager::PowerManagementPolicy prefs_policy_;

  // Was ApplyPrefs() called?
  bool prefs_were_set_;

  // Maps from an ID representing a request to prevent the screen from
  // getting dimmed or turned off or to prevent the system from suspending
  // to the reason for the request.
  WakeLockMap screen_wake_locks_;
  WakeLockMap system_wake_locks_;

  // Should entries in |screen_wake_locks_| be honored?  If false, screen
  // wake locks are just treated as system wake locks instead.
  bool honor_screen_wake_locks_;

  // Next ID to be used by AddScreenWakeLock() or AddSystemWakeLock().
  int next_wake_lock_id_;

  DISALLOW_COPY_AND_ASSIGN(PowerPolicyController);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_POWER_POLICY_CONTROLLER_H_
