// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "views/controls/menu/menu_host_root_view.h"

#include "views/controls/menu/menu_controller.h"
#include "views/controls/menu/submenu_view.h"

namespace views {

MenuHostRootView::MenuHostRootView(Widget* widget,
                                   SubmenuView* submenu)
    : internal::RootView(widget),
      submenu_(submenu),
      forward_drag_to_menu_controller_(true) {
}

bool MenuHostRootView::OnMousePressed(const MouseEvent& event) {
  forward_drag_to_menu_controller_ =
      !GetLocalBounds().Contains(event.location()) ||
      !RootView::OnMousePressed(event);
  if (forward_drag_to_menu_controller_ && GetMenuController())
    GetMenuController()->OnMousePressed(submenu_, event);
  return true;
}

bool MenuHostRootView::OnMouseDragged(const MouseEvent& event) {
  if (forward_drag_to_menu_controller_ && GetMenuController()) {
    GetMenuController()->OnMouseDragged(submenu_, event);
    return true;
  }
  return RootView::OnMouseDragged(event);
}

void MenuHostRootView::OnMouseReleased(const MouseEvent& event) {
  RootView::OnMouseReleased(event);
  if (forward_drag_to_menu_controller_ && GetMenuController()) {
    forward_drag_to_menu_controller_ = false;
    GetMenuController()->OnMouseReleased(submenu_, event);
  }
}

void MenuHostRootView::OnMouseMoved(const MouseEvent& event) {
  RootView::OnMouseMoved(event);
  if (GetMenuController())
    GetMenuController()->OnMouseMoved(submenu_, event);
}

bool MenuHostRootView::OnMouseWheel(const MouseWheelEvent& event) {
#if defined(OS_LINUX)
  // ChromeOS uses MenuController to forward events like other
  // mouse events.
  return GetMenuController() &&
      GetMenuController()->OnMouseWheel(submenu_, event);
#else
  // Windows uses focus_util_win::RerouteMouseWheel to forward events to
  // the right menu.
  // RootView::OnMouseWheel forwards to the focused view. We don't have a
  // focused view, so we need to override this then forward to the menu.
  return submenu_->OnMouseWheel(event);
#endif
}

MenuController* MenuHostRootView::GetMenuController() {
  return submenu_ ? submenu_->GetMenuItem()->GetMenuController() : NULL;
}

}  // namespace views
