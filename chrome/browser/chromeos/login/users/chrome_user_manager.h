// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_USERS_CHROME_USER_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_USERS_CHROME_USER_MANAGER_H_

#include "base/basictypes.h"
#include "base/memory/ref_counted.h"
#include "base/task_runner.h"
#include "chrome/browser/chromeos/login/users/affiliation.h"
#include "chrome/browser/chromeos/login/users/user_manager_interface.h"
#include "components/user_manager/user_manager_base.h"

namespace chromeos {

// Chrome specific interface of the UserManager.
class ChromeUserManager : public user_manager::UserManagerBase,
                          public UserManagerInterface {
 public:
  ChromeUserManager(scoped_refptr<base::TaskRunner> task_runner,
                    scoped_refptr<base::TaskRunner> blocking_task_runner);
  ~ChromeUserManager() override;

  // Returns current ChromeUserManager or NULL if instance hasn't been
  // yet initialized.
  static ChromeUserManager* Get();

  // Helper method for sorting out of user list only users that can create
  // supervised users.
  static user_manager::UserList GetUsersAllowedAsSupervisedUserManagers(
      const user_manager::UserList& user_list);

  // Sets affiliation status for the user |user_id| judging by
  // |user_affiliation_ids| and device affiliation IDs.
  virtual void SetUserAffiliation(
      const std::string& user_email,
      const AffiliationIDSet& user_affiliation_ids) = 0;

  DISALLOW_COPY_AND_ASSIGN(ChromeUserManager);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_USERS_CHROME_USER_MANAGER_H_
