// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile_shortcut_manager.h"

// static
bool ProfileShortcutManager::IsFeatureEnabled() {
  return false;
}

// static
ProfileShortcutManager* ProfileShortcutManager::Create(
    ProfileInfoCache& cache) {
  return NULL;
}
