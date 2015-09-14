// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/web_memory_dump_provider_adapter.h"

#include "content/child/web_process_memory_dump_impl.h"
#include "third_party/WebKit/public/platform/WebMemoryDumpProvider.h"

namespace content {

WebMemoryDumpProviderAdapter::WebMemoryDumpProviderAdapter(
    blink::WebMemoryDumpProvider* wmdp)
    : web_memory_dump_provider_(wmdp), is_registered_(false) {
}

WebMemoryDumpProviderAdapter::~WebMemoryDumpProviderAdapter() {
  DCHECK(!is_registered_);
}

bool WebMemoryDumpProviderAdapter::OnMemoryDump(
    const base::trace_event::MemoryDumpArgs& args,
    base::trace_event::ProcessMemoryDump* pmd) {
  blink::WebMemoryDumpLevelOfDetail level;
  switch (args.level_of_detail) {
    case base::trace_event::MemoryDumpLevelOfDetail::LIGHT:
      // TODO(primiano): switch to actual constant once the corresponding
      // rename lands in blink and rolls.
      level = static_cast<blink::WebMemoryDumpLevelOfDetail>(0);
      break;
    case base::trace_event::MemoryDumpLevelOfDetail::DETAILED:
      level = static_cast<blink::WebMemoryDumpLevelOfDetail>(1);
      break;
    default:
      NOTREACHED();
      return false;
  }
  WebProcessMemoryDumpImpl web_pmd_impl(pmd);

  return web_memory_dump_provider_->onMemoryDump(level, &web_pmd_impl);
}

}  // namespace content
