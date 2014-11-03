// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_EXTENSIONS_EXTENSION_ACTION_VIEW_CONTROLLER_H_
#define CHROME_BROWSER_UI_EXTENSIONS_EXTENSION_ACTION_VIEW_CONTROLLER_H_

#include "chrome/browser/extensions/extension_action_icon_factory.h"
#include "chrome/browser/extensions/extension_context_menu_model.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_controller.h"
#include "ui/gfx/image/image.h"

class Browser;
class ExtensionAction;
class ExtensionActionPlatformDelegate;
class GURL;

namespace extensions {
class Command;
class Extension;
}

// The platform-independent controller for an ExtensionAction that is shown on
// the toolbar (such as a page or browser action).
class ExtensionActionViewController
    : public ToolbarActionViewController,
      public ExtensionActionIconFactory::Observer,
      public ExtensionContextMenuModel::PopupDelegate {
 public:
  // The different options for showing a popup.
  enum PopupShowAction { SHOW_POPUP, SHOW_POPUP_AND_INSPECT };

  ExtensionActionViewController(const extensions::Extension* extension,
                                Browser* browser,
                                ExtensionAction* extension_action);
  ~ExtensionActionViewController() override;

  // ToolbarActionViewController:
  const std::string& GetId() const override;
  void SetDelegate(ToolbarActionViewDelegate* delegate) override;
  gfx::Image GetIcon(content::WebContents* web_contents) override;
  gfx::ImageSkia GetIconWithBadge() override;
  base::string16 GetActionName() const override;
  base::string16 GetAccessibleName(content::WebContents* web_contents) const
      override;
  base::string16 GetTooltip(content::WebContents* web_contents) const override;
  bool IsEnabled(content::WebContents* web_contents) const override;
  bool HasPopup(content::WebContents* web_contents) const override;
  void HidePopup() override;
  gfx::NativeView GetPopupNativeView() override;
  bool IsMenuRunning() const override;
  bool CanDrag() const override;
  bool ExecuteAction(bool by_user) override;
  void PaintExtra(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  content::WebContents* web_contents) const override;
  void RegisterCommand() override;

  // ExtensionContextMenuModel::PopupDelegate:
  void InspectPopup() override;

  // Populates |command| with the command associated with |extension|, if one
  // exists. Returns true if |command| was populated.
  bool GetExtensionCommand(extensions::Command* command);

  const extensions::Extension* extension() const { return extension_; }
  Browser* browser() { return browser_; }
  ExtensionAction* extension_action() { return extension_action_; }
  const ExtensionAction* extension_action() const { return extension_action_; }
  ToolbarActionViewDelegate* view_delegate() { return view_delegate_; }

  void set_icon_observer(ExtensionActionIconFactory::Observer* icon_observer) {
    icon_observer_ = icon_observer;
  }

 private:
  // ExtensionActionIconFactory::Observer:
  void OnIconUpdated() override;

  // Executes the extension action with |show_action|. If
  // |grant_tab_permissions| is true, this will grant the extension active tab
  // permissions. Only do this if this was done through a user action (and not
  // e.g. an API). Returns true if a popup is shown.
  bool ExecuteAction(PopupShowAction show_action, bool grant_tab_permissions);

  // Shows the popup for the extension action, given the associated |popup_url|.
  // |grant_tab_permissions| is true if active tab permissions should be given
  // to the extension; this is only true if the popup is opened through a user
  // action.
  // Returns true if a popup is successfully shown.
  bool ShowPopupWithUrl(PopupShowAction show_action,
                        const GURL& popup_url,
                        bool grant_tab_permissions);

  // The extension associated with the action we're displaying.
  const extensions::Extension* extension_;

  // The corresponding browser.
  Browser* browser_;

  // The browser action this view represents. The ExtensionAction is not owned
  // by this class.
  ExtensionAction* extension_action_;

  // Our view delegate.
  ToolbarActionViewDelegate* view_delegate_;

  // The delegate to handle platform-specific implementations.
  scoped_ptr<ExtensionActionPlatformDelegate> platform_delegate_;

  // The object that will be used to get the browser action icon for us.
  // It may load the icon asynchronously (in which case the initial icon
  // returned by the factory will be transparent), so we have to observe it for
  // updates to the icon.
  ExtensionActionIconFactory icon_factory_;

  // An additional observer that we need to notify when the icon of the button
  // has been updated.
  ExtensionActionIconFactory::Observer* icon_observer_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionActionViewController);
};

#endif  // CHROME_BROWSER_UI_EXTENSIONS_EXTENSION_ACTION_VIEW_CONTROLLER_H_
