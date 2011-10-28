// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// This class simulates a slow download.  This used in a UI test to test the
// download manager.  Requests to |kUnknownSizeUrl| and |kKnownSizeUrl| start
// downloads that pause after the first N bytes, to be completed by sending a
// request to |kFinishDownloadUrl|.

#ifndef CONTENT_BROWSER_NET_URL_REQUEST_SLOW_DOWNLOAD_JOB_H_
#define CONTENT_BROWSER_NET_URL_REQUEST_SLOW_DOWNLOAD_JOB_H_
#pragma once

#include <set>
#include <string>

#include "base/task.h"
#include "content/common/content_export.h"
#include "net/url_request/url_request_job.h"

class URLRequestSlowDownloadJob : public net::URLRequestJob {
 public:
  // Test URLs.
  CONTENT_EXPORT static const char kUnknownSizeUrl[];
  CONTENT_EXPORT static const char kKnownSizeUrl[];
  CONTENT_EXPORT static const char kFinishDownloadUrl[];

  // Download sizes.
  CONTENT_EXPORT static const int kFirstDownloadSize;
  CONTENT_EXPORT static const int kSecondDownloadSize;

  // Timer callback, used to check to see if we should finish our download and
  // send the second chunk.
  void CheckDoneStatus();

  // net::URLRequestJob methods
  virtual void Start();
  virtual bool GetMimeType(std::string* mime_type) const;
  virtual void GetResponseInfo(net::HttpResponseInfo* info);
  virtual bool ReadRawData(net::IOBuffer* buf, int buf_size, int *bytes_read);

  static net::URLRequestJob* Factory(net::URLRequest* request,
                                     const std::string& scheme);

  // Returns the current number of URLRequestSlowDownloadJobs that have
  // not yet completed.
  CONTENT_EXPORT static size_t NumberOutstandingRequests();

  // Adds the testing URLs to the net::URLRequestFilter.
  CONTENT_EXPORT static void AddUrlHandler();

 private:
  explicit URLRequestSlowDownloadJob(net::URLRequest* request);
  virtual ~URLRequestSlowDownloadJob();

  void GetResponseInfoConst(net::HttpResponseInfo* info) const;

  // Mark all pending requests to be finished.  We keep track of pending
  // requests in |pending_requests_|.
  static void FinishPendingRequests();
  static std::set<URLRequestSlowDownloadJob*> pending_requests_;

  void StartAsync();

  void set_should_finish_download() { should_finish_download_ = true; }

  int first_download_size_remaining_;
  bool should_finish_download_;
  scoped_refptr<net::IOBuffer> buffer_;
  int buffer_size_;

  ScopedRunnableMethodFactory<URLRequestSlowDownloadJob> method_factory_;
};

#endif  // CONTENT_BROWSER_NET_URL_REQUEST_SLOW_DOWNLOAD_JOB_H_
