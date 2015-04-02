// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/signin/oauth2_login_manager.h"

#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/metrics/histogram.h"
#include "base/prefs/pref_service.h"
#include "base/strings/string_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/login/signin/token_handle_util.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/chrome_signin_client_factory.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/common/chrome_switches.h"
#include "chromeos/chromeos_switches.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_client.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "google_apis/gaia/gaia_constants.h"
#include "google_apis/gaia/gaia_urls.h"
#include "net/url_request/url_request_context_getter.h"

namespace chromeos {

namespace {

static const char kServiceScopeGetUserInfo[] =
    "https://www.googleapis.com/auth/userinfo.email";
static const int kMaxRetries = 5;

}  // namespace

OAuth2LoginManager::OAuth2LoginManager(Profile* user_profile)
    : user_profile_(user_profile),
      restore_strategy_(RESTORE_FROM_COOKIE_JAR),
      state_(SESSION_RESTORE_NOT_STARTED),
      weak_factory_(this) {
  GetTokenService()->AddObserver(this);

  // For telemetry, we mark session restore completed to avoid warnings from
  // MergeSessionThrottle.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          chromeos::switches::kDisableGaiaServices)) {
    SetSessionRestoreState(SESSION_RESTORE_DONE);
  }
}

OAuth2LoginManager::~OAuth2LoginManager() {
}

void OAuth2LoginManager::AddObserver(OAuth2LoginManager::Observer* observer) {
  observer_list_.AddObserver(observer);
}

void OAuth2LoginManager::RemoveObserver(
    OAuth2LoginManager::Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void OAuth2LoginManager::RestoreSession(
    net::URLRequestContextGetter* auth_request_context,
    SessionRestoreStrategy restore_strategy,
    const std::string& oauth2_refresh_token,
    const std::string& auth_code) {
  DCHECK(user_profile_);
  auth_request_context_ = auth_request_context;
  restore_strategy_ = restore_strategy;
  refresh_token_ = oauth2_refresh_token;
  oauthlogin_access_token_ = std::string();
  auth_code_ = auth_code;
  session_restore_start_ = base::Time::Now();
  SetSessionRestoreState(OAuth2LoginManager::SESSION_RESTORE_PREPARING);
  ContinueSessionRestore();
}

void OAuth2LoginManager::ContinueSessionRestore() {
  if (restore_strategy_ == RESTORE_FROM_COOKIE_JAR ||
      restore_strategy_ == RESTORE_FROM_AUTH_CODE) {
    FetchOAuth2Tokens();
    return;
  }

  // Save passed OAuth2 refresh token.
  if (restore_strategy_ == RESTORE_FROM_PASSED_OAUTH2_REFRESH_TOKEN) {
    DCHECK(!refresh_token_.empty());
    restore_strategy_ = RESTORE_FROM_SAVED_OAUTH2_REFRESH_TOKEN;
    StoreOAuth2Token();
    return;
  }

  DCHECK(restore_strategy_ == RESTORE_FROM_SAVED_OAUTH2_REFRESH_TOKEN);
  RestoreSessionFromSavedTokens();
}

void OAuth2LoginManager::RestoreSessionFromSavedTokens() {
  ProfileOAuth2TokenService* token_service = GetTokenService();
  const std::string& primary_account_id = GetPrimaryAccountId();
  if (token_service->RefreshTokenIsAvailable(primary_account_id)) {
    VLOG(1) << "OAuth2 refresh token is already loaded.";
    VerifySessionCookies();
  } else {
    VLOG(1) << "Loading OAuth2 refresh token from database.";

    // Flag user with unknown token status in case there are no saved tokens
    // and OnRefreshTokenAvailable is not called. Flagging it here would
    // cause user to go through Gaia in next login to obtain a new refresh
    // token.
    user_manager::UserManager::Get()->SaveUserOAuthStatus(
        primary_account_id, user_manager::User::OAUTH_TOKEN_STATUS_UNKNOWN);

    token_service->LoadCredentials(primary_account_id);
  }
}

void OAuth2LoginManager::Stop() {
  oauth2_token_fetcher_.reset();
  login_verifier_.reset();
}

bool OAuth2LoginManager::ShouldBlockTabLoading() {
  return state_ == SESSION_RESTORE_PREPARING ||
         state_ == SESSION_RESTORE_IN_PROGRESS;
}

void OAuth2LoginManager::OnRefreshTokenAvailable(
    const std::string& account_id) {
  VLOG(1) << "OnRefreshTokenAvailable";

  if (state_ == SESSION_RESTORE_NOT_STARTED)
    return;

  // TODO(fgorski): Once ProfileOAuth2TokenService supports multi-login, make
  // sure to restore session cookies in the context of the correct account_id.

  // Do not validate tokens for supervised users, as they don't actually have
  // oauth2 token.
  if (user_manager::UserManager::Get()->IsLoggedInAsSupervisedUser()) {
    VLOG(1) << "Logged in as supervised user, skip token validation.";
    return;
  }
  // Only restore session cookies for the primary account in the profile.
  if (GetPrimaryAccountId() == account_id) {
    // Token is loaded. Undo the flagging before token loading.
    user_manager::UserManager::Get()->SaveUserOAuthStatus(
        account_id, user_manager::User::OAUTH2_TOKEN_STATUS_VALID);
    VerifySessionCookies();
  }
}

ProfileOAuth2TokenService* OAuth2LoginManager::GetTokenService() {
  return ProfileOAuth2TokenServiceFactory::GetForProfile(user_profile_);
}

const std::string& OAuth2LoginManager::GetPrimaryAccountId() {
  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfile(user_profile_);
  return signin_manager->GetAuthenticatedAccountId();
}

void OAuth2LoginManager::StoreOAuth2Token() {
  const std::string& primary_account_id = GetPrimaryAccountId();
  if (primary_account_id.empty()) {
    GetAccountInfoOfRefreshToken(refresh_token_);
    return;
  }

  UpdateCredentials(primary_account_id);
}

void OAuth2LoginManager::GetAccountInfoOfRefreshToken(
    const std::string& refresh_token) {
  gaia::OAuthClientInfo client_info;
  GaiaUrls* gaia_urls = GaiaUrls::GetInstance();
  client_info.client_id = gaia_urls->oauth2_chrome_client_id();
  client_info.client_secret = gaia_urls->oauth2_chrome_client_secret();

  account_info_fetcher_.reset(new gaia::GaiaOAuthClient(
      auth_request_context_.get()));
  account_info_fetcher_->RefreshToken(client_info, refresh_token,
      std::vector<std::string>(1, kServiceScopeGetUserInfo), kMaxRetries,
      this);
}

void OAuth2LoginManager::UpdateCredentials(const std::string& account_id) {
  DCHECK(!account_id.empty());
  DCHECK(!refresh_token_.empty());
  // |account_id| is assumed to be already canonicalized if it's an email.
  GetTokenService()->UpdateCredentials(account_id, refresh_token_);

  FOR_EACH_OBSERVER(Observer, observer_list_,
                    OnNewRefreshTokenAvaiable(user_profile_));
}

void OAuth2LoginManager::OnRefreshTokenResponse(
    const std::string& access_token,
    int expires_in_seconds) {
  account_info_fetcher_->GetUserInfo(access_token, kMaxRetries, this);
}

void OAuth2LoginManager::OnGetUserInfoResponse(
    scoped_ptr<base::DictionaryValue> user_info) {
  account_info_fetcher_.reset();

  std::string gaia_id;
  std::string email;
  user_info->GetString("id", &gaia_id);
  user_info->GetString("email", &email);

  AccountTrackerService* account_tracker =
      AccountTrackerServiceFactory::GetForProfile(user_profile_);
  account_tracker->SeedAccountInfo(gaia_id, email);
  UpdateCredentials(account_tracker->PickAccountIdForAccount(gaia_id, email));
}

void OAuth2LoginManager::OnOAuthError() {
  account_info_fetcher_.reset();
  LOG(ERROR) << "Account info fetch failed!";
  SetSessionRestoreState(OAuth2LoginManager::SESSION_RESTORE_FAILED);
}

void OAuth2LoginManager::OnNetworkError(int response_code) {
  account_info_fetcher_.reset();
  LOG(ERROR) << "Account info fetch failed! response_code=" << response_code;
  SetSessionRestoreState(OAuth2LoginManager::SESSION_RESTORE_FAILED);
}

void OAuth2LoginManager::FetchOAuth2Tokens() {
  DCHECK(auth_request_context_.get());
  // If we have authenticated cookie jar, get OAuth1 token first, then fetch
  // SID/LSID cookies through OAuthLogin call.
  if (restore_strategy_ == RESTORE_FROM_COOKIE_JAR) {
    SigninClient* signin_client =
        ChromeSigninClientFactory::GetForProfile(user_profile_);
    std::string signin_scoped_device_id =
        signin_client->GetSigninScopedDeviceId();

    oauth2_token_fetcher_.reset(
        new OAuth2TokenFetcher(this, auth_request_context_.get()));
    oauth2_token_fetcher_->StartExchangeFromCookies(std::string(),
                                                    signin_scoped_device_id);
  } else if (restore_strategy_ == RESTORE_FROM_AUTH_CODE) {
    DCHECK(!auth_code_.empty());
    oauth2_token_fetcher_.reset(
        new OAuth2TokenFetcher(this,
                               g_browser_process->system_request_context()));
    oauth2_token_fetcher_->StartExchangeFromAuthCode(auth_code_);
  } else {
    NOTREACHED();
    SetSessionRestoreState(OAuth2LoginManager::SESSION_RESTORE_FAILED);
  }
}

void OAuth2LoginManager::OnOAuth2TokensAvailable(
    const GaiaAuthConsumer::ClientOAuthResult& oauth2_tokens) {
  VLOG(1) << "OAuth2 tokens fetched";
  DCHECK(refresh_token_.empty());
  refresh_token_.assign(oauth2_tokens.refresh_token);
  oauthlogin_access_token_ = oauth2_tokens.access_token;

  auto user = chromeos::ProfileHelper::Get()->GetUserByProfile(user_profile_);
  DCHECK(user);
  if (user) {
    token_handle_util_.reset(
        new TokenHandleUtil(user_manager::UserManager::Get()));
    token_handle_util_->GetTokenHandle(
        user->GetUserID(), oauthlogin_access_token_,
        base::Bind(&OAuth2LoginManager::OnTokenHandleComplete,
                   weak_factory_.GetWeakPtr()));
  }
  StoreOAuth2Token();
}

void OAuth2LoginManager::OnOAuth2TokensFetchFailed() {
  LOG(ERROR) << "OAuth2 tokens fetch failed!";
  RecordSessionRestoreOutcome(SESSION_RESTORE_TOKEN_FETCH_FAILED,
                              SESSION_RESTORE_FAILED);
}

void OAuth2LoginManager::VerifySessionCookies() {
  DCHECK(!login_verifier_.get());
  login_verifier_.reset(
      new OAuth2LoginVerifier(this,
                              g_browser_process->system_request_context(),
                              user_profile_->GetRequestContext(),
                              oauthlogin_access_token_));

  if (restore_strategy_ == RESTORE_FROM_SAVED_OAUTH2_REFRESH_TOKEN) {
    login_verifier_->VerifyUserCookies(user_profile_);
    return;
  }

  RestoreSessionCookies();
}

void OAuth2LoginManager::RestoreSessionCookies() {
  SetSessionRestoreState(SESSION_RESTORE_IN_PROGRESS);
  login_verifier_->VerifyProfileTokens(user_profile_);
}

void OAuth2LoginManager::Shutdown() {
  GetTokenService()->RemoveObserver(this);
  login_verifier_.reset();
  oauth2_token_fetcher_.reset();
}

void OAuth2LoginManager::OnSessionMergeSuccess() {
  VLOG(1) << "OAuth2 refresh and/or GAIA token verification succeeded.";
  RecordSessionRestoreOutcome(SESSION_RESTORE_SUCCESS,
                              SESSION_RESTORE_DONE);
}

void OAuth2LoginManager::OnSessionMergeFailure(bool connection_error) {
  LOG(ERROR) << "OAuth2 refresh and GAIA token verification failed!"
             << " connection_error: " << connection_error;
  RecordSessionRestoreOutcome(SESSION_RESTORE_MERGE_SESSION_FAILED,
                              connection_error ?
                                  SESSION_RESTORE_CONNECTION_FAILED :
                                  SESSION_RESTORE_FAILED);
}

void OAuth2LoginManager::OnListAccountsSuccess(const std::string& data) {
  MergeVerificationOutcome outcome = POST_MERGE_SUCCESS;
  // Let's analyze which accounts we see logged in here:
  std::vector<std::pair<std::string, bool> > accounts;
  gaia::ParseListAccountsData(data, &accounts);
  std::string user_email = gaia::CanonicalizeEmail(GetPrimaryAccountId());
  if (!accounts.empty()) {
    bool found = false;
    bool first = true;
    for (std::vector<std::pair<std::string, bool> >::const_iterator iter =
             accounts.begin();
         iter != accounts.end(); ++iter) {
      if (gaia::CanonicalizeEmail(iter->first) == user_email) {
        found = iter->second;
        break;
      }

      first = false;
    }

    if (!found)
      outcome = POST_MERGE_MISSING_PRIMARY_ACCOUNT;
    else if (!first)
      outcome = POST_MERGE_PRIMARY_NOT_FIRST_ACCOUNT;

  } else {
    outcome = POST_MERGE_NO_ACCOUNTS;
  }

  bool is_pre_merge = (state_ == SESSION_RESTORE_PREPARING);
  RecordCookiesCheckOutcome(is_pre_merge, outcome);
  // If the primary account is missing during the initial cookie freshness
  // check, try to restore GAIA session cookies form the OAuth2 tokens.
  if (is_pre_merge) {
    if (outcome != POST_MERGE_SUCCESS &&
        outcome != POST_MERGE_PRIMARY_NOT_FIRST_ACCOUNT) {
      RestoreSessionCookies();
    } else {
      // We are done with this account, it's GAIA cookies are legit.
      RecordSessionRestoreOutcome(SESSION_RESTORE_NOT_NEEDED,
                                  SESSION_RESTORE_DONE);
    }
  }
}

void OAuth2LoginManager::OnListAccountsFailure(bool connection_error) {
  bool is_pre_merge = (state_ == SESSION_RESTORE_PREPARING);
  RecordCookiesCheckOutcome(
      is_pre_merge,
      connection_error ? POST_MERGE_CONNECTION_FAILED :
                         POST_MERGE_VERIFICATION_FAILED);
  if (is_pre_merge) {
    if (!connection_error) {
      // If we failed to get account list, our cookies might be stale so we
      // need to attempt to restore them.
      RestoreSessionCookies();
    } else {
      RecordSessionRestoreOutcome(SESSION_RESTORE_LISTACCOUNTS_FAILED,
                                  SESSION_RESTORE_CONNECTION_FAILED);
    }
  }
}

void OAuth2LoginManager::RecordSessionRestoreOutcome(
    SessionRestoreOutcome outcome,
    OAuth2LoginManager::SessionRestoreState state) {
  UMA_HISTOGRAM_ENUMERATION("OAuth2Login.SessionRestore",
                            outcome,
                            SESSION_RESTORE_COUNT);
  SetSessionRestoreState(state);
}

// static
void OAuth2LoginManager::RecordCookiesCheckOutcome(
    bool is_pre_merge,
    MergeVerificationOutcome outcome) {
  if (is_pre_merge) {
    UMA_HISTOGRAM_ENUMERATION("OAuth2Login.PreMergeVerification",
                              outcome,
                              POST_MERGE_COUNT);
  } else {
    UMA_HISTOGRAM_ENUMERATION("OAuth2Login.PostMergeVerification",
                              outcome,
                              POST_MERGE_COUNT);
  }
}

void OAuth2LoginManager::SetSessionRestoreState(
    OAuth2LoginManager::SessionRestoreState state) {
  if (state_ == state)
    return;

  state_ = state;
  if (state == OAuth2LoginManager::SESSION_RESTORE_FAILED) {
    UMA_HISTOGRAM_TIMES("OAuth2Login.SessionRestoreTimeToFailure",
                        base::Time::Now() - session_restore_start_);
  } else if (state == OAuth2LoginManager::SESSION_RESTORE_DONE) {
    UMA_HISTOGRAM_TIMES("OAuth2Login.SessionRestoreTimeToSuccess",
                        base::Time::Now() - session_restore_start_);
  }

  FOR_EACH_OBSERVER(Observer, observer_list_,
                    OnSessionRestoreStateChanged(user_profile_, state_));
}

void OAuth2LoginManager::OnTokenHandleComplete(
    const user_manager::UserID& user_id,
    TokenHandleUtil::TokenHandleStatus status) {
  base::MessageLoop::current()->DeleteSoon(FROM_HERE,
                                           token_handle_util_.release());
}

void OAuth2LoginManager::SetSessionRestoreStartForTesting(
    const base::Time& time) {
  session_restore_start_ = time;
}

}  // namespace chromeos
