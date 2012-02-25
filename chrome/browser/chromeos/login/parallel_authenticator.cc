// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/parallel_authenticator.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/string_util.h"
#include "chrome/browser/chromeos/boot_times_loader.h"
#include "chrome/browser/chromeos/cros/cros_library.h"
#include "chrome/browser/chromeos/cros/cryptohome_library.h"
#include "chrome/browser/chromeos/login/authentication_notification_details.h"
#include "chrome/browser/chromeos/login/login_status_consumer.h"
#include "chrome/browser/chromeos/login/ownership_service.h"
#include "chrome/browser/chromeos/login/user_manager.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

using content::BrowserThread;
using file_util::ReadFileToString;

namespace chromeos {

namespace {

// Milliseconds until we timeout our attempt to hit ClientLogin.
const int kClientLoginTimeoutMs = 10000;

// Records status and calls resolver->Resolve().
void TriggerResolve(AuthAttemptState* attempt,
                    AuthAttemptStateResolver* resolver,
                    bool success,
                    int return_code) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  attempt->RecordCryptohomeStatus(success, return_code);
  resolver->Resolve();
}

// Calls TriggerResolve on the IO thread.
void TriggerResolveOnIoThread(AuthAttemptState* attempt,
                              AuthAttemptStateResolver* resolver,
                              bool success,
                              int return_code) {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&TriggerResolve, attempt, resolver, success, return_code));
}

// Calls TriggerResolve on the IO thread while adding login time marker.
void TriggerResolveOnIoThreadWithLoginTimeMarker(
    const std::string& marker_name,
    AuthAttemptState* attempt,
    AuthAttemptStateResolver* resolver,
    bool success,
    int return_code) {
  chromeos::BootTimesLoader::Get()->AddLoginTimeMarker(marker_name, false);
  TriggerResolveOnIoThread(attempt, resolver, success, return_code);
}

// Calls cryptohome's mount method.
void Mount(AuthAttemptState* attempt,
           AuthAttemptStateResolver* resolver,
           bool create_if_missing) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  chromeos::BootTimesLoader::Get()->AddLoginTimeMarker(
      "CryptohomeMount-Start", false);
  CrosLibrary::Get()->GetCryptohomeLibrary()->AsyncMount(
      attempt->username,
      attempt->ascii_hash,
      create_if_missing,
      base::Bind(&TriggerResolveOnIoThreadWithLoginTimeMarker,
                 "CryptohomeMount-End",
                 attempt,
                 resolver));
}

// Calls cryptohome's mount method for guest.
void MountGuest(AuthAttemptState* attempt,
                AuthAttemptStateResolver* resolver) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  CrosLibrary::Get()->GetCryptohomeLibrary()->AsyncMountGuest(
      base::Bind(&TriggerResolveOnIoThreadWithLoginTimeMarker,
                 "CryptohomeMount-End",
                 attempt,
                 resolver));
}

// Calls cryptohome's key migration method.
void Migrate(AuthAttemptState* attempt,
             AuthAttemptStateResolver* resolver,
             bool passing_old_hash,
             const std::string& hash) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  chromeos::BootTimesLoader::Get()->AddLoginTimeMarker(
      "CryptohomeMigrate-Start", false);
  CryptohomeLibrary* lib = CrosLibrary::Get()->GetCryptohomeLibrary();
  if (passing_old_hash) {
    lib->AsyncMigrateKey(
        attempt->username,
        hash,
        attempt->ascii_hash,
        base::Bind(&TriggerResolveOnIoThreadWithLoginTimeMarker,
                   "CryptohomeMount-End",
                   attempt,
                   resolver));
  } else {
    lib->AsyncMigrateKey(
        attempt->username,
        attempt->ascii_hash,
        hash,
        base::Bind(&TriggerResolveOnIoThreadWithLoginTimeMarker,
                   "CryptohomeMount-End",
                   attempt,
                   resolver));
  }
}

// Calls cryptohome's remove method.
void Remove(AuthAttemptState* attempt,
            AuthAttemptStateResolver* resolver) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  chromeos::BootTimesLoader::Get()->AddLoginTimeMarker(
      "CryptohomeRemove-Start", false);
  CrosLibrary::Get()->GetCryptohomeLibrary()->AsyncRemove(
      attempt->username,
      base::Bind(&TriggerResolveOnIoThreadWithLoginTimeMarker,
                 "CryptohomeRemove-End",
                 attempt,
                 resolver));
}

// Calls cryptohome's key check method.
void CheckKey(AuthAttemptState* attempt,
              AuthAttemptStateResolver* resolver) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  CrosLibrary::Get()->GetCryptohomeLibrary()->AsyncCheckKey(
      attempt->username,
      attempt->ascii_hash,
      base::Bind(&TriggerResolveOnIoThread, attempt, resolver));
}

// Resets |current_state| and runs |callback| on the UI thread.
void ResetCryptohomeStatusAndRunCallback(AuthAttemptState* current_state,
                                         base::Closure callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  current_state->ResetCryptohomeStatus();
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, callback);
}

// Returns whether the login failure was connection issue.
bool WasConnectionIssue(const LoginFailure& online_outcome) {
  return ((online_outcome.reason() == LoginFailure::LOGIN_TIMED_OUT) ||
          (online_outcome.error().state() ==
           GoogleServiceAuthError::CONNECTION_FAILED) ||
          (online_outcome.error().state() ==
           GoogleServiceAuthError::REQUEST_CANCELED));
}

}  // namespace

ParallelAuthenticator::ParallelAuthenticator(LoginStatusConsumer* consumer)
    : Authenticator(consumer),
      migrate_attempted_(false),
      remove_attempted_(false),
      mount_guest_attempted_(false),
      check_key_attempted_(false),
      already_reported_success_(false),
      using_oauth_(
          !CommandLine::ForCurrentProcess()->HasSwitch(
              switches::kSkipOAuthLogin)) {
  // If not already owned, this is a no-op.  If it is, this loads the owner's
  // public key off of disk.
  OwnershipService::GetSharedInstance()->StartLoadOwnerKeyAttempt();
}

ParallelAuthenticator::~ParallelAuthenticator() {}

void ParallelAuthenticator::AuthenticateToLogin(
    Profile* profile,
    const std::string& username,
    const std::string& password,
    const std::string& login_token,
    const std::string& login_captcha) {
  std::string canonicalized = Authenticator::Canonicalize(username);
  authentication_profile_ = profile;
  current_state_.reset(
      new AuthAttemptState(
          canonicalized,
          password,
          CrosLibrary::Get()->GetCryptohomeLibrary()->HashPassword(password),
          login_token,
          login_captcha,
          !UserManager::Get()->IsKnownUser(canonicalized)));
  const bool create_if_missing = false;
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&Mount,
                 current_state_.get(),
                 static_cast<AuthAttemptStateResolver*>(this),
                 create_if_missing));

  // ClientLogin authentication check should happen immediately here.
  // We should not try OAuthLogin check until the profile loads.
  if (!using_oauth_) {
    // Initiate ClientLogin-based post authentication.
    current_online_ = new OnlineAttempt(using_oauth_,
                                        current_state_.get(),
                                        this);
    current_online_->Initiate(profile);
  }
}

void ParallelAuthenticator::CompleteLogin(Profile* profile,
                                          const std::string& username,
                                          const std::string& password) {
  std::string canonicalized = Authenticator::Canonicalize(username);
  authentication_profile_ = profile;
  current_state_.reset(
      new AuthAttemptState(
          canonicalized,
          password,
          CrosLibrary::Get()->GetCryptohomeLibrary()->HashPassword(password),
          !UserManager::Get()->IsKnownUser(canonicalized)));
  const bool create_if_missing = false;
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&Mount,
                 current_state_.get(),
                 static_cast<AuthAttemptStateResolver*>(this),
                 create_if_missing));

  if (!using_oauth_) {
    // Test automation needs to disable oauth, but that leads to other
    // services not being able to fetch a token, leading to browser crashes.
    // So initiate ClientLogin-based post authentication.
    // TODO(xiyuan): This should not be required.
    current_online_ = new OnlineAttempt(using_oauth_,
                                        current_state_.get(),
                                        this);
    current_online_->Initiate(profile);
  } else {
    // For login completion from extension, we just need to resolve the current
    // auth attempt state, the rest of OAuth related tasks will be done in
    // parallel.
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&ParallelAuthenticator::ResolveLoginCompletionStatus, this));
  }
}

void ParallelAuthenticator::AuthenticateToUnlock(const std::string& username,
                                                 const std::string& password) {
  current_state_.reset(
      new AuthAttemptState(
          Authenticator::Canonicalize(username),
          CrosLibrary::Get()->GetCryptohomeLibrary()->HashPassword(password)));
  check_key_attempted_ = true;
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&CheckKey,
                 current_state_.get(),
                 static_cast<AuthAttemptStateResolver*>(this)));
}

void ParallelAuthenticator::LoginDemoUser() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  // Note: we use kDemoUser other places to identify if we're in demo mode.
  current_state_.reset(
      new AuthAttemptState(kDemoUser, "", "", "", "", false));
  mount_guest_attempted_ = true;
  MountGuest(current_state_.get(),
             static_cast<AuthAttemptStateResolver*>(this));
}

void ParallelAuthenticator::LoginOffTheRecord() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  current_state_.reset(new AuthAttemptState("", "", "", "", "", false));
  mount_guest_attempted_ = true;
  MountGuest(current_state_.get(),
             static_cast<AuthAttemptStateResolver*>(this));
}

void ParallelAuthenticator::OnDemoUserLoginSuccess() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  VLOG(1) << "Demo user login success";
  // Send notification of success
  AuthenticationNotificationDetails details(true);
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_LOGIN_AUTHENTICATION,
      content::NotificationService::AllSources(),
      content::Details<AuthenticationNotificationDetails>(&details));
  consumer_->OnDemoUserLoginSuccess();
}

void ParallelAuthenticator::OnLoginSuccess(bool request_pending) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  VLOG(1) << "Login success";
  // Send notification of success
  AuthenticationNotificationDetails details(true);
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_LOGIN_AUTHENTICATION,
      content::NotificationService::AllSources(),
      content::Details<AuthenticationNotificationDetails>(&details));
  {
    base::AutoLock for_this_block(success_lock_);
    already_reported_success_ = true;
  }
  consumer_->OnLoginSuccess(current_state_->username,
                            current_state_->password,
                            request_pending,
                            using_oauth_);
}

void ParallelAuthenticator::OnOffTheRecordLoginSuccess() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  // Send notification of success
  AuthenticationNotificationDetails details(true);
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_LOGIN_AUTHENTICATION,
      content::NotificationService::AllSources(),
      content::Details<AuthenticationNotificationDetails>(&details));
  consumer_->OnOffTheRecordLoginSuccess();
}

void ParallelAuthenticator::OnPasswordChangeDetected() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  consumer_->OnPasswordChangeDetected();
}

void ParallelAuthenticator::OnLoginFailure(const LoginFailure& error) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  // Send notification of failure
  AuthenticationNotificationDetails details(false);
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_LOGIN_AUTHENTICATION,
      content::NotificationService::AllSources(),
      content::Details<AuthenticationNotificationDetails>(&details));
  LOG(WARNING) << "Login failed: " << error.GetErrorString();
  consumer_->OnLoginFailure(error);
}

void ParallelAuthenticator::RecordOAuthCheckFailure(
    const std::string& user_name) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(using_oauth_);
  // Mark this account's OAuth token state as invalid in the local state.
  UserManager::Get()->SaveUserOAuthStatus(user_name,
                                          User::OAUTH_TOKEN_STATUS_INVALID);
}

void ParallelAuthenticator::RecoverEncryptedData(
    const std::string& old_password) {
  std::string old_hash =
      CrosLibrary::Get()->GetCryptohomeLibrary()->HashPassword(old_password);
  migrate_attempted_ = true;
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&ResetCryptohomeStatusAndRunCallback,
                 current_state_.get(),
                 base::Bind(&Migrate,
                            current_state_.get(),
                            static_cast<AuthAttemptStateResolver*>(this),
                            true,
                            old_hash)));
}

void ParallelAuthenticator::ResyncEncryptedData() {
  remove_attempted_ = true;
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&ResetCryptohomeStatusAndRunCallback,
                 current_state_.get(),
                 base::Bind(&Remove,
                            current_state_.get(),
                            static_cast<AuthAttemptStateResolver*>(this))));
}

void ParallelAuthenticator::RetryAuth(Profile* profile,
                                      const std::string& username,
                                      const std::string& password,
                                      const std::string& login_token,
                                      const std::string& login_captcha) {
  reauth_state_.reset(
      new AuthAttemptState(
          Authenticator::Canonicalize(username),
          password,
          CrosLibrary::Get()->GetCryptohomeLibrary()->HashPassword(password),
          login_token,
          login_captcha,
          false /* not a new user */));
  // Always use ClientLogin regardless of using_oauth flag. This is because
  // we are unable to renew oauth token on lock screen currently and will
  // stuck with lock screen if we use OAuthLogin here.
  // TODO(xiyuan): Revisit this after we support Gaia in lock screen.
  current_online_ = new OnlineAttempt(false /* using_oauth */,
                                      reauth_state_.get(),
                                      this);
  current_online_->Initiate(profile);
}


void ParallelAuthenticator::Resolve() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  bool request_pending = false;
  bool create = false;
  ParallelAuthenticator::AuthState state = ResolveState();
  VLOG(1) << "Resolved state to: " << state;
  switch (state) {
    case CONTINUE:
    case POSSIBLE_PW_CHANGE:
    case NO_MOUNT:
      // These are intermediate states; we need more info from a request that
      // is still pending.
      break;
    case FAILED_MOUNT:
      // In this case, whether login succeeded or not, we can't log
      // the user in because their data is horked.  So, override with
      // the appropriate failure.
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::Bind(&ParallelAuthenticator::OnLoginFailure, this,
                     LoginFailure(LoginFailure::COULD_NOT_MOUNT_CRYPTOHOME)));
      break;
    case FAILED_REMOVE:
      // In this case, we tried to remove the user's old cryptohome at her
      // request, and the remove failed.
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::Bind(&ParallelAuthenticator::OnLoginFailure, this,
                     LoginFailure(LoginFailure::DATA_REMOVAL_FAILED)));
      break;
    case FAILED_TMPFS:
      // In this case, we tried to mount a tmpfs for guest and failed.
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::Bind(&ParallelAuthenticator::OnLoginFailure, this,
                     LoginFailure(LoginFailure::COULD_NOT_MOUNT_TMPFS)));
      break;
    case CREATE_NEW:
      create = true;
    case RECOVER_MOUNT:
      current_state_->ResetCryptohomeStatus();
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::Bind(&Mount,
                     current_state_.get(),
                     static_cast<AuthAttemptStateResolver*>(this),
                     create));
      break;
    case NEED_OLD_PW:
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::Bind(&ParallelAuthenticator::OnPasswordChangeDetected, this));
      break;
    case ONLINE_FAILED:
      // In this case, we know online login was rejected because the account
      // is disabled or something similarly fatal.  Sending the user through
      // the same path they get when their password is rejected is cleaner
      // for now.
      // TODO(cmasone): optimize this so that we don't send the user through
      // the 'changed password' path when we know doing so won't succeed.
    case NEED_NEW_PW: {
        {
          base::AutoLock for_this_block(success_lock_);
          if (!already_reported_success_) {
            // This allows us to present the same behavior for "online:
            // fail, offline: ok", regardless of the order in which we
            // receive the results.  There will be cases in which we get
            // the online failure some time after the offline success,
            // so we just force all cases in this category to present like this:
            // OnLoginSuccess(..., ..., true) -> OnLoginFailure().
            BrowserThread::PostTask(
                BrowserThread::UI, FROM_HERE,
                base::Bind(&ParallelAuthenticator::OnLoginSuccess, this, true));
          }
        }
        const LoginFailure& login_failure =
            reauth_state_.get() ? reauth_state_->online_outcome() :
                                  current_state_->online_outcome();
        BrowserThread::PostTask(
            BrowserThread::UI, FROM_HERE,
            base::Bind(&ParallelAuthenticator::OnLoginFailure, this,
                       login_failure));
        // Check if we couldn't verify OAuth token here.
        if (using_oauth_ &&
            login_failure.reason() == LoginFailure::NETWORK_AUTH_FAILED) {
          BrowserThread::PostTask(
              BrowserThread::UI, FROM_HERE,
              base::Bind(&ParallelAuthenticator::RecordOAuthCheckFailure, this,
                         (reauth_state_.get() ? reauth_state_->username :
                             current_state_->username)));
        }
        break;
    }
    case HAVE_NEW_PW:
      migrate_attempted_ = true;
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::Bind(&Migrate,
                     reauth_state_.get(),
                     static_cast<AuthAttemptStateResolver*>(this),
                     true,
                     current_state_->ascii_hash));
      break;
    case OFFLINE_LOGIN:
      VLOG(2) << "Offline login";
      // Marking request_pending to false when using OAuth because OAuth related
      // tasks are performed after user profile is mounted and are not performed
      // by ParallelAuthenticator.
      // TODO(xiyuan): Revert this when we support Gaia in lock screen and
      // start to use ParallelAuthenticator's VerifyOAuth1AccessToken again.
      request_pending = using_oauth_ ?
          false :
          !current_state_->online_complete();
      // Fall through.
    case UNLOCK:
      VLOG(2) << "Unlock";
      // Fall through.
    case ONLINE_LOGIN:
      VLOG(2) << "Online login";
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::Bind(&ParallelAuthenticator::OnLoginSuccess, this,
                     request_pending));
      break;
    case DEMO_LOGIN:
      VLOG(2) << "Demo login";
      using_oauth_ = false;
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::Bind(&ParallelAuthenticator::OnDemoUserLoginSuccess, this));
      break;
    case GUEST_LOGIN:
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::Bind(&ParallelAuthenticator::OnOffTheRecordLoginSuccess, this));
      break;
    case LOGIN_FAILED:
      current_state_->ResetCryptohomeStatus();
      BrowserThread::PostTask(BrowserThread::UI,
                              FROM_HERE,
                              base::Bind(
                                  &ParallelAuthenticator::OnLoginFailure,
                                  this,
                                  current_state_->online_outcome()));
      break;
    default:
      NOTREACHED();
      break;
  }
}

ParallelAuthenticator::AuthState ParallelAuthenticator::ResolveState() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  // If we haven't mounted the user's home dir yet, we can't be done.
  // We never get past here if a cryptohome op is still pending.
  // This is an important invariant.
  if (!current_state_->cryptohome_complete())
    return CONTINUE;

  AuthState state = (reauth_state_.get() ? ResolveReauthState() : CONTINUE);
  if (state != CONTINUE)
    return state;

  if (current_state_->cryptohome_outcome())
    state = ResolveCryptohomeSuccessState();
  else
    state = ResolveCryptohomeFailureState();

  DCHECK(current_state_->cryptohome_complete());  // Ensure invariant holds.
  migrate_attempted_ = false;
  remove_attempted_ = false;
  mount_guest_attempted_ = false;
  check_key_attempted_ = false;

  if (state != POSSIBLE_PW_CHANGE &&
      state != NO_MOUNT &&
      state != OFFLINE_LOGIN)
    return state;

  if (current_state_->online_complete()) {
    if (current_state_->online_outcome().reason() == LoginFailure::NONE) {
      // Online attempt succeeded as well, so combine the results.
      return ResolveOnlineSuccessState(state);
    }
    // Online login attempt was rejected or failed to occur.
    return ResolveOnlineFailureState(state);
  }
  // if online isn't complete yet, just return the offline result.
  return state;
}

ParallelAuthenticator::AuthState
ParallelAuthenticator::ResolveReauthState() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (reauth_state_->cryptohome_complete()) {
    if (!reauth_state_->cryptohome_outcome()) {
      // If we've tried to migrate and failed, log the error and just wait
      // til next time the user logs in to migrate their cryptohome key.
      LOG(ERROR) << "Failed to migrate cryptohome key: "
                 << reauth_state_->cryptohome_code();
    }
    reauth_state_.reset(NULL);
    return ONLINE_LOGIN;
  }
  // Haven't tried the migrate yet, must be processing the online auth attempt.
  if (!reauth_state_->online_complete()) {
    NOTREACHED();  // Shouldn't be here at all, if online reauth isn't done!
    return CONTINUE;
  }
  return (reauth_state_->online_outcome().reason() == LoginFailure::NONE) ?
      HAVE_NEW_PW : NEED_NEW_PW;
}

ParallelAuthenticator::AuthState
ParallelAuthenticator::ResolveCryptohomeFailureState() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (remove_attempted_)
    return FAILED_REMOVE;
  if (mount_guest_attempted_)
    return FAILED_TMPFS;
  if (migrate_attempted_)
    return NEED_OLD_PW;
  if (check_key_attempted_)
    return LOGIN_FAILED;

  // Return intermediate states in the following cases:
  // 1. When there is a parallel online attempt to resolve them later;
  //    This is the case with legacy ClientLogin flow;
  // 2. When there is an online result to use;
  //    This is the case after user finishes Gaia login;
  if (current_online_.get() || current_state_->online_complete()) {
    if (current_state_->cryptohome_code() ==
        cryptohome::MOUNT_ERROR_KEY_FAILURE) {
      // If we tried a mount but they used the wrong key, we may need to
      // ask the user for her old password.  We'll only know once we've
      // done the online check.
      return POSSIBLE_PW_CHANGE;
    }
    if (current_state_->cryptohome_code() ==
        cryptohome::MOUNT_ERROR_USER_DOES_NOT_EXIST) {
      // If we tried a mount but the user did not exist, then we should wait
      // for online login to succeed and try again with the "create" flag set.
      return NO_MOUNT;
    }
  }

  return FAILED_MOUNT;
}

ParallelAuthenticator::AuthState
ParallelAuthenticator::ResolveCryptohomeSuccessState() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (remove_attempted_)
    return CREATE_NEW;
  if (mount_guest_attempted_) {
    if (current_state_->username == kDemoUser)
      return DEMO_LOGIN;
    else
      return GUEST_LOGIN;
  }
  if (migrate_attempted_)
    return RECOVER_MOUNT;
  if (check_key_attempted_)
    return UNLOCK;
  return OFFLINE_LOGIN;
}

ParallelAuthenticator::AuthState
ParallelAuthenticator::ResolveOnlineFailureState(
    ParallelAuthenticator::AuthState offline_state) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (offline_state == OFFLINE_LOGIN) {
    if (WasConnectionIssue(current_state_->online_outcome())) {
      // Couldn't do an online check, so just go with the offline result.
      return OFFLINE_LOGIN;
    }
    // Otherwise, online login was rejected!
    if (current_state_->online_outcome().error().state() ==
        GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS) {
      return NEED_NEW_PW;
    }
    return ONLINE_FAILED;
  }
  return LOGIN_FAILED;
}

ParallelAuthenticator::AuthState
ParallelAuthenticator::ResolveOnlineSuccessState(
    ParallelAuthenticator::AuthState offline_state) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  switch (offline_state) {
    case POSSIBLE_PW_CHANGE:
      return NEED_OLD_PW;
    case NO_MOUNT:
      return CREATE_NEW;
    case OFFLINE_LOGIN:
      return ONLINE_LOGIN;
    default:
      NOTREACHED();
      return offline_state;
  }
}

void ParallelAuthenticator::ResolveLoginCompletionStatus() {
  // Shortcut online state resolution process.
  current_state_->RecordOnlineLoginStatus(LoginFailure::None());
  Resolve();
}

}  // namespace chromeos
