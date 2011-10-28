// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_CONTENT_RESTRICTION_H_
#define CONTENT_COMMON_CONTENT_RESTRICTION_H_
#pragma once

// Used by a full-page plugin to disable browser commands because of
// restrictions on how the data is to be used (i.e. can't copy/print).
enum ContentRestriction {
  CONTENT_RESTRICTION_COPY  = 1 << 0,
  CONTENT_RESTRICTION_CUT   = 1 << 1,
  CONTENT_RESTRICTION_PASTE = 1 << 2,
  CONTENT_RESTRICTION_PRINT = 1 << 3,
  CONTENT_RESTRICTION_SAVE  = 1 << 4
};

#endif  // CONTENT_COMMON_CONTENT_RESTRICTION_H_
