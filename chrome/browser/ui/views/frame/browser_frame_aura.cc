// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/browser_frame_aura.h"

#include "base/command_line.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/system_menu_model_delegate.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/aura/window_observer.h"
#include "ui/base/hit_test.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/gfx/font.h"
#include "ui/views/controls/menu/menu_model_adapter.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/view.h"
#include "ui/views/views_switches.h"

#if defined(USE_ASH)
#include "ash/wm/property_util.h"
#endif

#if !defined(OS_CHROMEOS)
#include "chrome/browser/ui/views/frame/desktop_browser_frame_aura.h"
#endif

using aura::Window;

////////////////////////////////////////////////////////////////////////////////
// BrowserFrameAura::WindowPropertyWatcher

class BrowserFrameAura::WindowPropertyWatcher : public aura::WindowObserver {
 public:
  explicit WindowPropertyWatcher(BrowserFrameAura* browser_frame_aura,
                                 BrowserFrame* browser_frame)
      : browser_frame_aura_(browser_frame_aura),
        browser_frame_(browser_frame) {}

  virtual void OnWindowPropertyChanged(aura::Window* window,
                                       const void* key,
                                       intptr_t old) OVERRIDE {
    if (key != aura::client::kShowStateKey)
      return;

    ui::WindowShowState old_state = static_cast<ui::WindowShowState>(old);
    ui::WindowShowState new_state =
        window->GetProperty(aura::client::kShowStateKey);

    // Allow the frame to be replaced when entering or exiting the maximized
    // state.
    if (browser_frame_->non_client_view() &&
        browser_frame_aura_->browser_view()->browser()->is_app() &&
        (old_state == ui::SHOW_STATE_MAXIMIZED ||
         new_state == ui::SHOW_STATE_MAXIMIZED)) {
      // Defer frame layout when replacing the frame. Layout will occur when the
      // window's bounds are updated. The window maximize/restore animations
      // clone the window's layers and rely on the subsequent layout to set
      // the layer sizes.
      // If the window is minimized, the frame view needs to be updated via
      // an OnBoundsChanged event so that the frame will change its size
      // properly.
      browser_frame_->non_client_view()->UpdateFrame(
          old_state == ui::SHOW_STATE_MINIMIZED);
    }
  }

  virtual void OnWindowBoundsChanged(aura::Window* window,
                                     const gfx::Rect& old_bounds,
                                     const gfx::Rect& new_bounds) OVERRIDE {
    // Don't do anything if we don't have our non-client view yet.
    if (!browser_frame_->non_client_view())
      return;

    // If the window just moved to the top of the screen, or just moved away
    // from it, invoke Layout() so the header size can change.
    if ((old_bounds.y() == 0 && new_bounds.y() != 0) ||
        (old_bounds.y() != 0 && new_bounds.y() == 0))
      browser_frame_->non_client_view()->Layout();
  }

 private:
  BrowserFrameAura* browser_frame_aura_;
  BrowserFrame* browser_frame_;

  DISALLOW_COPY_AND_ASSIGN(WindowPropertyWatcher);
};

///////////////////////////////////////////////////////////////////////////////
// BrowserFrameAura, public:

BrowserFrameAura::BrowserFrameAura(BrowserFrame* browser_frame,
                                   BrowserView* browser_view)
    : views::NativeWidgetAura(browser_frame),
      browser_view_(browser_view),
      window_property_watcher_(new WindowPropertyWatcher(this, browser_frame)) {
  GetNativeWindow()->SetName("BrowserFrameAura");
  GetNativeWindow()->AddObserver(window_property_watcher_.get());
#if defined(USE_ASH)
  // Tabbed browsers and apps (some apps are TYPE_POPUP) get their own
  // workspace.
  if (browser_view->browser()->type() != Browser::TYPE_POPUP ||
      browser_view->browser()->is_app()) {
    ash::SetPersistsAcrossAllWorkspaces(
        GetNativeWindow(),
        ash::WINDOW_PERSISTS_ACROSS_ALL_WORKSPACES_VALUE_NO);
  }
#endif
}

///////////////////////////////////////////////////////////////////////////////
// BrowserFrameAura, views::ContextMenuController overrides:
void BrowserFrameAura::ShowContextMenuForView(views::View* source,
                                              const gfx::Point& p) {
  // Only show context menu if point is in unobscured parts of browser, i.e.
  // if NonClientHitTest returns :
  // - HTCAPTION: in title bar or unobscured part of tabstrip
  // - HTNOWHERE: as the name implies.
  views::NonClientView* non_client_view = browser_view()->frame()->
      non_client_view();
  gfx::Point point_in_view_coords(p);
  views::View::ConvertPointFromScreen(non_client_view, &point_in_view_coords);
  int hit_test = non_client_view->NonClientHitTest(point_in_view_coords);
  if (hit_test == HTCAPTION || hit_test == HTNOWHERE) {
    SystemMenuModelDelegate menu_delegate(browser_view(),
                                          browser_view()->browser());
    ui::SimpleMenuModel model(&menu_delegate);
    model.AddItemWithStringId(IDC_RESTORE_TAB, IDS_RESTORE_TAB);
    model.AddItemWithStringId(IDC_NEW_TAB, IDS_NEW_TAB);
    model.AddSeparator(ui::NORMAL_SEPARATOR);
    model.AddItemWithStringId(IDC_TASK_MANAGER, IDS_TASK_MANAGER);
    views::MenuModelAdapter menu_adapter(&model);
    menu_runner_.reset(new views::MenuRunner(menu_adapter.CreateMenu()));

    if (menu_runner_->RunMenuAt(source->GetWidget(), NULL,
          gfx::Rect(p, gfx::Size(0,0)), views::MenuItemView::TOPLEFT,
          views::MenuRunner::HAS_MNEMONICS | views::MenuRunner::CONTEXT_MENU) ==
        views::MenuRunner::MENU_DELETED)
      return;
  }
}

///////////////////////////////////////////////////////////////////////////////
// BrowserFrameAura, views::NativeWidgetAura overrides:

void BrowserFrameAura::OnWindowDestroying() {
  // Window is destroyed before our destructor is called, so clean up our
  // observer here.
  GetNativeWindow()->RemoveObserver(window_property_watcher_.get());
  views::NativeWidgetAura::OnWindowDestroying();
}

void BrowserFrameAura::OnWindowTargetVisibilityChanged(bool visible) {
  // On Aura when the BrowserView is shown it tries to restore focus, but can
  // be blocked when this parent BrowserFrameAura isn't visible. Therefore we
  // RestoreFocus() when we become visible, which results in the web contents
  // being asked to focus, which places focus either in the web contents or in
  // the location bar as appropriate.
  if (visible)
    browser_view_->RestoreFocus();
  views::NativeWidgetAura::OnWindowTargetVisibilityChanged(visible);
}

////////////////////////////////////////////////////////////////////////////////
// BrowserFrameAura, NativeBrowserFrame implementation:

views::NativeWidget* BrowserFrameAura::AsNativeWidget() {
  return this;
}

const views::NativeWidget* BrowserFrameAura::AsNativeWidget() const {
  return this;
}

void BrowserFrameAura::InitSystemContextMenu() {
  views::NonClientView* non_client_view =
      browser_view()->frame()->non_client_view();
  DCHECK(non_client_view);
  non_client_view->set_context_menu_controller(this);
}

int BrowserFrameAura::GetMinimizeButtonOffset() const {
  return 0;
}

void BrowserFrameAura::TabStripDisplayModeChanged() {
}

////////////////////////////////////////////////////////////////////////////////
// BrowserFrame, public:

// static
const gfx::Font& BrowserFrame::GetTitleFont() {
  static gfx::Font* title_font = new gfx::Font;
  return *title_font;
}

////////////////////////////////////////////////////////////////////////////////
// NativeBrowserFrame, public:

// static
NativeBrowserFrame* NativeBrowserFrame::CreateNativeBrowserFrame(
    BrowserFrame* browser_frame,
    BrowserView* browser_view) {
#if !defined(OS_CHROMEOS)
  if (CommandLine::ForCurrentProcess()->HasSwitch(
        views::switches::kDesktopAura))
    return new DesktopBrowserFrameAura(browser_frame, browser_view);
#endif
  return new BrowserFrameAura(browser_frame, browser_view);
}

///////////////////////////////////////////////////////////////////////////////
// BrowserFrameAura, private:

BrowserFrameAura::~BrowserFrameAura() {
}
