// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_STORAGE_DURABLE_STORAGE_PERMISSION_CONTEXT_H_
#define CHROME_BROWSER_STORAGE_DURABLE_STORAGE_PERMISSION_CONTEXT_H_

#include "chrome/browser/permissions/permission_context_base.h"

class DurableStoragePermissionContext : public PermissionContextBase {
 public:
  explicit DurableStoragePermissionContext(Profile* profile);
  ~DurableStoragePermissionContext() override = default;

  bool IsRestrictedToSecureOrigins() const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DurableStoragePermissionContext);
};

#endif  // CHROME_BROWSER_STORAGE_DURABLE_STORAGE_PERMISSION_CONTEXT_H_
