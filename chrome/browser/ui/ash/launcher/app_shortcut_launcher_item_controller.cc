// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/app_shortcut_launcher_item_controller.h"

#include "ash/wm/window_util.h"
#include "chrome/browser/extensions/extension_process_manager.h"
#include "chrome/browser/extensions/extension_system.h"
#include "chrome/browser/favicon/favicon_tab_helper.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_app_menu_item.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_app_menu_item_tab.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller_per_app.h"
#include "chrome/browser/ui/ash/launcher/launcher_item_controller.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/extensions/native_app_window.h"
#include "chrome/browser/ui/host_desktop.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/extensions/manifest_handlers/app_launch_info.h"
#include "content/public/browser/web_contents.h"
#include "ui/aura/window.h"
#include "ui/base/events/event.h"
#include "ui/views/corewm/window_animations.h"

using extensions::Extension;

namespace {

// The time delta between clicks in which clicks to launch V2 apps are ignored.
const int kClickSuppressionInMS = 1000;

}  // namespace

// Item controller for an app shortcut. Shortcuts track app and launcher ids,
// but do not have any associated windows (opening a shortcut will replace the
// item with the appropriate LauncherItemController type).
AppShortcutLauncherItemController::AppShortcutLauncherItemController(
    const std::string& app_id,
    ChromeLauncherControllerPerApp* controller)
    : LauncherItemController(TYPE_SHORTCUT, app_id, controller),
      app_controller_(controller) {
  // To detect V1 applications we use their domain and match them against the
  // used URL. This will also work with applications like Google Drive.
  const Extension* extension =
      launcher_controller()->GetExtensionForAppID(app_id);
  // Some unit tests have no real extension.
  if (extension) {
    set_refocus_url(GURL(
        extensions::AppLaunchInfo::GetLaunchWebURL(extension).spec() + "*"));
  }
}

AppShortcutLauncherItemController::~AppShortcutLauncherItemController() {
}

string16 AppShortcutLauncherItemController::GetTitle() {
  return GetAppTitle();
}

bool AppShortcutLauncherItemController::HasWindow(aura::Window* window) const {
  std::vector<content::WebContents*> content =
      app_controller_->GetV1ApplicationsFromAppId(app_id());
  for (size_t i = 0; i < content.size(); i++) {
    Browser* browser = chrome::FindBrowserWithWebContents(content[i]);
    if (browser && browser->window()->GetNativeWindow() == window)
      return true;
  }
  return false;
}

bool AppShortcutLauncherItemController::IsOpen() const {
  return !app_controller_->GetV1ApplicationsFromAppId(app_id()).empty();
}

bool AppShortcutLauncherItemController::IsVisible() const {
  // Return true if any browser window associated with the app is visible.
  std::vector<content::WebContents*> content =
      app_controller_->GetV1ApplicationsFromAppId(app_id());
  for (size_t i = 0; i < content.size(); i++) {
    Browser* browser = chrome::FindBrowserWithWebContents(content[i]);
    if (browser && browser->window()->GetNativeWindow()->IsVisible())
      return true;
  }
  return false;
}

void AppShortcutLauncherItemController::Launch(int event_flags) {
  app_controller_->LaunchApp(app_id(), event_flags);
}

void AppShortcutLauncherItemController::Activate() {
  content::WebContents* content = GetLRUApplication();
  if (!content) {
    if (IsV2App()) {
      // Ideally we come here only once. After that ShellLauncherItemController
      // will take over when the shell window gets opened. However there are
      // apps which take a lot of time for pre-processing (like the files app)
      // before they open a window. Since there is currently no other way to
      // detect if an app was started we suppress any further clicks within a
      // special time out.
      if (!AllowNextLaunchAttempt())
        return;
    }
    Launch(ui::EF_NONE);
    return;
  }
  ActivateContent(content);
}

void AppShortcutLauncherItemController::Close() {
  // Close all running 'programs' of this type.
  std::vector<content::WebContents*> content =
      app_controller_->GetV1ApplicationsFromAppId(app_id());
  for (size_t i = 0; i < content.size(); i++) {
    Browser* browser = chrome::FindBrowserWithWebContents(content[i]);
    if (!browser)
      continue;
    TabStripModel* tab_strip = browser->tab_strip_model();
    int index = tab_strip->GetIndexOfWebContents(content[i]);
    DCHECK(index != TabStripModel::kNoTab);
    tab_strip->CloseWebContentsAt(index, TabStripModel::CLOSE_NONE);
  }
}

void AppShortcutLauncherItemController::Clicked(const ui::Event& event) {
  // In case of a keyboard event, we were called by a hotkey. In that case we
  // activate the next item in line if an item of our list is already active.
  if (event.type() == ui::ET_KEY_RELEASED) {
    if (AdvanceToNextApp())
      return;
  }
  Activate();
}

void AppShortcutLauncherItemController::OnRemoved() {
  // AppShortcutLauncherItemController is unowned; delete on removal.
  delete this;
}

void AppShortcutLauncherItemController::LauncherItemChanged(
    int model_index,
    const ash::LauncherItem& old_item) {
}

ChromeLauncherAppMenuItems
AppShortcutLauncherItemController::GetApplicationList(int event_flags) {
  ChromeLauncherAppMenuItems items;
  // Add the application name to the menu.
  items.push_back(new ChromeLauncherAppMenuItem(GetTitle(), NULL, false));

  std::vector<content::WebContents*> content_list = GetRunningApplications();

  for (size_t i = 0; i < content_list.size(); i++) {
    content::WebContents* web_contents = content_list[i];
    // Get the icon.
    gfx::Image app_icon = app_controller_->GetAppListIcon(web_contents);
    string16 title = app_controller_->GetAppListTitle(web_contents);
    items.push_back(new ChromeLauncherAppMenuItemTab(
        title, &app_icon, web_contents, i == 0));
  }
  return items.Pass();
}

std::vector<content::WebContents*>
AppShortcutLauncherItemController::GetRunningApplications() {
  std::vector<content::WebContents*> items;

  URLPattern refocus_pattern(URLPattern::SCHEME_ALL);
  refocus_pattern.SetMatchAllURLs(true);

  if (!refocus_url_.is_empty()) {
    refocus_pattern.SetMatchAllURLs(false);
    refocus_pattern.Parse(refocus_url_.spec());
  }

  const Extension* extension =
      launcher_controller()->GetExtensionForAppID(app_id());

  // It is possible to come here While an extension gets loaded.
  if (!extension)
    return items;

  const BrowserList* ash_browser_list =
      BrowserList::GetInstance(chrome::HOST_DESKTOP_TYPE_ASH);
  for (BrowserList::const_iterator it = ash_browser_list->begin();
       it != ash_browser_list->end(); ++it) {
    Browser* browser = *it;
    TabStripModel* tab_strip = browser->tab_strip_model();
    for (int index = 0; index < tab_strip->count(); index++) {
      content::WebContents* web_contents = tab_strip->GetWebContentsAt(index);
      if (WebContentMatchesApp(extension, refocus_pattern, web_contents))
        items.push_back(web_contents);
    }
  }
  return items;
}

content::WebContents* AppShortcutLauncherItemController::GetLRUApplication() {
  URLPattern refocus_pattern(URLPattern::SCHEME_ALL);
  refocus_pattern.SetMatchAllURLs(true);

  if (!refocus_url_.is_empty()) {
    refocus_pattern.SetMatchAllURLs(false);
    refocus_pattern.Parse(refocus_url_.spec());
  }

  const Extension* extension =
      launcher_controller()->GetExtensionForAppID(app_id());

  // We may get here while the extension is loading (and NULL).
  if (!extension)
    return NULL;

  const BrowserList* ash_browser_list =
      BrowserList::GetInstance(chrome::HOST_DESKTOP_TYPE_ASH);
  for (BrowserList::const_reverse_iterator
       it = ash_browser_list->begin_last_active();
       it != ash_browser_list->end_last_active(); ++it) {
    Browser* browser = *it;
    TabStripModel* tab_strip = browser->tab_strip_model();
    // We start to enumerate from the active index.
    int active_index = tab_strip->active_index();
    for (int index = 0; index < tab_strip->count(); index++) {
      content::WebContents* web_contents = tab_strip->GetWebContentsAt(
          (index + active_index) % tab_strip->count());
      if (WebContentMatchesApp(extension, refocus_pattern, web_contents))
        return web_contents;
    }
  }
  return NULL;
}

bool AppShortcutLauncherItemController::WebContentMatchesApp(
    const extensions::Extension* extension,
    const URLPattern& refocus_pattern,
    content::WebContents* web_contents) {
  const GURL tab_url = web_contents->GetURL();
  // There are three ways to identify the association of a URL with this
  // extension:
  // - The refocus pattern is matched (needed for apps like drive).
  // - The extension's origin + extent gets matched.
  // - The launcher controller knows that the tab got created for this app.
  return ((!refocus_pattern.match_all_urls() &&
           refocus_pattern.MatchesURL(tab_url)) ||
          (extension->OverlapsWithOrigin(tab_url) &&
           extension->web_extent().MatchesURL(tab_url)) ||
          launcher_controller()->GetPerAppInterface()->
             IsWebContentHandledByApplication(web_contents, app_id()));
}

void AppShortcutLauncherItemController::ActivateContent(
    content::WebContents* content) {
  Browser* browser = chrome::FindBrowserWithWebContents(content);
  TabStripModel* tab_strip = browser->tab_strip_model();
  int index = tab_strip->GetIndexOfWebContents(content);
  DCHECK_NE(TabStripModel::kNoTab, index);

  int old_index = tab_strip->active_index();
  if (index != old_index)
    tab_strip->ActivateTabAt(index, false);
  app_controller_->ActivateWindowOrMinimizeIfActive(browser->window(),
      index == old_index && GetRunningApplications().size() == 1);
}

bool AppShortcutLauncherItemController::AdvanceToNextApp() {
  std::vector<content::WebContents*> items = GetRunningApplications();
  if (items.size() >= 1) {
    Browser* browser = chrome::FindBrowserWithWindow(
        ash::wm::GetActiveWindow());
    if (browser) {
      TabStripModel* tab_strip = browser->tab_strip_model();
      content::WebContents* active = tab_strip->GetWebContentsAt(
          tab_strip->active_index());
      std::vector<content::WebContents*>::const_iterator i(
          std::find(items.begin(), items.end(), active));
      if (i != items.end()) {
        if (items.size() == 1) {
          // If there is only a single item available, we animate it upon key
          // action.
          AnimateWindow(browser->window()->GetNativeWindow(),
              views::corewm::WINDOW_ANIMATION_TYPE_BOUNCE);
        } else {
          int index = (static_cast<int>(i - items.begin()) + 1) % items.size();
          ActivateContent(items[index]);
        }
        return true;
      }
    }
  }
  return false;
}

bool AppShortcutLauncherItemController::IsV2App() {
  const Extension* extension = app_controller_->GetExtensionForAppID(app_id());
  return extension && extension->is_platform_app();
}

bool AppShortcutLauncherItemController::AllowNextLaunchAttempt() {
  if (last_launch_attempt_.is_null() ||
      last_launch_attempt_ + base::TimeDelta::FromMilliseconds(
          kClickSuppressionInMS) < base::Time::Now()) {
    last_launch_attempt_ = base::Time::Now();
    return true;
  }
  return false;
}
