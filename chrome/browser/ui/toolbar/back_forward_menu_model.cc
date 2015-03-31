// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/toolbar/back_forward_menu_model.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/prefs/pref_service.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "components/favicon_base/favicon_types.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/user_metrics.h"
#include "content/public/browser/web_contents.h"
#include "grit/theme_resources.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/text_elider.h"

using base::UserMetricsAction;
using content::NavigationController;
using content::NavigationEntry;
using content::WebContents;

const int BackForwardMenuModel::kMaxHistoryItems = 12;
const int BackForwardMenuModel::kMaxChapterStops = 5;
static const int kMaxWidth = 700;

BackForwardMenuModel::BackForwardMenuModel(Browser* browser,
                                           ModelType model_type)
    : browser_(browser),
      test_web_contents_(NULL),
      model_type_(model_type),
      menu_model_delegate_(NULL) {
}

BackForwardMenuModel::~BackForwardMenuModel() {
}

bool BackForwardMenuModel::HasIcons() const {
  return true;
}

int BackForwardMenuModel::GetItemCount() const {
  int items = GetHistoryItemCount();

  if (items > 0) {
    int chapter_stops = 0;

    // Next, we count ChapterStops, if any.
    if (items == kMaxHistoryItems)
      chapter_stops = GetChapterStopCount(items);

    if (chapter_stops)
      items += chapter_stops + 1;  // Chapter stops also need a separator.

    // If the menu is not empty, add two positions in the end
    // for a separator and a "Show Full History" item.
    items += 2;
  }

  return items;
}

ui::MenuModel::ItemType BackForwardMenuModel::GetTypeAt(int index) const {
  return IsSeparator(index) ? TYPE_SEPARATOR : TYPE_COMMAND;
}

ui::MenuSeparatorType BackForwardMenuModel::GetSeparatorTypeAt(
    int index) const {
  return ui::NORMAL_SEPARATOR;
}

int BackForwardMenuModel::GetCommandIdAt(int index) const {
  return index;
}

base::string16 BackForwardMenuModel::GetLabelAt(int index) const {
  // Return label "Show Full History" for the last item of the menu.
  if (index == GetItemCount() - 1)
    return l10n_util::GetStringUTF16(IDS_SHOWFULLHISTORY_LINK);

  // Return an empty string for a separator.
  if (IsSeparator(index))
    return base::string16();

  // Return the entry title, escaping any '&' characters and eliding it if it's
  // super long.
  NavigationEntry* entry = GetNavigationEntry(index);
  Profile* profile =
      Profile::FromBrowserContext(GetWebContents()->GetBrowserContext());
  base::string16 menu_text(entry->GetTitleForDisplay(
      profile->GetPrefs()->GetString(prefs::kAcceptLanguages)));
  menu_text =
      gfx::ElideText(menu_text, gfx::FontList(), kMaxWidth, gfx::ELIDE_TAIL);

#if !defined(OS_MACOSX)
  for (size_t i = menu_text.find('&'); i != base::string16::npos;
       i = menu_text.find('&', i + 2)) {
    menu_text.insert(i, 1, '&');
  }
#endif

  return menu_text;
}

bool BackForwardMenuModel::IsItemDynamicAt(int index) const {
  // This object is only used for a single showing of a menu.
  return false;
}

bool BackForwardMenuModel::GetAcceleratorAt(
    int index,
    ui::Accelerator* accelerator) const {
  return false;
}

bool BackForwardMenuModel::IsItemCheckedAt(int index) const {
  return false;
}

int BackForwardMenuModel::GetGroupIdAt(int index) const {
  return false;
}

bool BackForwardMenuModel::GetIconAt(int index, gfx::Image* icon) {
  if (!ItemHasIcon(index))
    return false;

  if (index == GetItemCount() - 1) {
    *icon = ResourceBundle::GetSharedInstance().GetNativeImageNamed(
        IDR_HISTORY_FAVICON);
  } else {
    NavigationEntry* entry = GetNavigationEntry(index);
    *icon = entry->GetFavicon().image;
    if (!entry->GetFavicon().valid && menu_model_delegate()) {
      FetchFavicon(entry);
    }
  }

  return true;
}

ui::ButtonMenuItemModel* BackForwardMenuModel::GetButtonMenuItemAt(
    int index) const {
  return NULL;
}

bool BackForwardMenuModel::IsEnabledAt(int index) const {
  return index < GetItemCount() && !IsSeparator(index);
}

ui::MenuModel* BackForwardMenuModel::GetSubmenuModelAt(int index) const {
  return NULL;
}

void BackForwardMenuModel::HighlightChangedTo(int index) {
}

void BackForwardMenuModel::ActivatedAt(int index) {
  ActivatedAt(index, 0);
}

void BackForwardMenuModel::ActivatedAt(int index, int event_flags) {
  DCHECK(!IsSeparator(index));

  // Execute the command for the last item: "Show Full History".
  if (index == GetItemCount() - 1) {
    content::RecordComputedAction(BuildActionName("ShowFullHistory", -1));
    chrome::ShowSingletonTabOverwritingNTP(browser_,
        chrome::GetSingletonTabNavigateParams(
            browser_, GURL(chrome::kChromeUIHistoryURL)));
    return;
  }

  // Log whether it was a history or chapter click.
  if (index < GetHistoryItemCount()) {
    content::RecordComputedAction(
        BuildActionName("HistoryClick", index));
  } else {
    content::RecordComputedAction(
        BuildActionName("ChapterClick", index - GetHistoryItemCount() - 1));
  }

  int controller_index = MenuIndexToNavEntryIndex(index);
  WindowOpenDisposition disposition =
      ui::DispositionFromEventFlags(event_flags);
  if (!chrome::NavigateToIndexWithDisposition(browser_,
                                              controller_index,
                                              disposition)) {
    NOTREACHED();
  }
}

void BackForwardMenuModel::MenuWillShow() {
  content::RecordComputedAction(BuildActionName("Popup", -1));
  requested_favicons_.clear();
  cancelable_task_tracker_.TryCancelAll();
}

bool BackForwardMenuModel::IsSeparator(int index) const {
  int history_items = GetHistoryItemCount();
  // If the index is past the number of history items + separator,
  // we then consider if it is a chapter-stop entry.
  if (index > history_items) {
    // We either are in ChapterStop area, or at the end of the list (the "Show
    // Full History" link).
    int chapter_stops = GetChapterStopCount(history_items);
    if (chapter_stops == 0)
      return false;  // We must have reached the "Show Full History" link.
    // Otherwise, look to see if we have reached the separator for the
    // chapter-stops. If not, this is a chapter stop.
    return (index == history_items + 1 + chapter_stops);
  }

  // Look to see if we have reached the separator for the history items.
  return index == history_items;
}

void BackForwardMenuModel::SetMenuModelDelegate(
      ui::MenuModelDelegate* menu_model_delegate) {
  menu_model_delegate_ = menu_model_delegate;
}

ui::MenuModelDelegate* BackForwardMenuModel::GetMenuModelDelegate() const {
  return menu_model_delegate_;
}

void BackForwardMenuModel::FetchFavicon(NavigationEntry* entry) {
  // If the favicon has already been requested for this menu, don't do
  // anything.
  if (requested_favicons_.find(entry->GetUniqueID()) !=
      requested_favicons_.end()) {
    return;
  }
  requested_favicons_.insert(entry->GetUniqueID());
  FaviconService* favicon_service = FaviconServiceFactory::GetForProfile(
      browser_->profile(), ServiceAccessType::EXPLICIT_ACCESS);
  if (!favicon_service)
    return;

  favicon_service->GetFaviconImageForPageURL(
      entry->GetURL(),
      base::Bind(&BackForwardMenuModel::OnFavIconDataAvailable,
                 base::Unretained(this),
                 entry->GetUniqueID()),
      &cancelable_task_tracker_);
}

void BackForwardMenuModel::OnFavIconDataAvailable(
    int navigation_entry_unique_id,
    const favicon_base::FaviconImageResult& image_result) {
  if (!image_result.image.IsEmpty()) {
    // Find the current model_index for the unique id.
    NavigationEntry* entry = NULL;
    int model_index = -1;
    for (int i = 0; i < GetItemCount() - 1; i++) {
      if (IsSeparator(i))
        continue;
      if (GetNavigationEntry(i)->GetUniqueID() == navigation_entry_unique_id) {
        model_index = i;
        entry = GetNavigationEntry(i);
        break;
      }
    }

    if (!entry)
      // The NavigationEntry wasn't found, this can happen if the user
      // navigates to another page and a NavigatationEntry falls out of the
      // range of kMaxHistoryItems.
      return;

    // Now that we have a valid NavigationEntry, decode the favicon and assign
    // it to the NavigationEntry.
    entry->GetFavicon().valid = true;
    entry->GetFavicon().url = image_result.icon_url;
    entry->GetFavicon().image = image_result.image;
    if (menu_model_delegate()) {
      menu_model_delegate()->OnIconChanged(model_index);
    }
  }
}

int BackForwardMenuModel::GetHistoryItemCount() const {
  WebContents* contents = GetWebContents();
  int items = 0;

  if (model_type_ == FORWARD_MENU) {
    // Only count items from n+1 to end (if n is current entry)
    items = contents->GetController().GetEntryCount() -
            contents->GetController().GetCurrentEntryIndex() - 1;
  } else {
    items = contents->GetController().GetCurrentEntryIndex();
  }

  if (items > kMaxHistoryItems)
    items = kMaxHistoryItems;
  else if (items < 0)
    items = 0;

  return items;
}

int BackForwardMenuModel::GetChapterStopCount(int history_items) const {
  WebContents* contents = GetWebContents();

  int chapter_stops = 0;
  int current_entry = contents->GetController().GetCurrentEntryIndex();

  if (history_items == kMaxHistoryItems) {
    int chapter_id = current_entry;
    if (model_type_ == FORWARD_MENU) {
      chapter_id += history_items;
    } else {
      chapter_id -= history_items;
    }

    do {
      chapter_id = GetIndexOfNextChapterStop(chapter_id,
          model_type_ == FORWARD_MENU);
      if (chapter_id != -1)
        ++chapter_stops;
    } while (chapter_id != -1 && chapter_stops < kMaxChapterStops);
  }

  return chapter_stops;
}

int BackForwardMenuModel::GetIndexOfNextChapterStop(int start_from,
                                                    bool forward) const {
  WebContents* contents = GetWebContents();
  NavigationController& controller = contents->GetController();

  int max_count = controller.GetEntryCount();
  if (start_from < 0 || start_from >= max_count)
    return -1;  // Out of bounds.

  if (forward) {
    if (start_from < max_count - 1) {
      // We want to advance over the current chapter stop, so we add one.
      // We don't need to do this when direction is backwards.
      start_from++;
    } else {
      return -1;
    }
  }

  NavigationEntry* start_entry = controller.GetEntryAtIndex(start_from);
  const GURL& url = start_entry->GetURL();

  if (!forward) {
    // When going backwards we return the first entry we find that has a
    // different domain.
    for (int i = start_from - 1; i >= 0; --i) {
      if (!net::registry_controlled_domains::SameDomainOrHost(url,
              controller.GetEntryAtIndex(i)->GetURL(),
              net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES))
        return i;
    }
    // We have reached the beginning without finding a chapter stop.
    return -1;
  } else {
    // When going forwards we return the entry before the entry that has a
    // different domain.
    for (int i = start_from + 1; i < max_count; ++i) {
      if (!net::registry_controlled_domains::SameDomainOrHost(url,
              controller.GetEntryAtIndex(i)->GetURL(),
              net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES))
        return i - 1;
    }
    // Last entry is always considered a chapter stop.
    return max_count - 1;
  }
}

int BackForwardMenuModel::FindChapterStop(int offset,
                                          bool forward,
                                          int skip) const {
  if (offset < 0 || skip < 0)
    return -1;

  if (!forward)
    offset *= -1;

  WebContents* contents = GetWebContents();
  int entry = contents->GetController().GetCurrentEntryIndex() + offset;
  for (int i = 0; i < skip + 1; i++)
    entry = GetIndexOfNextChapterStop(entry, forward);

  return entry;
}

bool BackForwardMenuModel::ItemHasCommand(int index) const {
  return index < GetItemCount() && !IsSeparator(index);
}

bool BackForwardMenuModel::ItemHasIcon(int index) const {
  return index < GetItemCount() && !IsSeparator(index);
}

base::string16 BackForwardMenuModel::GetShowFullHistoryLabel() const {
  return l10n_util::GetStringUTF16(IDS_SHOWFULLHISTORY_LINK);
}

WebContents* BackForwardMenuModel::GetWebContents() const {
  // We use the test web contents if the unit test has specified it.
  return test_web_contents_ ?
      test_web_contents_ :
      browser_->tab_strip_model()->GetActiveWebContents();
}

int BackForwardMenuModel::MenuIndexToNavEntryIndex(int index) const {
  WebContents* contents = GetWebContents();
  int history_items = GetHistoryItemCount();

  DCHECK_GE(index, 0);

  // Convert anything above the History items separator.
  if (index < history_items) {
    if (model_type_ == FORWARD_MENU) {
      index += contents->GetController().GetCurrentEntryIndex() + 1;
    } else {
      // Back menu is reverse.
      index = contents->GetController().GetCurrentEntryIndex() - (index + 1);
    }
    return index;
  }
  if (index == history_items)
    return -1;  // Don't translate the separator for history items.

  if (index >= history_items + 1 + GetChapterStopCount(history_items))
    return -1;  // This is beyond the last chapter stop so we abort.

  // This menu item is a chapter stop located between the two separators.
  index = FindChapterStop(history_items,
                          model_type_ == FORWARD_MENU,
                          index - history_items - 1);

  return index;
}

NavigationEntry* BackForwardMenuModel::GetNavigationEntry(int index) const {
  int controller_index = MenuIndexToNavEntryIndex(index);
  NavigationController& controller = GetWebContents()->GetController();
  if (controller_index >= 0 && controller_index < controller.GetEntryCount())
    return controller.GetEntryAtIndex(controller_index);

  NOTREACHED();
  return NULL;
}

std::string BackForwardMenuModel::BuildActionName(
    const std::string& action, int index) const {
  DCHECK(!action.empty());
  DCHECK(index >= -1);
  std::string metric_string;
  if (model_type_ == FORWARD_MENU)
    metric_string += "ForwardMenu_";
  else
    metric_string += "BackMenu_";
  metric_string += action;
  if (index != -1) {
    // +1 is for historical reasons (indices used to start at 1).
    metric_string += base::IntToString(index + 1);
  }
  return metric_string;
}
