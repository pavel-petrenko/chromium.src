// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/enhanced_bookmarks/chrome_bookmark_server_cluster_service.h"

#include "chrome/browser/sync/profile_sync_service.h"
#include "components/enhanced_bookmarks/enhanced_bookmark_model.h"

namespace enhanced_bookmarks {

ChromeBookmarkServerClusterService::ChromeBookmarkServerClusterService(
    const std::string& application_language_code,
    scoped_refptr<net::URLRequestContextGetter> request_context_getter,
    ProfileOAuth2TokenService* token_service,
    SigninManagerBase* signin_manager,
    EnhancedBookmarkModel* enhanced_bookmark_model,
    PrefService* pref_service,
    ProfileSyncService* sync_service)
    : BookmarkServerClusterService(application_language_code,
                                   request_context_getter,
                                   token_service,
                                   signin_manager,
                                   enhanced_bookmark_model,
                                   pref_service),
      sync_service_(sync_service) {
  if (sync_service_)
    sync_service_->AddObserver(this);
}

ChromeBookmarkServerClusterService::~ChromeBookmarkServerClusterService() {
  if (sync_service_)
    sync_service_->RemoveObserver(this);
}

void ChromeBookmarkServerClusterService::AddObserver(
    BookmarkServerServiceObserver* observer) {
  BookmarkServerClusterService::AddObserver(observer);
  if (sync_refresh_skipped_) {
    sync_refresh_skipped_ = false;
    TriggerTokenRequest(false);
  }
}

void ChromeBookmarkServerClusterService::OnStateChanged() {
  // Do nothing.
}

void ChromeBookmarkServerClusterService::OnSyncCycleCompleted() {
  // The stars cluster API relies on the information in chrome-sync. Sending a
  // cluster request immediately after a bookmark is changed from the bookmark
  // observer notification will yield the wrong results. The request must be
  // delayed until the sync cycle has completed. In fact, the ideal signal would
  // be "bookmark changed by sync", but we don't have that yet, and this is a
  // compromise.
  // Note that we will be skipping calling this cluster API if there is no
  // observer attached, because calling that is meaningless without UI to show.
  if (model_->loaded()) {
    if (observers_.might_have_observers()) {
      TriggerTokenRequest(false);
      sync_refresh_skipped_ = false;
    } else {
      sync_refresh_skipped_ = true;
    }
  }
}

}  // namespace enhanced_bookmarks
