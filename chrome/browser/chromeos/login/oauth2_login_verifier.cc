// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/oauth2_login_verifier.h"

#include <vector>

#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "base/string_util.h"
#include "base/stringprintf.h"
#include "base/time.h"
#include "chrome/browser/chromeos/cros/cros_library.h"
#include "chrome/browser/chromeos/cros/network_library.h"
#include "content/public/browser/browser_thread.h"
#include "google_apis/gaia/gaia_constants.h"
#include "google_apis/gaia/gaia_urls.h"

using content::BrowserThread;

namespace {

// OAuth token request max retry count.
const int kMaxRequestAttemptCount = 5;
// OAuth token request retry delay in milliseconds.
const int kRequestRestartDelay = 3000;

}  // namespace

namespace chromeos {

OAuth2LoginVerifier::OAuth2LoginVerifier(
    OAuth2LoginVerifier::Delegate* delegate,
    net::URLRequestContextGetter* system_request_context,
    net::URLRequestContextGetter* user_request_context)
    : delegate_(delegate),
      ALLOW_THIS_IN_INITIALIZER_LIST(token_fetcher_(
          this, system_request_context)),
      ALLOW_THIS_IN_INITIALIZER_LIST(gaia_system_fetcher_(
          this, std::string(GaiaConstants::kChromeOSSource),
          system_request_context)),
      ALLOW_THIS_IN_INITIALIZER_LIST(gaia_fetcher_(
          this, std::string(GaiaConstants::kChromeOSSource),
          user_request_context)),
      retry_count_(0) {
  DCHECK(delegate);
}

OAuth2LoginVerifier::~OAuth2LoginVerifier() {
}

void OAuth2LoginVerifier::VerifyTokens(const std::string& oauth2_refresh_token,
                                       const std::string& gaia_token) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (CrosLibrary::Get()->libcros_loaded()) {
    // Delay the verification if the network is not connected or on a captive
    // portal.
    const Network* network =
        CrosLibrary::Get()->GetNetworkLibrary()->active_network();
    if (!network || !network->connected() || network->restricted_pool()) {
      // If network is offline, defer the token fetching until online.
      VLOG(1) << "Network is offline.  Deferring OAuth2 access token fetch.";
      BrowserThread::PostDelayedTask(
          BrowserThread::UI, FROM_HERE,
          base::Bind(&OAuth2LoginVerifier::VerifyTokens,
                     AsWeakPtr(),
                     oauth2_refresh_token,
                     gaia_token),
          base::TimeDelta::FromMilliseconds(kRequestRestartDelay));
      return;
    }
  }

  access_token_.clear();
  refresh_token_ = oauth2_refresh_token;
  gaia_token_ = gaia_token;
  if (gaia_token_.empty()) {
    // If we have no token for GAIA service and can't merge the session
    // immediately, attempt to perform OAuthLogin and reconstruct all service
    // tokens.
    RestoreSessionFromOAuth2RefreshToken();
  } else {
    // If GAIA token is present, we can attempt to shortcut the process straight
    // to /MergeSession call.
    RestoreSessionFromGaiaToken();
  }
}


void OAuth2LoginVerifier::RestoreSessionFromOAuth2RefreshToken() {
  type_ = RESTORE_FROM_OAUTH2_REFRESH_TOKEN;
  StartFetchingOAuthLoginAccessToken();
}

void OAuth2LoginVerifier::RestoreSessionFromGaiaToken() {
  type_ = RESTORE_FROM_GAIA_TOKEN;
  StartMergeSession();
}

void OAuth2LoginVerifier::StartFetchingOAuthLoginAccessToken() {
  access_token_.clear();
  gaia_token_.clear();
  std::vector<std::string> scopes;
  scopes.push_back(GaiaUrls::GetInstance()->oauth1_login_scope());
  token_fetcher_.Start(GaiaUrls::GetInstance()->oauth2_chrome_client_id(),
                       GaiaUrls::GetInstance()->oauth2_chrome_client_secret(),
                       refresh_token_,
                       scopes);
}

void OAuth2LoginVerifier::OnGetTokenSuccess(
    const std::string& access_token, const base::Time& expiration_time) {
  LOG(INFO) << "Got OAuth2 access token!";
  retry_count_ = 0;
  access_token_ = access_token;
  StartOAuthLoginForUberToken();
}

void OAuth2LoginVerifier::OnGetTokenFailure(
    const GoogleServiceAuthError& error) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  LOG(ERROR) << "Failed to get OAuth2 access token, "
             << " error: " << error.state();
  RetryOnError(
      "GetOAuth2AccessToken", error,
      base::Bind(&OAuth2LoginVerifier::StartFetchingOAuthLoginAccessToken,
                 AsWeakPtr()),
      base::Bind(&Delegate::OnOAuthLoginFailure,
                 base::Unretained(delegate_)));
}

void OAuth2LoginVerifier::StartOAuthLoginForUberToken() {
  // No service will fetch us uber auth token.
  gaia_system_fetcher_.StartTokenFetchForUberAuthExchange(access_token_);
}


void OAuth2LoginVerifier::OnUberAuthTokenSuccess(
    const std::string& uber_token) {
  LOG(INFO) << "OAuthLogin(uber_token) successful!";
  retry_count_ = 0;
  gaia_token_ = uber_token;
  StartMergeSession();
}

void OAuth2LoginVerifier::OnUberAuthTokenFailure(
    const GoogleServiceAuthError& error) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  LOG(ERROR) << "OAuthLogin(uber_token) failed,"
             << " error: " << error.state();
  RetryOnError("OAuthLoginUberToken", error,
               base::Bind(&OAuth2LoginVerifier::StartOAuthLoginForUberToken,
                          AsWeakPtr()),
               base::Bind(&Delegate::OnOAuthLoginFailure,
                          base::Unretained(delegate_)));
}

void OAuth2LoginVerifier::StartOAuthLoginForGaiaCredentials() {
  // No service will fetch us uber auth token.
  gaia_system_fetcher_.StartOAuthLogin(access_token_, EmptyString());
}

void OAuth2LoginVerifier::OnClientLoginSuccess(
    const ClientLoginResult& gaia_credentials) {
  LOG(INFO) << "OAuthLogin(SID+LSID) successful!";
  retry_count_ = 0;
  gaia_credentials_ = gaia_credentials;
  delegate_->OnOAuthLoginSuccess(gaia_credentials_);
}

void OAuth2LoginVerifier::OnClientLoginFailure(
    const GoogleServiceAuthError& error) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  LOG(ERROR) << "OAuthLogin(SID+LSID failed),"
             << " error: " << error.state();
  RetryOnError(
      "OAuthLoginGaiaCred", error,
      base::Bind(&OAuth2LoginVerifier::StartOAuthLoginForGaiaCredentials,
                 AsWeakPtr()),
      base::Bind(&Delegate::OnOAuthLoginFailure,
                 base::Unretained(delegate_)));
}

void OAuth2LoginVerifier::StartMergeSession() {
  DCHECK(!gaia_token_.empty());
  gaia_fetcher_.StartMergeSession(gaia_token_);
}

void OAuth2LoginVerifier::OnMergeSessionSuccess(const std::string& data) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  LOG(INFO) << "MergeSession successful.";
  UMA_HISTOGRAM_ENUMERATION("OAuth2Login.MergeSessionOrigin", type_, 2);
  delegate_->OnSessionMergeSuccess();
  // Get GAIA credentials needed to kick off TokenService and friends.
  StartOAuthLoginForGaiaCredentials();
}

void OAuth2LoginVerifier::OnMergeSessionFailure(
    const GoogleServiceAuthError& error) {
  LOG(ERROR) << "Failed MergeSession request,"
             << " error: " << error.state();
  // If MergeSession from GAIA service token fails, retry the session restore
  // from OAuth2 refresh token. If that failed too, signal the delegate.
  RetryOnError(
      (type_ == RESTORE_FROM_GAIA_TOKEN) ?
           "MergeSessionGAIA" : "MergeSessionOAuth2",
      error,
      base::Bind(&OAuth2LoginVerifier::StartMergeSession,
                 AsWeakPtr()),
      (type_ == RESTORE_FROM_GAIA_TOKEN) ?
          base::Bind(&OAuth2LoginVerifier::RestoreSessionFromOAuth2RefreshToken,
                     AsWeakPtr()) :
          base::Bind(&Delegate::OnSessionMergeFailure,
                     base::Unretained(delegate_)));
}

void OAuth2LoginVerifier::RetryOnError(const char* operation_id,
                                       const GoogleServiceAuthError& error,
                                       const base::Closure& task_to_retry,
                                       const base::Closure& error_handler) {
  if ((error.state() == GoogleServiceAuthError::CONNECTION_FAILED ||
       error.state() == GoogleServiceAuthError::SERVICE_UNAVAILABLE ||
       error.state() == GoogleServiceAuthError::REQUEST_CANCELED) &&
      retry_count_ < kMaxRequestAttemptCount) {
    retry_count_++;
    UMA_HISTOGRAM_ENUMERATION(
        base::StringPrintf("OAuth2Login.%sWithRetry", operation_id),
        error.state(),
        GoogleServiceAuthError::NUM_STATES);
    BrowserThread::PostDelayedTask(
        BrowserThread::UI, FROM_HERE, task_to_retry,
        base::TimeDelta::FromMilliseconds(kRequestRestartDelay));
    return;
  }

  LOG(ERROR) << "Unrecoverable error or retry count max reached for "
             << operation_id;
  UMA_HISTOGRAM_ENUMERATION(
      base::StringPrintf("OAuth2Login.%sFailureCause", operation_id),
      error.state(),
      GoogleServiceAuthError::NUM_STATES);
  error_handler.Run();
}

}  // namespace chromeos
