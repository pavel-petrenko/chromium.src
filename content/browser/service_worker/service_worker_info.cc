// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_info.h"

#include "content/common/service_worker/service_worker_types.h"
#include "ipc/ipc_message.h"

namespace content {

ServiceWorkerVersionInfo::ServiceWorkerVersionInfo()
    : running_status(ServiceWorkerVersion::STOPPED),
      status(ServiceWorkerVersion::NEW),
      version_id(kInvalidServiceWorkerVersionId),
      process_id(-1),
      thread_id(-1),
      devtools_agent_route_id(MSG_ROUTING_NONE) {
}

ServiceWorkerVersionInfo::ServiceWorkerVersionInfo(
    ServiceWorkerVersion::RunningStatus running_status,
    ServiceWorkerVersion::Status status,
    const GURL& script_url,
    int64 version_id,
    int process_id,
    int thread_id,
    int devtools_agent_route_id)
    : running_status(running_status),
      status(status),
      script_url(script_url),
      version_id(version_id),
      process_id(process_id),
      thread_id(thread_id),
      devtools_agent_route_id(devtools_agent_route_id) {
}

ServiceWorkerVersionInfo::~ServiceWorkerVersionInfo() {}

ServiceWorkerRegistrationInfo::ServiceWorkerRegistrationInfo() {}

ServiceWorkerRegistrationInfo::ServiceWorkerRegistrationInfo(
    const GURL& pattern,
    int64 registration_id,
    const ServiceWorkerVersionInfo& active_version,
    const ServiceWorkerVersionInfo& waiting_version,
    const ServiceWorkerVersionInfo& installing_version)
    : pattern(pattern),
      registration_id(registration_id),
      active_version(active_version),
      waiting_version(waiting_version),
      installing_version(installing_version) {
}

ServiceWorkerRegistrationInfo::~ServiceWorkerRegistrationInfo() {}

}  // namespace content
