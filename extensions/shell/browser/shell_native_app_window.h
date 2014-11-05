// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_BROWSER_SHELL_NATIVE_APP_WINDOW_H_
#define EXTENSIONS_SHELL_BROWSER_SHELL_NATIVE_APP_WINDOW_H_

#include "extensions/browser/app_window/app_window.h"
#include "extensions/browser/app_window/native_app_window.h"

namespace extensions {

// app_shell's NativeAppWindow implementation.
class ShellNativeAppWindow : public NativeAppWindow {
 public:
  ShellNativeAppWindow(AppWindow* app_window,
                       const AppWindow::CreateParams& params);
  ~ShellNativeAppWindow() override;

  // ui::BaseView overrides:
  bool IsActive() const override;
  bool IsMaximized() const override;
  bool IsMinimized() const override;
  bool IsFullscreen() const override;
  gfx::NativeWindow GetNativeWindow() const override;
  gfx::Rect GetRestoredBounds() const override;
  ui::WindowShowState GetRestoredState() const override;
  gfx::Rect GetBounds() const override;
  void Show() override;
  void Hide() override;
  void ShowInactive() override;
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

  // web_modal::ModalDialogHost overrides:
  gfx::NativeView GetHostView() const override;
  gfx::Point GetDialogPosition(const gfx::Size& size) override;
  void AddObserver(web_modal::ModalDialogHostObserver* observer) override;
  void RemoveObserver(web_modal::ModalDialogHostObserver* observer) override;

  // web_modal::WebContentsModalDialogHost overrides:
  gfx::Size GetMaximumDialogSize() override;

  // NativeAppWindow overrides:
  void SetFullscreen(int fullscreen_types) override;
  bool IsFullscreenOrPending() const override;
  void UpdateWindowIcon() override;
  void UpdateWindowTitle() override;
  void UpdateBadgeIcon() override;
  void UpdateDraggableRegions(
      const std::vector<DraggableRegion>& regions) override;
  SkRegion* GetDraggableRegion() override;
  void UpdateShape(scoped_ptr<SkRegion> region) override;
  void HandleKeyboardEvent(
      const content::NativeWebKeyboardEvent& event) override;
  bool IsFrameless() const override;
  bool HasFrameColor() const override;
  SkColor ActiveFrameColor() const override;
  SkColor InactiveFrameColor() const override;
  gfx::Insets GetFrameInsets() const override;
  void ShowWithApp() override;
  void HideWithApp() override;
  void UpdateShelfMenu() override;
  gfx::Size GetContentMinimumSize() const override;
  gfx::Size GetContentMaximumSize() const override;
  void SetContentSizeConstraints(const gfx::Size& min_size,
                                 const gfx::Size& max_size) override;
  void SetVisibleOnAllWorkspaces(bool always_visible) override;
  bool CanHaveAlphaEnabled() const override;

 private:
  aura::Window* GetWindow() const;

  AppWindow* app_window_;

  DISALLOW_COPY_AND_ASSIGN(ShellNativeAppWindow);
};

}  // namespace extensions

#endif  // EXTENSIONS_SHELL_BROWSER_SHELL_NATIVE_APP_WINDOW_H_
