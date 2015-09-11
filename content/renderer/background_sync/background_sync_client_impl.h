// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_BACKGROUND_SYNC_BACKGROUND_SYNC_CLIENT_IMPL_H_
#define CONTENT_RENDERER_BACKGROUND_SYNC_BACKGROUND_SYNC_CLIENT_IMPL_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/common/background_sync_service.mojom.h"
#include "content/common/content_export.h"
#include "third_party/mojo/src/mojo/public/cpp/bindings/strong_binding.h"

namespace content {

class CONTENT_EXPORT BackgroundSyncClientImpl
    : public NON_EXPORTED_BASE(BackgroundSyncServiceClient) {
 public:
  static void Create(
      int64_t service_worker_registration_id,
      mojo::InterfaceRequest<BackgroundSyncServiceClient> request);

  ~BackgroundSyncClientImpl() override;

 private:
  using SyncCallback = mojo::Callback<void(ServiceWorkerEventStatus)>;
  BackgroundSyncClientImpl(
      int64_t service_worker_registration_id,
      mojo::InterfaceRequest<BackgroundSyncServiceClient> request);

  // BackgroundSyncServiceClient methods:
  void Sync(int handle_id, const SyncCallback& callback) override;
  void SyncDidGetRegistration(const SyncCallback& callback,
                              BackgroundSyncError error,
                              const SyncRegistrationPtr& registration);

  int64_t service_worker_registration_id_;
  mojo::StrongBinding<BackgroundSyncServiceClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundSyncClientImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_BACKGROUND_SYNC_BACKGROUND_SYNC_CLIENT_IMPL_H_
