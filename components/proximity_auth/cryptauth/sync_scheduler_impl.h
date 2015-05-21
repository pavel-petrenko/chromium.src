// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PROXIMITY_AUTH_CRYPTAUTH_CRYPTAUTH_SYNC_SCHEDULER_IMPL_H
#define COMPONENTS_PROXIMITY_AUTH_CRYPTAUTH_CRYPTAUTH_SYNC_SCHEDULER_IMPL_H

#include "base/timer/timer.h"
#include "components/proximity_auth/cryptauth/sync_scheduler.h"

namespace proximity_auth {

// Implementation of SyncScheduler.
class SyncSchedulerImpl : public SyncScheduler {
 public:
  // Creates the scheduler:
  // |delegate|: Handles sync requests and must outlive the scheduler.
  // |refresh_period|: The time to wait for the PERIODIC_REFRESH strategy.
  // |base_recovery_period|: The initial time to wait for the
  //    AGGRESSIVE_RECOVERY strategy. The time delta is increased for each
  //    subsequent failure.
  // |max_jitter_ratio|: The maximum ratio that the time to next sync can be
  //    jittered (both positively and negatively).
  // |scheduler_name|: The name of the scheduler for debugging purposes.
  SyncSchedulerImpl(Delegate* delegate,
                    base::TimeDelta refresh_period,
                    base::TimeDelta base_recovery_period,
                    double max_jitter_ratio,
                    const std::string& scheduler_name);

  ~SyncSchedulerImpl() override;

  // SyncScheduler:
  void Start(const base::TimeDelta& elapsed_time_since_last_sync,
             Strategy strategy) override;
  void ForceSync() override;
  base::TimeDelta GetTimeToNextSync() const override;
  Strategy GetStrategy() const override;
  SyncState GetSyncState() const override;

 protected:
  // Creates and returns a base::Timer object. Exposed for testing.
  virtual scoped_ptr<base::Timer> CreateTimer();

 private:
  // SyncScheduler:
  void OnSyncCompleted(bool success) override;

  // Called when |timer_| is fired.
  void OnTimerFired();

  // Schedules |timer_| for the next sync request.
  void ScheduleNextSync(const base::TimeDelta& sync_delta);

  // Adds a random jitter to the value of GetPeriod(). The returned
  // TimeDelta will be clamped to be non-negative.
  base::TimeDelta GetJitteredPeriod();

  // Returns the time to wait for the current strategy.
  base::TimeDelta GetPeriod();

  // The delegate handling sync requests when they are fired.
  Delegate* const delegate_;

  // The time to wait until the next refresh when the last sync attempt was
  // successful.
  const base::TimeDelta refresh_period_;

  // The base recovery period for the AGGRESSIVE_RECOVERY strategy before
  // backoffs are applied.
  const base::TimeDelta base_recovery_period_;

  // The maximum percentage (both positively and negatively) that the time to
  // wait between each sync request is jittered. The jitter is randomly applied
  // to each period so we can avoid synchronous calls to the server.
  const double max_jitter_ratio_;

  // The name of the scheduler, used for debugging purposes.
  const std::string scheduler_name_;

  // The current strategy of the scheduler.
  Strategy strategy_;

  // The current state of the scheduler.
  SyncState sync_state_;

  // The number of failed syncs made in a row. Once a sync request succeeds,
  // this counter is reset.
  size_t failure_count_;

  // Timer firing for the next sync request.
  scoped_ptr<base::Timer> timer_;

  base::WeakPtrFactory<SyncSchedulerImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SyncSchedulerImpl);
};

}  // namespace proximity_auth

#endif  // COMPONENTS_PROXIMITY_CRYPTAUTH_CRYPTAUTH_SYNC_SCHEDULER_IMPL_H
