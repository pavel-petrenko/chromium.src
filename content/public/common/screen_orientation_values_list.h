// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_SCREEN_ORIENTATION_VALUES_LIST_H_
#define CONTENT_PUBLIC_COMMON_SCREEN_ORIENTATION_VALUES_LIST_H_

#ifndef DEFINE_SCREEN_ORIENTATION_VALUE
#error "DEFINE_SCREEN_ORIENTATION_VALUE should be defined before including this"
#endif

// These values are defined with macros so that a Java class can be generated
// for them.
DEFINE_SCREEN_ORIENTATION_VALUE(PORTRAIT_PRIMARY, 1 << 0)
DEFINE_SCREEN_ORIENTATION_VALUE(LANDSCAPE_PRIMARY, 1 << 1)
DEFINE_SCREEN_ORIENTATION_VALUE(PORTRAIT_SECONDARY, 1 << 2)
DEFINE_SCREEN_ORIENTATION_VALUE(LANDSCAPE_SECONDARY, 1 << 3)

#endif  // CONTENT_PUBLIC_COMMON_SCREEN_ORIENTATION_VALUES_LIST_H_
