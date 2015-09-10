// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/global_error/global_error.h"

#include "base/logging.h"
#include "chrome/browser/ui/global_error/global_error_bubble_view_base.h"
#include "grit/theme_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"

#if !defined(OS_MACOSX) && !defined(OS_ANDROID)
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/vector_icons_public.h"
#include "ui/native_theme/common_theme.h"
#include "ui/native_theme/native_theme.h"
#endif

// GlobalError ---------------------------------------------------------------

GlobalError::GlobalError() {}

GlobalError::~GlobalError() {}

GlobalError::Severity GlobalError::GetSeverity() { return SEVERITY_MEDIUM; }

gfx::Image GlobalError::MenuItemIcon() {
#if defined(OS_MACOSX) || defined(OS_ANDROID)
  return ResourceBundle::GetSharedInstance().GetNativeImageNamed(
      IDR_INPUT_ALERT_MENU);
#else
  SkColor icon_color;
  ui::CommonThemeGetSystemColor(ui::NativeTheme::kColorId_Amber, &icon_color);
  return gfx::Image(
      gfx::CreateVectorIcon(gfx::VectorIconId::WARNING, 18, icon_color));
#endif
}

// GlobalErrorWithStandardBubble ---------------------------------------------

GlobalErrorWithStandardBubble::GlobalErrorWithStandardBubble()
    : has_shown_bubble_view_(false), bubble_view_(NULL) {}

GlobalErrorWithStandardBubble::~GlobalErrorWithStandardBubble() {}

bool GlobalErrorWithStandardBubble::HasBubbleView() { return true; }

bool GlobalErrorWithStandardBubble::HasShownBubbleView() {
  return has_shown_bubble_view_;
}

void GlobalErrorWithStandardBubble::ShowBubbleView(Browser* browser) {
  has_shown_bubble_view_ = true;
  bubble_view_ =
      GlobalErrorBubbleViewBase::ShowStandardBubbleView(browser, AsWeakPtr());
}

GlobalErrorBubbleViewBase* GlobalErrorWithStandardBubble::GetBubbleView() {
  return bubble_view_;
}

bool GlobalErrorWithStandardBubble::ShouldCloseOnDeactivate() const {
  return true;
}

gfx::Image GlobalErrorWithStandardBubble::GetBubbleViewIcon() {
  // If you change this make sure to also change the menu icon and the wrench
  // icon color.
  return ResourceBundle::GetSharedInstance().GetNativeImageNamed(
      IDR_INPUT_ALERT);
}

bool GlobalErrorWithStandardBubble::ShouldShowCloseButton() const {
  return false;
}

bool GlobalErrorWithStandardBubble::ShouldAddElevationIconToAcceptButton() {
  return false;
}

void GlobalErrorWithStandardBubble::BubbleViewDidClose(Browser* browser) {
  DCHECK(browser);
  bubble_view_ = NULL;
  OnBubbleViewDidClose(browser);
}
