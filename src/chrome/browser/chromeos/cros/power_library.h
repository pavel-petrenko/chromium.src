// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CROS_POWER_LIBRARY_H_
#define CHROME_BROWSER_CHROMEOS_CROS_POWER_LIBRARY_H_
#pragma once

// TODO(sque): Move to chrome/browser/chromeos/system, crosbug.com/16558

#include "base/callback.h"

namespace base {
class TimeDelta;
}

namespace chromeos {

typedef base::Callback<void(int64_t)> CalculateIdleTimeCallback;

struct PowerSupplyStatus {
  bool line_power_on;

  bool battery_is_present;
  bool battery_is_full;

  // Time in seconds until the battery is empty or full, 0 for unknown.
  int64 battery_seconds_to_empty;
  int64 battery_seconds_to_full;

  double battery_percentage;

  PowerSupplyStatus() : line_power_on(false),
                        battery_is_present(false),
                        battery_is_full(false),
                        battery_seconds_to_empty(0),
                        battery_seconds_to_full(0),
                        battery_percentage(0) {}
};

// This interface defines interaction with the ChromeOS power library APIs.
// Classes can add themselves as observers. Users can get an instance of this
// library class like this: chromeos::CrosLibrary::Get()->GetPowerLibrary()
class PowerLibrary {
 public:
  class Observer {
   public:
    virtual void PowerChanged(const PowerSupplyStatus& status) = 0;
    virtual void SystemResumed() = 0;

   protected:
    virtual ~Observer() {}
  };

  virtual ~PowerLibrary() {}

  virtual void Init() = 0;

  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // Calculates idle time asynchronously. If it encounters some error,
  // it returns -1.
  virtual void CalculateIdleTime(CalculateIdleTimeCallback* callback) = 0;

  // Enable/disable screen lock for current session.
  virtual void EnableScreenLock(bool enable) = 0;

  // Requests restart of the system.
  virtual void RequestRestart() = 0;

  // Requests shutdown of the system.
  virtual void RequestShutdown() = 0;

  // UI initiated request for status update.
  virtual void RequestStatusUpdate() = 0;

  // Factory function, creates a new instance and returns ownership.
  // For normal usage, access the singleton via CrosLibrary::Get().
  static PowerLibrary* GetImpl(bool stub);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_CROS_POWER_LIBRARY_H_
