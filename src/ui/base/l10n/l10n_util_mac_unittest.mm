// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Foundation/Foundation.h>

#include "base/sys_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "ui/base/l10n/l10n_util_mac.h"

typedef PlatformTest L10nUtilMacTest;

TEST_F(L10nUtilMacTest, FixUpWindowsStyleLabel) {
  struct TestData {
    NSString* input;
    NSString* output;
  };

  TestData data[] = {
    { @"", @"" },
    { @"nothing", @"nothing" },
    { @"foo &bar", @"foo bar" },
    { @"foo &&bar", @"foo &bar" },
    { @"foo &&&bar", @"foo &bar" },
    { @"&foo &&bar", @"foo &bar" },
    { @"&foo &bar", @"foo bar" },
    { @"foo bar.", @"foo bar." },
    { @"foo bar..", @"foo bar.." },
    { @"foo bar...", @"foo bar\u2026" },
    { @"foo.bar", @"foo.bar" },
    { @"foo..bar", @"foo..bar" },
    { @"foo...bar", @"foo\u2026bar" },
    { @"foo...bar...", @"foo\u2026bar\u2026" },
    { @"foo(&b)", @"foo" },
    { @"foo(&b)...", @"foo\u2026" },
    { @"(&b)foo", @"foo" },
  };
  for (size_t idx = 0; idx < ARRAYSIZE_UNSAFE(data); ++idx) {
    string16 input16(base::SysNSStringToUTF16(data[idx].input));

    NSString* result = l10n_util::FixUpWindowsStyleLabel(input16);
    EXPECT_TRUE(result != nil) << "Fixup Failed, idx = " << idx;

    EXPECT_TRUE([data[idx].output isEqual:result])
        << "For idx " << idx << ", expected '" << [data[idx].output UTF8String]
        << "', got '" << [result UTF8String] << "'";
  }
}
