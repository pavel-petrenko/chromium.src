// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gin/shell_runner.h"

#include "base/compiler_specific.h"
#include "base/message_loop/message_loop.h"
#include "gin/array_buffer.h"
#include "gin/converter.h"
#include "gin/public/isolate_holder.h"
#include "gin/v8_initializer.h"
#include "testing/gtest/include/gtest/gtest.h"

#ifdef V8_USE_EXTERNAL_STARTUP_DATA
#include "gin/public/isolate_holder.h"
#endif

using v8::Isolate;
using v8::Object;
using v8::Script;
using v8::String;

namespace gin {

TEST(RunnerTest, Run) {
  base::MessageLoop message_loop;
  std::string source = "this.result = 'PASS';\n";

#ifdef V8_USE_EXTERNAL_STARTUP_DATA
  gin::V8Initializer::LoadV8Snapshot();
#endif

  gin::IsolateHolder::Initialize(gin::IsolateHolder::kStrictMode,
                                 gin::ArrayBufferAllocator::SharedInstance());
  gin::IsolateHolder instance;

  ShellRunnerDelegate delegate;
  Isolate* isolate = instance.isolate();
  ShellRunner runner(&delegate, isolate);
  Runner::Scope scope(&runner);
  runner.Run(source, "test_data.js");

  std::string result;
  EXPECT_TRUE(Converter<std::string>::FromV8(isolate,
      runner.global()->Get(StringToV8(isolate, "result")),
      &result));
  EXPECT_EQ("PASS", result);
}

}  // namespace gin
