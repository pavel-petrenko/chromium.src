// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SUPERVISED_USER_CHILD_ACCOUNTS_CHILD_ACCOUNT_SERVICE_H_
#define CHROME_BROWSER_SUPERVISED_USER_CHILD_ACCOUNTS_CHILD_ACCOUNT_SERVICE_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/supervised_user/child_accounts/family_info_fetcher.h"
#include "chrome/browser/supervised_user/supervised_user_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "net/base/backoff_entry.h"

namespace base {
class FilePath;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

class Profile;

// This class handles detection of child accounts (on sign-in as well as on
// browser restart), and triggers the appropriate behavior (e.g. enable the
// supervised user experience, fetch information about the parent(s)).
class ChildAccountService : public KeyedService,
                            public FamilyInfoFetcher::Consumer,
                            public AccountTrackerService::Observer,
                            public SupervisedUserService::Delegate {
 public:
  ~ChildAccountService() override;

  static bool IsChildAccountDetectionEnabled();

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  void Init();

  // Sets whether the signed-in account is a child account.
  // Public so it can be called on platforms where child account detection
  // happens outside of this class (like Android).
  void SetIsChildAccount(bool is_child_account);

  // Responds whether at least one request for child status was successful.
  // And we got answer whether the profile belongs to a child account or not.
  bool IsChildAccountStatusKnown();

  // KeyedService:
  void Shutdown() override;

  void AddChildStatusReceivedCallback(const base::Closure& callback);

 private:
  friend class ChildAccountServiceFactory;
  // Use |ChildAccountServiceFactory::GetForProfile(...)| to get an instance of
  // this service.
  explicit ChildAccountService(Profile* profile);

  // SupervisedUserService::Delegate implementation.
  bool SetActive(bool active) override;

  // AccountTrackerService::Observer implementation.
  void OnAccountUpdated(const AccountInfo& info) override;

  // FamilyInfoFetcher::Consumer implementation.
  void OnGetFamilyMembersSuccess(
      const std::vector<FamilyInfoFetcher::FamilyMember>& members) override;
  void OnFailure(FamilyInfoFetcher::ErrorCode error) override;

  void StartFetchingFamilyInfo();
  void CancelFetchingFamilyInfo();
  void ScheduleNextFamilyInfoUpdate(base::TimeDelta delay);

  void PropagateChildStatusToUser(bool is_child);

  void SetFirstCustodianPrefs(const FamilyInfoFetcher::FamilyMember& custodian);
  void SetSecondCustodianPrefs(
      const FamilyInfoFetcher::FamilyMember& custodian);
  void ClearFirstCustodianPrefs();
  void ClearSecondCustodianPrefs();

  // Owns us via the KeyedService mechanism.
  Profile* profile_;

  bool active_;

  scoped_ptr<FamilyInfoFetcher> family_fetcher_;
  // If fetching the family info fails, retry with exponential backoff.
  base::OneShotTimer<ChildAccountService> family_fetch_timer_;
  net::BackoffEntry family_fetch_backoff_;

  // Callbacks to run when the user status becomes known.
  std::vector<base::Closure> status_received_callback_list_;

  base::WeakPtrFactory<ChildAccountService> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ChildAccountService);
};

#endif  // CHROME_BROWSER_SUPERVISED_USER_CHILD_ACCOUNTS_CHILD_ACCOUNT_SERVICE_H_
