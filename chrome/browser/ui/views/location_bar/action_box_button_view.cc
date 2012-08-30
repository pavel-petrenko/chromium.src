// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/action_box_button_view.h"

#include "base/command_line.h"
#include "base/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/command_updater.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_system.h"
#include "chrome/browser/ui/toolbar/action_box_menu_model.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/action_box_menu.h"
#include "chrome/browser/ui/views/browser_dialogs.h"
#include "chrome/common/chrome_switches.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"
#include "ui/base/accessibility/accessible_view_state.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

// Colors used for button backgrounds.
const SkColor kNormalBackgroundColor = SkColorSetRGB(255, 255, 255);
const SkColor kHotBackgroundColor = SkColorSetRGB(239, 239, 239);
const SkColor kPushedBackgroundColor = SkColorSetRGB(207, 207, 207);

const SkColor kNormalBorderColor = SkColorSetRGB(255, 255, 255);
const SkColor kHotBorderColor = SkColorSetRGB(223, 223, 223);
const SkColor kPushedBorderColor = SkColorSetRGB(191, 191, 191);

}  // namespace


ActionBoxButtonView::ActionBoxButtonView(Browser* browser, Profile* profile)
    : views::MenuButton(NULL, string16(), this, false),
      browser_(browser),
      profile_(profile) {
  set_id(VIEW_ID_ACTION_BOX_BUTTON);
  SetTooltipText(l10n_util::GetStringUTF16(IDS_TOOLTIP_ACTION_BOX_BUTTON));
  SetIcon(*ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
      IDR_ACTION_BOX_BUTTON));
  set_accessibility_focusable(true);
  set_border(NULL);
}

ActionBoxButtonView::~ActionBoxButtonView() {
}

SkColor ActionBoxButtonView::GetBackgroundColor() {
  switch (state()) {
    case BS_PUSHED:
      return kPushedBackgroundColor;
    case BS_HOT:
      return kHotBackgroundColor;
    default:
      return kNormalBackgroundColor;
  }
}

SkColor ActionBoxButtonView::GetBorderColor() {
  switch (state()) {
    case BS_PUSHED:
      return kPushedBorderColor;
    case BS_HOT:
      return kHotBorderColor;
    default:
      return kNormalBorderColor;
  }
}

bool ActionBoxButtonView::IsActionBoxEnabled() {
  return CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableActionBox);
}

void ActionBoxButtonView::GetAccessibleState(ui::AccessibleViewState* state) {
  MenuButton::GetAccessibleState(state);
  state->name = l10n_util::GetStringUTF16(IDS_ACCNAME_ACTION_BOX_BUTTON);
}

void ActionBoxButtonView::OnMenuButtonClicked(View* source,
                                              const gfx::Point& point) {
  ExtensionService* extension_service =
      extensions::ExtensionSystem::Get(profile_)->extension_service();

  ActionBoxMenuModel model(browser_, extension_service);
  ActionBoxMenu action_box_menu(browser_, &model);
  action_box_menu.Init();
  action_box_menu.RunMenu(this);
}
