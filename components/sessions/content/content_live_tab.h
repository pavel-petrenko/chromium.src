// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SESSIONS_CONTENT_CONTENT_LIVE_TAB_H_
#define COMPONENTS_SESSIONS_CONTENT_CONTENT_LIVE_TAB_H_

#include "base/basictypes.h"
#include "base/supports_user_data.h"
#include "components/sessions/content/content_serialized_navigation_builder.h"
#include "components/sessions/core/live_tab.h"
#include "components/sessions/sessions_export.h"
#include "content/public/browser/web_contents.h"

namespace content {
class NavigationController;
class NavigationEntry;
class SessionStorageNamespace;
}

class PersistentTabRestoreServiceTest;

namespace sessions {

// An implementation of LiveTab that is backed by content::WebContents for use
// on //content-based platforms.
// Implementation note: This class can't be a WebContentsUserData due to that
// class being unusable in the component build. crbug.com/532866
class SESSIONS_EXPORT ContentLiveTab
    : public LiveTab,
      public base::SupportsUserData::Data {
 public:
  ~ContentLiveTab() override;

  static void CreateForWebContents(content::WebContents* web_contents);
  static ContentLiveTab* FromWebContents(content::WebContents* web_contents);

  // LiveTab:
  int GetCurrentEntryIndex() override;
  int GetPendingEntryIndex() override;
  sessions::SerializedNavigationEntry GetEntryAtIndex(int index) override;
  sessions::SerializedNavigationEntry GetPendingEntry() override;
  int GetEntryCount() override;
  void LoadIfNecessary() override;
  const std::string& GetUserAgentOverride() const override;

  content::WebContents* web_contents() { return web_contents_; }
  const content::WebContents* web_contents() const { return web_contents_; }

 private:
  friend class base::SupportsUserData;
  friend class ::PersistentTabRestoreServiceTest;

  explicit ContentLiveTab(content::WebContents* contents);

  content::NavigationController& navigation_controller() {
    return web_contents_->GetController();
  }

  content::WebContents* web_contents_;

  DISALLOW_COPY_AND_ASSIGN(ContentLiveTab);
};

}  // namespace sessions

#endif  // COMPONENTS_SESSIONS_CONTENT_CONTENT_LIVE_TAB_H_
