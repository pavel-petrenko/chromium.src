// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "views/controls/combobox/native_combobox_views.h"

namespace views {

// static
NativeComboboxWrapper* NativeComboboxWrapper::CreateWrapper(
    Combobox* combobox) {
  return new NativeComboboxViews(combobox);
}

}  // namespace views
