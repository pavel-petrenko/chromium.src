// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/logging.h"
#include "base/port.h"
#include "base/test/perf_test_suite.h"
#include "media/formats/mp2t/timestamp_unroller.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {
namespace mp2t {

static std::vector<int64> TruncateTimestamps(
    const std::vector<int64>& timestamps) {
  const int nbits = 33;
  int64 truncate_mask = (GG_INT64_C(1) << nbits) - 1;
  std::vector<int64> truncated_timestamps(timestamps.size());
  for (size_t k = 0; k < timestamps.size(); k++)
    truncated_timestamps[k] = timestamps[k] & truncate_mask;
  return truncated_timestamps;
}

static void RunUnrollTest(const std::vector<int64>& timestamps) {
  std::vector<int64> truncated_timestamps = TruncateTimestamps(timestamps);

  TimestampUnroller timestamp_unroller;
  for (size_t k = 0; k < timestamps.size(); k++) {
    int64 unrolled_timestamp =
        timestamp_unroller.GetUnrolledTimestamp(truncated_timestamps[k]);
    EXPECT_EQ(timestamps[k], unrolled_timestamp);
  }
}

TEST(TimestampUnrollerTest, SingleStream) {
  // Array of 64 bit timestamps.
  // This is the expected result from unrolling these timestamps
  // truncated to 33 bits.
  int64 timestamps[] = {
    GG_INT64_C(0x0000000000000000),
    GG_INT64_C(-190),                // - 190
    GG_INT64_C(0x00000000aaaaa9ed),  // + 0xaaaaaaab
    GG_INT64_C(0x0000000155555498),  // + 0xaaaaaaab
    GG_INT64_C(0x00000001ffffff43),  // + 0xaaaaaaab
    GG_INT64_C(0x00000002aaaaa9ee),  // + 0xaaaaaaab
    GG_INT64_C(0x0000000355555499),  // + 0xaaaaaaab
    GG_INT64_C(0x00000003ffffff44),  // + 0xaaaaaaab
  };

  std::vector<int64> timestamps_vector(
      timestamps, timestamps + arraysize(timestamps));
  RunUnrollTest(timestamps_vector);
}

}  // namespace mp2t
}  // namespace media
