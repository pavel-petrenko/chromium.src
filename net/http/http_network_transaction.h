// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_NETWORK_TRANSACTION_H_
#define NET_HTTP_HTTP_NETWORK_TRANSACTION_H_

#include <string>

#include "base/basictypes.h"
#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/time/time.h"
#include "net/base/net_log.h"
#include "net/base/request_priority.h"
#include "net/http/http_auth.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_info.h"
#include "net/http/http_stream_factory.h"
#include "net/http/http_transaction.h"
#include "net/proxy/proxy_service.h"
#include "net/ssl/ssl_config_service.h"
#include "net/websockets/websocket_handshake_stream_base.h"

namespace net {

class ClientSocketHandle;
class HttpAuthController;
class HttpNetworkSession;
class HttpStream;
class HttpStreamRequest;
class IOBuffer;
class ProxyInfo;
class SpdySession;
struct HttpRequestInfo;

class NET_EXPORT_PRIVATE HttpNetworkTransaction
    : public HttpTransaction,
      public HttpStreamRequest::Delegate {
 public:
  HttpNetworkTransaction(RequestPriority priority,
                         HttpNetworkSession* session);

  ~HttpNetworkTransaction() override;

  // HttpTransaction methods:
  int Start(const HttpRequestInfo* request_info,
            const CompletionCallback& callback,
            const BoundNetLog& net_log) override;
  int RestartIgnoringLastError(const CompletionCallback& callback) override;
  int RestartWithCertificate(X509Certificate* client_cert,
                             const CompletionCallback& callback) override;
  int RestartWithAuth(const AuthCredentials& credentials,
                      const CompletionCallback& callback) override;
  bool IsReadyToRestartForAuth() override;

  int Read(IOBuffer* buf,
           int buf_len,
           const CompletionCallback& callback) override;
  void StopCaching() override;
  bool GetFullRequestHeaders(HttpRequestHeaders* headers) const override;
  int64 GetTotalReceivedBytes() const override;
  void DoneReading() override;
  const HttpResponseInfo* GetResponseInfo() const override;
  LoadState GetLoadState() const override;
  UploadProgress GetUploadProgress() const override;
  void SetQuicServerInfo(QuicServerInfo* quic_server_info) override;
  bool GetLoadTimingInfo(LoadTimingInfo* load_timing_info) const override;
  void SetPriority(RequestPriority priority) override;
  void SetWebSocketHandshakeStreamCreateHelper(
      WebSocketHandshakeStreamBase::CreateHelper* create_helper) override;
  void SetBeforeNetworkStartCallback(
      const BeforeNetworkStartCallback& callback) override;
  void SetBeforeProxyHeadersSentCallback(
      const BeforeProxyHeadersSentCallback& callback) override;
  int ResumeNetworkStart() override;

  // HttpStreamRequest::Delegate methods:
  void OnStreamReady(const SSLConfig& used_ssl_config,
                     const ProxyInfo& used_proxy_info,
                     HttpStream* stream) override;
  void OnWebSocketHandshakeStreamReady(
      const SSLConfig& used_ssl_config,
      const ProxyInfo& used_proxy_info,
      WebSocketHandshakeStreamBase* stream) override;
  void OnStreamFailed(int status, const SSLConfig& used_ssl_config) override;
  void OnCertificateError(int status,
                          const SSLConfig& used_ssl_config,
                          const SSLInfo& ssl_info) override;
  void OnNeedsProxyAuth(const HttpResponseInfo& response_info,
                        const SSLConfig& used_ssl_config,
                        const ProxyInfo& used_proxy_info,
                        HttpAuthController* auth_controller) override;
  void OnNeedsClientAuth(const SSLConfig& used_ssl_config,
                         SSLCertRequestInfo* cert_info) override;
  void OnHttpsProxyTunnelResponse(const HttpResponseInfo& response_info,
                                  const SSLConfig& used_ssl_config,
                                  const ProxyInfo& used_proxy_info,
                                  HttpStream* stream) override;

 private:
  friend class HttpNetworkTransactionSSLTest;

  FRIEND_TEST_ALL_PREFIXES(HttpNetworkTransactionTest,
                           ResetStateForRestart);
  FRIEND_TEST_ALL_PREFIXES(SpdyNetworkTransactionTest,
                           WindowUpdateReceived);
  FRIEND_TEST_ALL_PREFIXES(SpdyNetworkTransactionTest,
                           WindowUpdateSent);
  FRIEND_TEST_ALL_PREFIXES(SpdyNetworkTransactionTest,
                           WindowUpdateOverflow);
  FRIEND_TEST_ALL_PREFIXES(SpdyNetworkTransactionTest,
                           FlowControlStallResume);
  FRIEND_TEST_ALL_PREFIXES(SpdyNetworkTransactionTest,
                           FlowControlStallResumeAfterSettings);
  FRIEND_TEST_ALL_PREFIXES(SpdyNetworkTransactionTest,
                           FlowControlNegativeSendWindowSize);

  enum State {
    STATE_NOTIFY_BEFORE_CREATE_STREAM,
    STATE_CREATE_STREAM,
    STATE_CREATE_STREAM_COMPLETE,
    STATE_INIT_STREAM,
    STATE_INIT_STREAM_COMPLETE,
    STATE_GENERATE_PROXY_AUTH_TOKEN,
    STATE_GENERATE_PROXY_AUTH_TOKEN_COMPLETE,
    STATE_GENERATE_SERVER_AUTH_TOKEN,
    STATE_GENERATE_SERVER_AUTH_TOKEN_COMPLETE,
    STATE_INIT_REQUEST_BODY,
    STATE_INIT_REQUEST_BODY_COMPLETE,
    STATE_BUILD_REQUEST,
    STATE_BUILD_REQUEST_COMPLETE,
    STATE_SEND_REQUEST,
    STATE_SEND_REQUEST_COMPLETE,
    STATE_READ_HEADERS,
    STATE_READ_HEADERS_COMPLETE,
    STATE_READ_BODY,
    STATE_READ_BODY_COMPLETE,
    STATE_DRAIN_BODY_FOR_AUTH_RESTART,
    STATE_DRAIN_BODY_FOR_AUTH_RESTART_COMPLETE,
    STATE_NONE
  };

  bool is_https_request() const;

  // Returns true if the request is using an HTTP(S) proxy without being
  // tunneled via the CONNECT method.
  bool UsingHttpProxyWithoutTunnel() const;

  void DoCallback(int result);
  void OnIOComplete(int result);

  // Runs the state transition loop.
  int DoLoop(int result);

  // Each of these methods corresponds to a State value.  Those with an input
  // argument receive the result from the previous state.  If a method returns
  // ERR_IO_PENDING, then the result from OnIOComplete will be passed to the
  // next state method as the result arg.
  int DoNotifyBeforeCreateStream();
  int DoCreateStream();
  int DoCreateStreamComplete(int result);
  int DoInitStream();
  int DoInitStreamComplete(int result);
  int DoGenerateProxyAuthToken();
  int DoGenerateProxyAuthTokenComplete(int result);
  int DoGenerateServerAuthToken();
  int DoGenerateServerAuthTokenComplete(int result);
  int DoInitRequestBody();
  int DoInitRequestBodyComplete(int result);
  int DoBuildRequest();
  int DoBuildRequestComplete(int result);
  int DoSendRequest();
  int DoSendRequestComplete(int result);
  int DoReadHeaders();
  int DoReadHeadersComplete(int result);
  int DoReadBody();
  int DoReadBodyComplete(int result);
  int DoDrainBodyForAuthRestart();
  int DoDrainBodyForAuthRestartComplete(int result);

  void BuildRequestHeaders(bool using_http_proxy_without_tunnel);

  // Writes a log message to help debugging in the field when we block a proxy
  // response to a CONNECT request.
  void LogBlockedTunnelResponse(int response_code) const;

  // Called to handle a client certificate request.
  int HandleCertificateRequest(int error);

  // Called wherever ERR_HTTP_1_1_REQUIRED or
  // ERR_PROXY_HTTP_1_1_REQUIRED has to be handled.
  int HandleHttp11Required(int error);

  // Called to possibly handle a client authentication error.
  void HandleClientAuthError(int error);

  // Called to possibly recover from an SSL handshake error.  Sets next_state_
  // and returns OK if recovering from the error.  Otherwise, the same error
  // code is returned.
  int HandleSSLHandshakeError(int error);

  // Called to possibly recover from the given error.  Sets next_state_ and
  // returns OK if recovering from the error.  Otherwise, the same error code
  // is returned.
  int HandleIOError(int error);

  // Gets the response headers from the HttpStream.
  HttpResponseHeaders* GetResponseHeaders() const;

  // Called when the socket is unexpectedly closed.  Returns true if the request
  // should be resent in case of a socket reuse/close race.
  bool ShouldResendRequest() const;

  // Resets the connection and the request headers for resend.  Called when
  // ShouldResendRequest() is true.
  void ResetConnectionAndRequestForResend();

  // Sets up the state machine to restart the transaction with auth.
  void PrepareForAuthRestart(HttpAuth::Target target);

  // Called when we don't need to drain the response body or have drained it.
  // Resets |connection_| unless |keep_alive| is true, then calls
  // ResetStateForRestart.  Sets |next_state_| appropriately.
  void DidDrainBodyForAuthRestart(bool keep_alive);

  // Resets the members of the transaction so it can be restarted.
  void ResetStateForRestart();

  // Resets the members of the transaction, except |stream_|, which needs
  // to be maintained for multi-round auth.
  void ResetStateForAuthRestart();

  // Returns true if we should try to add a Proxy-Authorization header
  bool ShouldApplyProxyAuth() const;

  // Returns true if we should try to add an Authorization header.
  bool ShouldApplyServerAuth() const;

  // Handles HTTP status code 401 or 407.
  // HandleAuthChallenge() returns a network error code, or OK on success.
  // May update |pending_auth_target_| or |response_.auth_challenge|.
  int HandleAuthChallenge();

  // Returns true if we have auth credentials for the given target.
  bool HaveAuth(HttpAuth::Target target) const;

  // Get the {scheme, host, path, port} for the authentication target
  GURL AuthURL(HttpAuth::Target target) const;

  // Returns true if this transaction is for a WebSocket handshake
  bool ForWebSocketHandshake() const;

  // Debug helper.
  static std::string DescribeState(State state);

  void SetStream(HttpStream* stream);

  scoped_refptr<HttpAuthController>
      auth_controllers_[HttpAuth::AUTH_NUM_TARGETS];

  // Whether this transaction is waiting for proxy auth, server auth, or is
  // not waiting for any auth at all. |pending_auth_target_| is read and
  // cleared by RestartWithAuth().
  HttpAuth::Target pending_auth_target_;

  CompletionCallback io_callback_;
  CompletionCallback callback_;

  HttpNetworkSession* session_;

  BoundNetLog net_log_;
  const HttpRequestInfo* request_;
  RequestPriority priority_;
  HttpResponseInfo response_;

  // |proxy_info_| is the ProxyInfo used by the HttpStreamRequest.
  ProxyInfo proxy_info_;

  scoped_ptr<HttpStreamRequest> stream_request_;
  scoped_ptr<HttpStream> stream_;

  // True if we've validated the headers that the stream parser has returned.
  bool headers_valid_;

  SSLConfig server_ssl_config_;
  SSLConfig proxy_ssl_config_;
  // fallback_error_code contains the error code that caused the last TLS
  // fallback. If the fallback connection results in
  // ERR_SSL_INAPPROPRIATE_FALLBACK (i.e. the server indicated that the
  // fallback should not have been needed) then we use this value to return the
  // original error that triggered the fallback.
  int fallback_error_code_;

  HttpRequestHeaders request_headers_;

  // The size in bytes of the buffer we use to drain the response body that
  // we want to throw away.  The response body is typically a small error
  // page just a few hundred bytes long.
  static const int kDrainBodyBufferSize = 1024;

  // User buffer and length passed to the Read method.
  scoped_refptr<IOBuffer> read_buf_;
  int read_buf_len_;

  // Total number of bytes received on streams for this transaction.
  int64 total_received_bytes_;

  // When the transaction started / finished sending the request, including
  // the body, if present.
  base::TimeTicks send_start_time_;
  base::TimeTicks send_end_time_;

  // The next state in the state machine.
  State next_state_;

  // True when the tunnel is in the process of being established - we can't
  // read from the socket until the tunnel is done.
  bool establishing_tunnel_;

  // The helper object to use to create WebSocketHandshakeStreamBase
  // objects. Only relevant when establishing a WebSocket connection.
  WebSocketHandshakeStreamBase::CreateHelper*
      websocket_handshake_stream_base_create_helper_;

  BeforeNetworkStartCallback before_network_start_callback_;
  BeforeProxyHeadersSentCallback before_proxy_headers_sent_callback_;

  DISALLOW_COPY_AND_ASSIGN(HttpNetworkTransaction);
};

}  // namespace net

#endif  // NET_HTTP_HTTP_NETWORK_TRANSACTION_H_
