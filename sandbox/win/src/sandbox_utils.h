// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SANDBOX_SRC_SANDBOX_UTILS_H_
#define SANDBOX_SRC_SANDBOX_UTILS_H_

#include <windows.h>
#include <string>

#include "base/basictypes.h"
#include "base/strings/string16.h"
#include "sandbox/win/src/nt_internals.h"

namespace sandbox {

// Returns true if the current OS is Windows XP SP2 or later.
bool IsXPSP2OrLater();

void InitObjectAttribs(const base::string16& name,
                       ULONG attributes,
                       HANDLE root,
                       OBJECT_ATTRIBUTES* obj_attr,
                       UNICODE_STRING* uni_name);

}  // namespace sandbox

#endif  // SANDBOX_SRC_SANDBOX_UTILS_H_
