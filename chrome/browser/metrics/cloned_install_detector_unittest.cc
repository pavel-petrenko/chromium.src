// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/cloned_install_detector.h"

#include "base/prefs/testing_pref_service.h"
#include "chrome/browser/metrics/machine_id_provider.h"
#include "chrome/common/pref_names.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace metrics {

namespace {

const std::string kTestRawId = "test";
// Hashed machine id for |kTestRawId|.
const int kTestHashedId = 2216819;

}  // namespace

// TODO(jwd): Change this test to test the full flow and histogram outputs. It
// should also remove the need to make the test a friend of
// ClonedInstallDetector.
TEST(ClonedInstallDetectorTest, SaveId) {
  TestingPrefServiceSimple prefs;
  ClonedInstallDetector::RegisterPrefs(prefs.registry());

  scoped_ptr<ClonedInstallDetector> detector(
      new ClonedInstallDetector(MachineIdProvider::CreateInstance()));

  detector->SaveMachineId(&prefs, kTestRawId);

  EXPECT_EQ(kTestHashedId, prefs.GetInteger(prefs::kMetricsMachineId));
}

}  // namespace metrics
