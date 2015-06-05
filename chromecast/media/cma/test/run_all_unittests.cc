// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "base/test/test_suite.h"
#include "build/build_config.h"
#include "chromecast/base/metrics/cast_metrics_test_helper.h"
#include "media/base/media.h"

#if defined(OS_ANDROID)
#error "CMA not supported on Android"
#endif

class CmaTestSuite : public base::TestSuite {
 public:
  // Note: the base class constructor creates an AtExitManager.
  CmaTestSuite(int argc, char** argv) : TestSuite(argc, argv) {}
  ~CmaTestSuite() override {}

 protected:
  void Initialize() override;
};

void CmaTestSuite::Initialize() {
  // Run TestSuite::Initialize first so that logging is initialized.
  base::TestSuite::Initialize();

  // Some of the chromecast media unit tests require a metrics helper instance.
  // Provide a fake one.
  chromecast::metrics::InitializeMetricsHelperForTesting();

  // Initialize the FFMpeg library.
  // Note: at this time, AtExitManager is already present.
  media::InitializeMediaLibraryForTesting();
}

int main(int argc, char** argv) {
  CmaTestSuite test_suite(argc, argv);

  return base::LaunchUnitTests(
      argc, argv, base::Bind(&CmaTestSuite::Run,
                             base::Unretained(&test_suite)));
}
