// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/perf/perf_ui_test_suite.h"

int main(int argc, char **argv) {
  return PerfUITestSuite(argc, argv).Run();
}

