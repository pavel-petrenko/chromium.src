// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/renderer/render_process_observer.h"

namespace content {

bool RenderProcessObserver::OnControlMessageReceived(
    const IPC::Message& message) {
  return false;
}

}  // namespace content
