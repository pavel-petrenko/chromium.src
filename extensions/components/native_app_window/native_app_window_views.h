// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMPONENTS_NATIVE_APP_WINDOW_NATIVE_APP_WINDOW_VIEWS_H_
#define EXTENSIONS_COMPONENTS_NATIVE_APP_WINDOW_NATIVE_APP_WINDOW_VIEWS_H_

#include "base/observer_list.h"
#include "content/public/browser/web_contents_observer.h"
#include "extensions/browser/app_window/app_window.h"
#include "extensions/browser/app_window/native_app_window.h"
#include "extensions/browser/app_window/size_constraints.h"
#include "ui/gfx/rect.h"
#include "ui/views/controls/webview/unhandled_keyboard_event_handler.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"

class SkRegion;

namespace content {
class BrowserContext;
class RenderViewHost;
class WebContents;
}

namespace extensions {
class Extension;
}

namespace ui {
class MenuModel;
}

namespace views {
class MenuRunner;
class WebView;
}

namespace native_app_window {

// A NativeAppWindow backed by a views::Widget. This class may be used alone
// as a stub or subclassed (for example, ChromeNativeAppWindowViews).
class NativeAppWindowViews : public extensions::NativeAppWindow,
                             public content::WebContentsObserver,
                             public views::WidgetDelegateView,
                             public views::WidgetObserver {
 public:
  NativeAppWindowViews();
  ~NativeAppWindowViews() override;
  void Init(extensions::AppWindow* app_window,
            const extensions::AppWindow::CreateParams& create_params);

  // Signal that CanHaveTransparentBackground has changed.
  void OnCanHaveAlphaEnabledChanged();

  views::Widget* widget() { return widget_; }

  void set_window_for_testing(views::Widget* window) { widget_ = window; }
  void set_web_view_for_testing(views::WebView* view) { web_view_ = view; }

 protected:
  extensions::AppWindow* app_window() { return app_window_; }
  const extensions::AppWindow* app_window() const { return app_window_; }

  const views::Widget* widget() const { return widget_; }

  views::WebView* web_view() { return web_view_; }

  // Initializes |widget_| for |app_window|.
  virtual void InitializeWindow(
      extensions::AppWindow* app_window,
      const extensions::AppWindow::CreateParams& create_params);

  // ui::BaseWindow implementation.
  bool IsActive() const override;
  bool IsMaximized() const override;
  bool IsMinimized() const override;
  bool IsFullscreen() const override;
  gfx::NativeWindow GetNativeWindow() const override;
  gfx::Rect GetRestoredBounds() const override;
  ui::WindowShowState GetRestoredState() const override;
  gfx::Rect GetBounds() const override;
  void Show() override;
  void ShowInactive() override;
  void Hide() override;
  void Close() override;
  void Activate() override;
  void Deactivate() override;
  void Maximize() override;
  void Minimize() override;
  void Restore() override;
  void SetBounds(const gfx::Rect& bounds) override;
  void FlashFrame(bool flash) override;
  bool IsAlwaysOnTop() const override;
  void SetAlwaysOnTop(bool always_on_top) override;

  // WidgetDelegate implementation.
  void OnWidgetMove() override;
  views::View* GetInitiallyFocusedView() override;
  bool CanResize() const override;
  bool CanMaximize() const override;
  bool CanMinimize() const override;
  base::string16 GetWindowTitle() const override;
  bool ShouldShowWindowTitle() const override;
  bool ShouldShowWindowIcon() const override;
  void SaveWindowPlacement(const gfx::Rect& bounds,
                           ui::WindowShowState show_state) override;
  void DeleteDelegate() override;
  views::Widget* GetWidget() override;
  const views::Widget* GetWidget() const override;
  views::View* GetContentsView() override;
  bool ShouldDescendIntoChildForEventHandling(
      gfx::NativeView child,
      const gfx::Point& location) override;

  // WidgetObserver implementation.
  void OnWidgetVisibilityChanged(views::Widget* widget, bool visible) override;
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;

  // WebContentsObserver implementation.
  void RenderViewCreated(content::RenderViewHost* render_view_host) override;
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override;

  // views::View implementation.
  void Layout() override;
  void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) override;
  gfx::Size GetMinimumSize() const override;
  gfx::Size GetMaximumSize() const override;
  void OnFocus() override;

  // NativeAppWindow implementation.
  void SetFullscreen(int fullscreen_types) override;
  bool IsFullscreenOrPending() const override;
  void UpdateWindowIcon() override;
  void UpdateWindowTitle() override;
  void UpdateBadgeIcon() override;
  void UpdateDraggableRegions(
      const std::vector<extensions::DraggableRegion>& regions) override;
  SkRegion* GetDraggableRegion() override;
  void UpdateShape(scoped_ptr<SkRegion> region) override;
  void HandleKeyboardEvent(
      const content::NativeWebKeyboardEvent& event) override;
  bool IsFrameless() const override;
  bool HasFrameColor() const override;
  SkColor ActiveFrameColor() const override;
  SkColor InactiveFrameColor() const override;
  gfx::Insets GetFrameInsets() const override;
  void HideWithApp() override;
  void ShowWithApp() override;
  void UpdateShelfMenu() override;
  gfx::Size GetContentMinimumSize() const override;
  gfx::Size GetContentMaximumSize() const override;
  void SetContentSizeConstraints(const gfx::Size& min_size,
                                 const gfx::Size& max_size) override;
  bool CanHaveAlphaEnabled() const override;
  void SetVisibleOnAllWorkspaces(bool always_visible) override;

  // web_modal::WebContentsModalDialogHost implementation.
  gfx::NativeView GetHostView() const override;
  gfx::Point GetDialogPosition(const gfx::Size& size) override;
  gfx::Size GetMaximumDialogSize() override;
  void AddObserver(web_modal::ModalDialogHostObserver* observer) override;
  void RemoveObserver(web_modal::ModalDialogHostObserver* observer) override;

 private:
  // Informs modal dialogs that they need to update their positions.
  void OnViewWasResized();

  extensions::AppWindow* app_window_;  // Not owned.
  views::WebView* web_view_;
  views::Widget* widget_;

  scoped_ptr<SkRegion> draggable_region_;

  bool frameless_;
  bool resizable_;
  extensions::SizeConstraints size_constraints_;

  views::UnhandledKeyboardEventHandler unhandled_keyboard_event_handler_;

  ObserverList<web_modal::ModalDialogHostObserver> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(NativeAppWindowViews);
};

}  // namespace native_app_window

#endif  // EXTENSIONS_COMPONENTS_NATIVE_APP_WINDOW_NATIVE_APP_WINDOW_VIEWS_H_
