// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gcm/engine/instance_id_delete_token_request_handler.h"

#include "base/strings/string_number_conversions.h"
#include "google_apis/gcm/base/gcm_util.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"

namespace gcm {

namespace {

// Request constants.
const char kGMSVersionKey[] = "gmsv";
const char kInstanceIDKey[] = "appid";
const char kSenderKey[] = "sender";
const char kSubtypeKey[] = "X-subtype";
const char kScopeKey[] = "scope";

// Response constants.
const char kTokenPrefix[] = "token=";

}  // namespace

InstanceIDDeleteTokenRequestHandler::InstanceIDDeleteTokenRequestHandler(
    const std::string& instance_id,
    const std::string& authorized_entity,
    const std::string& scope,
    int gcm_version)
    : instance_id_(instance_id),
      authorized_entity_(authorized_entity),
      scope_(scope),
      gcm_version_(gcm_version) {
  DCHECK(!instance_id.empty());
  DCHECK(!authorized_entity.empty());
  DCHECK(!scope.empty());
}

InstanceIDDeleteTokenRequestHandler::~InstanceIDDeleteTokenRequestHandler() {}

void InstanceIDDeleteTokenRequestHandler::BuildRequestBody(std::string* body){
  BuildFormEncoding(kInstanceIDKey, instance_id_, body);
  BuildFormEncoding(kSenderKey, authorized_entity_, body);
  BuildFormEncoding(kScopeKey, scope_, body);
  BuildFormEncoding(kGMSVersionKey, base::IntToString(gcm_version_), body);
  // TODO(jianli): To work around server bug. To be removed when the server fix
  // is deployed.
  BuildFormEncoding(kSubtypeKey, authorized_entity_, body);
}

UnregistrationRequest::Status
InstanceIDDeleteTokenRequestHandler::ParseResponse(
    const net::URLFetcher* source) {
  std::string response;
  if (!source->GetResponseAsString(&response)) {
    DVLOG(1) << "Failed to get response body.";
    return UnregistrationRequest::NO_RESPONSE_BODY;
  }

  if (response.find(kTokenPrefix) == std::string::npos)
    return UnregistrationRequest::RESPONSE_PARSING_FAILED;

  return UnregistrationRequest::SUCCESS;
}

}  // namespace gcm
