// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/page_transition_types.h"

#include "base/logging.h"

namespace content {

PageTransition PageTransitionStripQualifier(PageTransition type) {
  return static_cast<PageTransition>(type & ~PAGE_TRANSITION_QUALIFIER_MASK);
}

bool PageTransitionIsValidType(int32 type) {
  PageTransition t = PageTransitionStripQualifier(
      static_cast<PageTransition>(type));
  return (t <= PAGE_TRANSITION_LAST_CORE);
}

PageTransition PageTransitionFromInt(int32 type) {
  if (!PageTransitionIsValidType(type)) {
    NOTREACHED() << "Invalid transition type " << type;

    // Return a safe default so we don't have corrupt data in release mode.
    return PAGE_TRANSITION_LINK;
  }
  return static_cast<PageTransition>(type);
}

bool PageTransitionIsMainFrame(PageTransition type) {
  int32 t = PageTransitionStripQualifier(type);
  return (t != PAGE_TRANSITION_AUTO_SUBFRAME &&
          t != PAGE_TRANSITION_MANUAL_SUBFRAME);
}

bool PageTransitionIsRedirect(PageTransition type) {
  return (type & PAGE_TRANSITION_IS_REDIRECT_MASK) != 0;
}

int32 PageTransitionGetQualifier(PageTransition type) {
  return type & PAGE_TRANSITION_QUALIFIER_MASK;
}

const char* PageTransitionGetCoreTransitionString(PageTransition type) {
  switch (type & PAGE_TRANSITION_CORE_MASK) {
    case PAGE_TRANSITION_LINK: return "link";
    case PAGE_TRANSITION_TYPED: return "typed";
    case PAGE_TRANSITION_AUTO_BOOKMARK: return "auto_bookmark";
    case PAGE_TRANSITION_AUTO_SUBFRAME: return "auto_subframe";
    case PAGE_TRANSITION_MANUAL_SUBFRAME: return "manual_subframe";
    case PAGE_TRANSITION_GENERATED: return "generated";
    case PAGE_TRANSITION_START_PAGE: return "start_page";
    case PAGE_TRANSITION_FORM_SUBMIT: return "form_submit";
    case PAGE_TRANSITION_RELOAD: return "reload";
    case PAGE_TRANSITION_KEYWORD: return "keyword";
    case PAGE_TRANSITION_KEYWORD_GENERATED: return "keyword_generated";
  }
  return NULL;
}

}  // namespace content
