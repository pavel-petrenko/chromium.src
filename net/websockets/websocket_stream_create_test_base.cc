// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/websockets/websocket_stream_create_test_base.h"

#include "base/callback.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/websockets/websocket_basic_handshake_stream.h"
#include "net/websockets/websocket_handshake_request_info.h"
#include "net/websockets/websocket_handshake_response_info.h"
#include "net/websockets/websocket_handshake_stream_create_helper.h"
#include "net/websockets/websocket_stream.h"
#include "url/deprecated_serialized_origin.h"
#include "url/gurl.h"

namespace net {

using HeaderKeyValuePair = WebSocketStreamCreateTestBase::HeaderKeyValuePair;

// A sub-class of WebSocketHandshakeStreamCreateHelper which always sets a
// deterministic key to use in the WebSocket handshake.
class DeterministicKeyWebSocketHandshakeStreamCreateHelper
    : public WebSocketHandshakeStreamCreateHelper {
 public:
  DeterministicKeyWebSocketHandshakeStreamCreateHelper(
      WebSocketStream::ConnectDelegate* connect_delegate,
      const std::vector<std::string>& requested_subprotocols)
      : WebSocketHandshakeStreamCreateHelper(connect_delegate,
                                             requested_subprotocols) {}

  void OnStreamCreated(WebSocketBasicHandshakeStream* stream) override {
    stream->SetWebSocketKeyForTesting("dGhlIHNhbXBsZSBub25jZQ==");
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(
      DeterministicKeyWebSocketHandshakeStreamCreateHelper);
};

class WebSocketStreamCreateTestBase::TestConnectDelegate
    : public WebSocketStream::ConnectDelegate {
 public:
  TestConnectDelegate(WebSocketStreamCreateTestBase* owner,
                      const base::Closure& done_callback)
      : owner_(owner), done_callback_(done_callback) {}

  void OnSuccess(scoped_ptr<WebSocketStream> stream) override {
    stream.swap(owner_->stream_);
    done_callback_.Run();
  }

  void OnFailure(const std::string& message) override {
    owner_->has_failed_ = true;
    owner_->failure_message_ = message;
    done_callback_.Run();
  }

  void OnStartOpeningHandshake(
      scoped_ptr<WebSocketHandshakeRequestInfo> request) override {
    // Can be called multiple times (in the case of HTTP auth). Last call
    // wins.
    owner_->request_info_ = request.Pass();
  }

  void OnFinishOpeningHandshake(
      scoped_ptr<WebSocketHandshakeResponseInfo> response) override {
    if (owner_->response_info_)
      ADD_FAILURE();
    owner_->response_info_ = response.Pass();
  }

  void OnSSLCertificateError(
      scoped_ptr<WebSocketEventInterface::SSLErrorCallbacks>
          ssl_error_callbacks,
      const SSLInfo& ssl_info,
      bool fatal) override {
    owner_->ssl_error_callbacks_ = ssl_error_callbacks.Pass();
    owner_->ssl_info_ = ssl_info;
    owner_->ssl_fatal_ = fatal;
  }

 private:
  WebSocketStreamCreateTestBase* owner_;
  base::Closure done_callback_;
  DISALLOW_COPY_AND_ASSIGN(TestConnectDelegate);
};

WebSocketStreamCreateTestBase::WebSocketStreamCreateTestBase()
    : has_failed_(false), ssl_fatal_(false) {
}

WebSocketStreamCreateTestBase::~WebSocketStreamCreateTestBase() {
}

void WebSocketStreamCreateTestBase::CreateAndConnectStream(
    const std::string& socket_url,
    const std::vector<std::string>& sub_protocols,
    const std::string& origin,
    scoped_ptr<base::Timer> timer) {
  for (size_t i = 0; i < ssl_data_.size(); ++i) {
    scoped_ptr<SSLSocketDataProvider> ssl_data(ssl_data_[i]);
    url_request_context_host_.AddSSLSocketDataProvider(ssl_data.Pass());
  }
  ssl_data_.weak_clear();
  scoped_ptr<WebSocketStream::ConnectDelegate> connect_delegate(
      new TestConnectDelegate(this, connect_run_loop_.QuitClosure()));
  WebSocketStream::ConnectDelegate* delegate = connect_delegate.get();
  scoped_ptr<WebSocketHandshakeStreamCreateHelper> create_helper(
      new DeterministicKeyWebSocketHandshakeStreamCreateHelper(delegate,
                                                               sub_protocols));
  stream_request_ = CreateAndConnectStreamForTesting(
      GURL(socket_url), create_helper.Pass(),
      url::DeprecatedSerializedOrigin(origin),
      url_request_context_host_.GetURLRequestContext(), BoundNetLog(),
      connect_delegate.Pass(),
      timer ? timer.Pass()
            : scoped_ptr<base::Timer>(new base::Timer(false, false)));
}

std::vector<HeaderKeyValuePair>
WebSocketStreamCreateTestBase::RequestHeadersToVector(
    const HttpRequestHeaders& headers) {
  HttpRequestHeaders::Iterator it(headers);
  std::vector<HeaderKeyValuePair> result;
  while (it.GetNext())
    result.push_back(HeaderKeyValuePair(it.name(), it.value()));
  return result;
}

std::vector<HeaderKeyValuePair>
WebSocketStreamCreateTestBase::ResponseHeadersToVector(
    const HttpResponseHeaders& headers) {
  void* iter = NULL;
  std::string name, value;
  std::vector<HeaderKeyValuePair> result;
  while (headers.EnumerateHeaderLines(&iter, &name, &value))
    result.push_back(HeaderKeyValuePair(name, value));
  return result;
}

void WebSocketStreamCreateTestBase::WaitUntilConnectDone() {
  connect_run_loop_.Run();
}

std::vector<std::string> WebSocketStreamCreateTestBase::NoSubProtocols() {
  return std::vector<std::string>();
}

}  // namespace net
