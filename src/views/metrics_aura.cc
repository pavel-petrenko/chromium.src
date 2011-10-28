// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "views/metrics.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

namespace views {

int GetDoubleClickInterval() {
#if defined(OS_WIN)
  return ::GetDoubleClickTime();
#else
  return 5;
#endif
}

int GetMenuShowDelay() {
#if defined(OS_WIN)
  static DWORD delay = 0;
  if (!delay && !SystemParametersInfo(SPI_GETMENUSHOWDELAY, 0, &delay, 0))
    delay = kDefaultMenuShowDelay;
  return delay;
#else
  return 0;
#endif
}

}  // namespace views
