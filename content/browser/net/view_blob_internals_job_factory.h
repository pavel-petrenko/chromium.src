// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_NET_VIEW_BLOB_INTERNALS_JOB_FACTORY_H_
#define CONTENT_BROWSER_NET_VIEW_BLOB_INTERNALS_JOB_FACTORY_H_

namespace net {
class NetworkDelegate;
class URLRequest;
class URLRequestJob;
}  // namespace net
namespace webkit_blob {
class BlobStorageController;
}  // webkit_blob

class GURL;

class ViewBlobInternalsJobFactory {
 public:
  static bool IsSupportedURL(const GURL& url);
  static net::URLRequestJob* CreateJobForRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate,
      webkit_blob::BlobStorageController* blob_storage_controller);
};

#endif  // CONTENT_BROWSER_NET_VIEW_BLOB_INTERNALS_JOB_FACTORY_H_
