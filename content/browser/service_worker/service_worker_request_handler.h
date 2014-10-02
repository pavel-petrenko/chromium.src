// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_REQUEST_HANDLER_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_REQUEST_HANDLER_H_

#include "base/basictypes.h"
#include "base/memory/weak_ptr.h"
#include "base/supports_user_data.h"
#include "base/time/time.h"
#include "content/common/content_export.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/public/common/resource_type.h"
#include "net/url_request/url_request_job_factory.h"

namespace net {
class NetworkDelegate;
class URLRequest;
class URLRequestInterceptor;
}

namespace storage {
class BlobStorageContext;
}

namespace content {

class ResourceRequestBody;
class ServiceWorkerContextCore;
class ServiceWorkerContextWrapper;
class ServiceWorkerProviderHost;

// Abstract base class for routing network requests to ServiceWorkers.
// Created one per URLRequest and attached to each request.
class CONTENT_EXPORT ServiceWorkerRequestHandler
    : public base::SupportsUserData::Data {
 public:
  // Attaches a newly created handler if the given |request| needs to
  // be handled by ServiceWorker.
  // TODO(kinuko): While utilizing UserData to attach data to URLRequest
  // has some precedence, it might be better to attach this handler in a more
  // explicit way within content layer, e.g. have ResourceRequestInfoImpl
  // own it.
  static void InitializeHandler(
      net::URLRequest* request,
      ServiceWorkerContextWrapper* context_wrapper,
      storage::BlobStorageContext* blob_storage_context,
      int process_id,
      int provider_id,
      bool skip_service_worker,
      FetchRequestMode request_mode,
      FetchCredentialsMode credentials_mode,
      ResourceType resource_type,
      scoped_refptr<ResourceRequestBody> body);

  // Returns the handler attached to |request|. This may return NULL
  // if no handler is attached.
  static ServiceWorkerRequestHandler* GetHandler(
      net::URLRequest* request);

  // Creates a protocol interceptor for ServiceWorker.
  static scoped_ptr<net::URLRequestInterceptor> CreateInterceptor();

  virtual ~ServiceWorkerRequestHandler();

  // Called via custom URLRequestJobFactory.
  virtual net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) = 0;

  virtual void GetExtraResponseInfo(
      bool* was_fetched_via_service_worker,
      bool* was_fallback_required_by_service_worker,
      GURL* original_url_via_service_worker,
      base::TimeTicks* fetch_start_time,
      base::TimeTicks* fetch_ready_time,
      base::TimeTicks* fetch_end_time) const = 0;

 protected:
  ServiceWorkerRequestHandler(
      base::WeakPtr<ServiceWorkerContextCore> context,
      base::WeakPtr<ServiceWorkerProviderHost> provider_host,
      base::WeakPtr<storage::BlobStorageContext> blob_storage_context,
      ResourceType resource_type);

  base::WeakPtr<ServiceWorkerContextCore> context_;
  base::WeakPtr<ServiceWorkerProviderHost> provider_host_;
  base::WeakPtr<storage::BlobStorageContext> blob_storage_context_;
  ResourceType resource_type_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerRequestHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_REQUEST_HANDLER_H_
