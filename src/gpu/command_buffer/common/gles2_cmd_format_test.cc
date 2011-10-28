// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains unit tests for gles2 commmands

#include "testing/gtest/include/gtest/gtest.h"
#include "gpu/command_buffer/common/gles2_cmd_format.h"

namespace gpu {
namespace gles2 {

class GLES2FormatTest : public testing::Test {
 protected:
  static const unsigned char kInitialValue = 0xBD;

  virtual void SetUp() {
    memset(buffer_, kInitialValue, sizeof(buffer_));
  }

  virtual void TearDown() {
  }

  template <typename T>
  T* GetBufferAs() {
    return static_cast<T*>(static_cast<void*>(&buffer_));
  }

  void CheckBytesWritten(
      const void* end, size_t expected_size, size_t written_size) {
    size_t actual_size = static_cast<const unsigned char*>(end) -
        GetBufferAs<const unsigned char>();
    EXPECT_LT(actual_size, sizeof(buffer_));
    EXPECT_GT(actual_size, 0u);
    EXPECT_EQ(expected_size, actual_size);
    EXPECT_EQ(kInitialValue, buffer_[written_size]);
    EXPECT_NE(kInitialValue, buffer_[written_size - 1]);
  }

  void CheckBytesWrittenMatchesExpectedSize(
      const void* end, size_t expected_size) {
    CheckBytesWritten(end, expected_size, expected_size);
  }

 private:
  unsigned char buffer_[1024];
};

// GCC requires these declarations, but MSVC requires they not be present
#ifndef _MSC_VER
const unsigned char GLES2FormatTest::kInitialValue;
#endif

#include "gpu/command_buffer/common/gles2_cmd_format_test_autogen.h"

}  // namespace gles2
}  // namespace gpu

