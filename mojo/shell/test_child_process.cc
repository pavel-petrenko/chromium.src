// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/shell/test_child_process.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"

namespace mojo {
namespace shell {

namespace {

void TrivialPostedTask() {
  VLOG(2) << "TrivialPostedTask()";
}

}  // namespace

TestChildProcess::TestChildProcess() {
}

TestChildProcess::~TestChildProcess() {
}

void TestChildProcess::Main() {
  VLOG(2) << "TestChildProcess::Main()";

  base::MessageLoop::current()->PostTask(FROM_HERE,
                                         base::Bind(&TrivialPostedTask));
  base::MessageLoop::current()->QuitWhenIdle();
}

}  // namespace shell
}  // namespace mojo
