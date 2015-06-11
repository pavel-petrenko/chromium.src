// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_APPS_APP_WINDOW_EASY_RESIZE_WINDOW_TARGETER_H_
#define CHROME_BROWSER_UI_VIEWS_APPS_APP_WINDOW_EASY_RESIZE_WINDOW_TARGETER_H_

#include "ui/wm/core/easy_resize_window_targeter.h"

namespace ui {
class BaseWindow;
}

// An EasyResizeEventTargeter whose behavior depends on the state of the app
// window.
class AppWindowEasyResizeWindowTargeter : public wm::EasyResizeWindowTargeter {
 public:
  // |aura_window| is the owner of this targeter.
  AppWindowEasyResizeWindowTargeter(aura::Window* aura_window,
                                    const gfx::Insets& insets,
                                    ui::BaseWindow* native_app_window);

  ~AppWindowEasyResizeWindowTargeter() override;

 protected:
  // aura::WindowTargeter:
  bool EventLocationInsideBounds(aura::Window* window,
                                 const ui::LocatedEvent& event) const override;

 private:
  ui::BaseWindow* native_app_window_;

  DISALLOW_COPY_AND_ASSIGN(AppWindowEasyResizeWindowTargeter);
};

#endif  // CHROME_BROWSER_UI_VIEWS_APPS_APP_WINDOW_EASY_RESIZE_WINDOW_TARGETER_H_
