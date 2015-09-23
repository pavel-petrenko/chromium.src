// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_REGISTRY_H_
#define COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_REGISTRY_H_

#include <string>

#include "base/containers/scoped_ptr_map.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "components/content_settings/core/browser/content_settings_info.h"
#include "components/content_settings/core/browser/website_settings_info.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"

namespace content_settings {

class WebsiteSettingsRegistry;

// This class stores ContentSettingsInfo objects for each content setting in the
// system and provides access to them. Global instances can be fetched and
// methods called from from any thread because all of its public methods are
// const.
class ContentSettingsRegistry {
 public:
  static ContentSettingsRegistry* GetInstance();

  // Reset the instance for use inside tests.
  void ResetForTest();

  const ContentSettingsInfo* Get(ContentSettingsType type) const;

 private:
  friend class ContentSettingsRegistryTest;
  friend struct base::DefaultLazyInstanceTraits<ContentSettingsRegistry>;

  ContentSettingsRegistry();
  ContentSettingsRegistry(WebsiteSettingsRegistry* website_settings_registry);
  ~ContentSettingsRegistry();

  void Init();

  // Register a new content setting. This maps an origin to an ALLOW/ASK/BLOCK
  // value (see the ContentSetting enum).
  void Register(ContentSettingsType type,
                const std::string& name,
                ContentSetting initial_default_value,
                WebsiteSettingsInfo::SyncStatus sync_status,
                const std::vector<std::string>& whitelisted_schemes);

  base::ScopedPtrMap<ContentSettingsType, scoped_ptr<ContentSettingsInfo>>
      content_settings_info_;
  WebsiteSettingsRegistry* website_settings_registry_;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingsRegistry);
};

}  // namespace content_settings

#endif  // COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_REGISTRY_H_
