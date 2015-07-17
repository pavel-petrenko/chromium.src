// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/fake_profile_oauth2_token_service.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/thread_task_runner_handle.h"
#include "google_apis/gaia/fake_oauth2_token_service_delegate.h"

FakeProfileOAuth2TokenService::PendingRequest::PendingRequest() {
}

FakeProfileOAuth2TokenService::PendingRequest::~PendingRequest() {
}

FakeProfileOAuth2TokenService::FakeProfileOAuth2TokenService()
    : FakeProfileOAuth2TokenService(
          new FakeOAuth2TokenServiceDelegate(nullptr)) {}

FakeProfileOAuth2TokenService::FakeProfileOAuth2TokenService(
    OAuth2TokenServiceDelegate* delegate)
    : ProfileOAuth2TokenService(delegate),
      auto_post_fetch_response_on_message_loop_(false),
      weak_ptr_factory_(this) {}

FakeProfileOAuth2TokenService::~FakeProfileOAuth2TokenService() {
}

void FakeProfileOAuth2TokenService::IssueAllTokensForAccount(
    const std::string& account_id,
    const std::string& access_token,
    const base::Time& expiration) {
  CompleteRequests(account_id,
                   true,
                   ScopeSet(),
                   GoogleServiceAuthError::AuthErrorNone(),
                   access_token,
                   expiration);
}

void FakeProfileOAuth2TokenService::IssueErrorForAllPendingRequestsForAccount(
    const std::string& account_id,
    const GoogleServiceAuthError& error) {
  CompleteRequests(account_id,
                   true,
                   ScopeSet(),
                   error,
                   std::string(),
                   base::Time());
}

void FakeProfileOAuth2TokenService::IssueTokenForScope(
    const ScopeSet& scope,
    const std::string& access_token,
    const base::Time& expiration) {
  CompleteRequests("",
                   false,
                   scope,
                   GoogleServiceAuthError::AuthErrorNone(),
                   access_token,
                   expiration);
}

void FakeProfileOAuth2TokenService::IssueErrorForScope(
    const ScopeSet& scope,
    const GoogleServiceAuthError& error) {
  CompleteRequests("", false, scope, error, std::string(), base::Time());
}

void FakeProfileOAuth2TokenService::IssueErrorForAllPendingRequests(
    const GoogleServiceAuthError& error) {
  CompleteRequests("", true, ScopeSet(), error, std::string(), base::Time());
}

void FakeProfileOAuth2TokenService::IssueTokenForAllPendingRequests(
    const std::string& access_token,
    const base::Time& expiration) {
  CompleteRequests("",
                   true,
                   ScopeSet(),
                   GoogleServiceAuthError::AuthErrorNone(),
                   access_token,
                   expiration);
}

void FakeProfileOAuth2TokenService::CompleteRequests(
    const std::string& account_id,
    bool all_scopes,
    const ScopeSet& scope,
    const GoogleServiceAuthError& error,
    const std::string& access_token,
    const base::Time& expiration) {
  std::vector<FakeProfileOAuth2TokenService::PendingRequest> requests =
      GetPendingRequests();

  // Walk the requests and notify the callbacks.
  for (std::vector<PendingRequest>::iterator it = requests.begin();
       it != requests.end(); ++it) {
    DCHECK(it->request);

    bool scope_matches = all_scopes || it->scopes == scope;
    bool account_matches = account_id.empty() || account_id == it->account_id;
    if (account_matches && scope_matches)
      it->request->InformConsumer(error, access_token, expiration);
  }
}

std::vector<FakeProfileOAuth2TokenService::PendingRequest>
FakeProfileOAuth2TokenService::GetPendingRequests() {
  std::vector<PendingRequest> valid_requests;
  for (std::vector<PendingRequest>::iterator it = pending_requests_.begin();
       it != pending_requests_.end(); ++it) {
    if (it->request)
      valid_requests.push_back(*it);
  }
  return valid_requests;
}

void FakeProfileOAuth2TokenService::FetchOAuth2Token(
    RequestImpl* request,
    const std::string& account_id,
    net::URLRequestContextGetter* getter,
    const std::string& client_id,
    const std::string& client_secret,
    const ScopeSet& scopes) {
  PendingRequest pending_request;
  pending_request.account_id = account_id;
  pending_request.client_id = client_id;
  pending_request.client_secret = client_secret;
  pending_request.scopes = scopes;
  pending_request.request = request->AsWeakPtr();
  pending_requests_.push_back(pending_request);

  if (auto_post_fetch_response_on_message_loop_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(&FakeProfileOAuth2TokenService::IssueAllTokensForAccount,
                   weak_ptr_factory_.GetWeakPtr(), account_id, "access_token",
                   base::Time::Max()));
  }
}

void FakeProfileOAuth2TokenService::InvalidateAccessTokenImpl(
    const std::string& account_id,
    const std::string& client_id,
    const ScopeSet& scopes,
    const std::string& access_token) {
  // Do nothing, as we don't have a cache from which to remove the token.
}
