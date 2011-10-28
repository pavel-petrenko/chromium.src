// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_PIPELINED_CONNECTION_IMPL_H_
#define NET_HTTP_HTTP_PIPELINED_CONNECTION_IMPL_H_
#pragma once

#include <map>
#include <queue>
#include <string>

#include "base/basictypes.h"
#include "base/memory/linked_ptr.h"
#include "base/task.h"
#include "net/base/completion_callback.h"
#include "net/base/net_export.h"
#include "net/base/net_log.h"
#include "net/base/ssl_config_service.h"
#include "net/base/upload_data_stream.h"
#include "net/http/http_pipelined_connection.h"
#include "net/http/http_request_info.h"
#include "net/http/http_stream_parser.h"
#include "net/proxy/proxy_info.h"

namespace net {

class ClientSocketHandle;
class GrowableIOBuffer;
class HttpRequestHeaders;
class HttpResponseInfo;
class IOBuffer;
class SSLCertRequestInfo;
class SSLInfo;

// This class manages all of the state for a single pipelined connection. It
// tracks the order that HTTP requests are sent and enforces that the
// subsequent reads occur in the appropriate order.
//
// If an error occurs related to pipelining, ERR_PIPELINE_EVICTION will be
// returned to the client. This indicates the client should retry the request
// without pipelining.
class NET_EXPORT_PRIVATE HttpPipelinedConnectionImpl
    : public HttpPipelinedConnection {
 public:
  HttpPipelinedConnectionImpl(ClientSocketHandle* connection,
                              Delegate* delegate,
                              const SSLConfig& used_ssl_config,
                              const ProxyInfo& used_proxy_info,
                              const BoundNetLog& net_log,
                              bool was_npn_negotiated);
  virtual ~HttpPipelinedConnectionImpl();

  // HttpPipelinedConnection interface.

  // Used by HttpStreamFactoryImpl and friends.
  virtual HttpPipelinedStream* CreateNewStream() OVERRIDE;

  // Used by HttpPipelinedHost.
  virtual int depth() const OVERRIDE;
  virtual bool usable() const OVERRIDE;
  virtual bool active() const OVERRIDE;

  // Used by HttpStreamFactoryImpl.
  virtual const SSLConfig& used_ssl_config() const OVERRIDE;
  virtual const ProxyInfo& used_proxy_info() const OVERRIDE;
  virtual const NetLog::Source& source() const OVERRIDE;
  virtual bool was_npn_negotiated() const OVERRIDE;

  // Used by HttpPipelinedStream.

  // Notifies this pipeline that a stream is no longer using it.
  void OnStreamDeleted(int pipeline_id);

  // Effective implementation of HttpStream. Note that we don't directly
  // implement that interface. Instead, these functions will be called by the
  // pass-through methods in HttpPipelinedStream.
  void InitializeParser(int pipeline_id,
                        const HttpRequestInfo* request,
                        const BoundNetLog& net_log);

  int SendRequest(int pipeline_id,
                  const std::string& request_line,
                  const HttpRequestHeaders& headers,
                  UploadDataStream* request_body,
                  HttpResponseInfo* response,
                  OldCompletionCallback* callback);

  int ReadResponseHeaders(int pipeline_id,
                          OldCompletionCallback* callback);

  int ReadResponseBody(int pipeline_id,
                       IOBuffer* buf, int buf_len,
                       OldCompletionCallback* callback);

  void Close(int pipeline_id,
             bool not_reusable);

  uint64 GetUploadProgress(int pipeline_id) const;

  HttpResponseInfo* GetResponseInfo(int pipeline_id);

  bool IsResponseBodyComplete(int pipeline_id) const;

  bool CanFindEndOfResponse(int pipeline_id) const;

  bool IsMoreDataBuffered(int pipeline_id) const;

  bool IsConnectionReused(int pipeline_id) const;

  void SetConnectionReused(int pipeline_id);

  void GetSSLInfo(int pipeline_id,
                  SSLInfo* ssl_info);

  void GetSSLCertRequestInfo(int pipeline_id,
                             SSLCertRequestInfo* cert_request_info);

 private:
  enum StreamState {
    STREAM_CREATED,
    STREAM_BOUND,
    STREAM_SENDING,
    STREAM_SENT,
    STREAM_READ_PENDING,
    STREAM_ACTIVE,
    STREAM_CLOSED,
    STREAM_UNUSED,
  };
  enum SendRequestState {
    SEND_STATE_NEXT_REQUEST,
    SEND_STATE_COMPLETE,
    SEND_STATE_NONE,
    SEND_STATE_UNUSABLE,
  };
  enum ReadHeadersState {
    READ_STATE_NEXT_HEADERS,
    READ_STATE_COMPLETE,
    READ_STATE_WAITING_FOR_CLOSE,
    READ_STATE_STREAM_CLOSED,
    READ_STATE_NONE,
    READ_STATE_UNUSABLE,
  };

  struct DeferredSendRequest {
    DeferredSendRequest();
    ~DeferredSendRequest();

    int pipeline_id;
    std::string request_line;
    HttpRequestHeaders headers;
    UploadDataStream* request_body;
    HttpResponseInfo* response;
    OldCompletionCallback* callback;
  };

  struct StreamInfo {
    StreamInfo();
    ~StreamInfo();

    linked_ptr<HttpStreamParser> parser;
    OldCompletionCallback* read_headers_callback;
    StreamState state;
  };

  typedef std::map<int, StreamInfo> StreamInfoMap;

  // Called after the first request is sent or in a task sometime after the
  // first stream is added to this pipeline. This gives the first request
  // priority to send, but doesn't hold up other requests if it doesn't.
  // When called the first time, notifies the |delegate_| that we can accept new
  // requests.
  void ActivatePipeline();

  // Responsible for sending one request at a time and waiting until each
  // comepletes.
  int DoSendRequestLoop(int result);

  // Called when an asynchronous Send() completes.
  void OnSendIOCallback(int result);

  // Sends the next deferred request. This may be called immediately after
  // SendRequest(), or it may be in a new task after a prior send completes in
  // DoSendComplete().
  int DoSendNextRequest(int result);

  // Notifies the user that the send has completed. This may be called directly
  // after SendRequest() for a synchronous request, or it may be called in
  // response to OnSendIOCallback for an asynchronous request.
  int DoSendComplete(int result);

  // Evicts all unsent deferred requests. This is called if there is a Send()
  // error or one of our streams informs us the connection is no longer
  // reusable.
  int DoEvictPendingSendRequests(int result);

  // Ensures that only the active request's HttpPipelinedSocket can read from
  // the underlying socket until it completes. A HttpPipelinedSocket informs us
  // that it's done by calling Close().
  int DoReadHeadersLoop(int result);

  // Called when the pending asynchronous ReadResponseHeaders() completes.
  void OnReadIOCallback(int result);

  // Determines if the next response in the pipeline is ready to be read.
  // If it's ready, then we call ReadResponseHeaders() on the underlying parser.
  // HttpPipelinedSocket indicates its readiness by calling
  // ReadResponseHeaders(). This function may be called immediately after
  // ReadResponseHeaders(), or it may be called in a new task after a previous
  // HttpPipelinedSocket finishes its work.
  int DoReadNextHeaders(int result);

  // Notifies the user that reading the headers has completed. This may happen
  // directly after DoReadNextHeaders() if the response is already available.
  // Otherwise, it is called in response to OnReadIOCallback().
  int DoReadHeadersComplete(int result);

  // This is a holding state. It does not do anything, except exit the
  // DoReadHeadersLoop(). It is called after DoReadHeadersComplete().
  int DoReadWaitingForClose(int result);

  // Cleans up the state associated with the active request. Invokes
  // DoReadNextHeaders() in a new task to start the next response. This is
  // called after the active request's HttpPipelinedSocket calls Close().
  int DoReadStreamClosed();

  // Removes all pending ReadResponseHeaders() requests from the queue. This may
  // happen if there is an error with the pipeline or one of our
  // HttpPipelinedSockets indicates the connection was suddenly closed.
  int DoEvictPendingReadHeaders(int result);

  // Invokes the user's callback in response to SendRequest() or
  // ReadResponseHeaders() completing on an underlying parser. This might be
  // invoked in response to our own IO callbacks, or it may be invoked if the
  // underlying parser completes SendRequest() or ReadResponseHeaders()
  // synchronously, but we've already returned ERR_IO_PENDING to the user's
  // SendRequest() or ReadResponseHeaders() call into us.
  void FireUserCallback(OldCompletionCallback* callback, int result);

  Delegate* delegate_;
  scoped_ptr<ClientSocketHandle> connection_;
  SSLConfig used_ssl_config_;
  ProxyInfo used_proxy_info_;
  BoundNetLog net_log_;
  bool was_npn_negotiated_;
  scoped_refptr<GrowableIOBuffer> read_buf_;
  int next_pipeline_id_;
  bool active_;
  bool usable_;
  bool completed_one_request_;
  ScopedRunnableMethodFactory<HttpPipelinedConnectionImpl> method_factory_;

  StreamInfoMap stream_info_map_;

  std::queue<int> request_order_;

  std::queue<DeferredSendRequest> deferred_request_queue_;
  SendRequestState send_next_state_;
  OldCompletionCallbackImpl<HttpPipelinedConnectionImpl> send_io_callback_;
  OldCompletionCallback* send_user_callback_;

  ReadHeadersState read_next_state_;
  OldCompletionCallbackImpl<HttpPipelinedConnectionImpl> read_io_callback_;
  OldCompletionCallback* read_user_callback_;

  DISALLOW_COPY_AND_ASSIGN(HttpPipelinedConnectionImpl);
};

}  // namespace net

#endif  // NET_HTTP_HTTP_PIPELINED_CONNECTION_IMPL_H_
