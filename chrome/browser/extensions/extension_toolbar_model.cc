// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_toolbar_model.h"

#include <string>

#include "base/metrics/histogram.h"
#include "base/metrics/histogram_base.h"
#include "base/prefs/pref_service.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/api/extension_action/extension_action_api.h"
#include "chrome/browser/extensions/extension_action.h"
#include "chrome/browser/extensions/extension_action_manager.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/extensions/extension_toolbar_model_factory.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/pref_names.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/feature_switch.h"

using extensions::Extension;
using extensions::ExtensionIdList;
using extensions::ExtensionList;

bool ExtensionToolbarModel::Observer::BrowserActionShowPopup(
    const extensions::Extension* extension) {
  return false;
}

ExtensionToolbarModel::ExtensionToolbarModel(
    Profile* profile,
    extensions::ExtensionPrefs* extension_prefs)
    : profile_(profile),
      extension_prefs_(extension_prefs),
      prefs_(profile_->GetPrefs()),
      extensions_initialized_(false),
      weak_ptr_factory_(this) {
  registrar_.Add(this, chrome::NOTIFICATION_EXTENSION_LOADED,
                 content::Source<Profile>(profile_));
  registrar_.Add(this, chrome::NOTIFICATION_EXTENSION_UNLOADED,
                 content::Source<Profile>(profile_));
  registrar_.Add(this, chrome::NOTIFICATION_EXTENSIONS_READY,
                 content::Source<Profile>(profile_));
  registrar_.Add(this, chrome::NOTIFICATION_EXTENSION_UNINSTALLED,
                 content::Source<Profile>(profile_));
  registrar_.Add(
      this, chrome::NOTIFICATION_EXTENSION_BROWSER_ACTION_VISIBILITY_CHANGED,
      content::Source<extensions::ExtensionPrefs>(extension_prefs_));

  visible_icon_count_ = prefs_->GetInteger(
      extensions::pref_names::kToolbarSize);
  pref_change_registrar_.Init(prefs_);
  pref_change_callback_ =
      base::Bind(&ExtensionToolbarModel::OnExtensionToolbarPrefChange,
                 base::Unretained(this));
  pref_change_registrar_.Add(extensions::pref_names::kToolbar,
                             pref_change_callback_);
}

ExtensionToolbarModel::~ExtensionToolbarModel() {
}

// static
ExtensionToolbarModel* ExtensionToolbarModel::Get(Profile* profile) {
  return ExtensionToolbarModelFactory::GetForProfile(profile);
}

void ExtensionToolbarModel::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void ExtensionToolbarModel::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void ExtensionToolbarModel::MoveBrowserAction(const Extension* extension,
                                              int index) {
  ExtensionList::iterator pos = std::find(toolbar_items_.begin(),
      toolbar_items_.end(), extension);
  if (pos == toolbar_items_.end()) {
    NOTREACHED();
    return;
  }
  toolbar_items_.erase(pos);

  ExtensionIdList::iterator pos_id;
  pos_id = std::find(last_known_positions_.begin(),
                     last_known_positions_.end(), extension->id());
  if (pos_id != last_known_positions_.end())
    last_known_positions_.erase(pos_id);

  int i = 0;
  bool inserted = false;
  for (ExtensionList::iterator iter = toolbar_items_.begin();
       iter != toolbar_items_.end();
       ++iter, ++i) {
    if (i == index) {
      pos_id = std::find(last_known_positions_.begin(),
                         last_known_positions_.end(), (*iter)->id());
      last_known_positions_.insert(pos_id, extension->id());

      toolbar_items_.insert(iter, make_scoped_refptr(extension));
      inserted = true;
      break;
    }
  }

  if (!inserted) {
    DCHECK_EQ(index, static_cast<int>(toolbar_items_.size()));
    index = toolbar_items_.size();

    toolbar_items_.push_back(make_scoped_refptr(extension));
    last_known_positions_.push_back(extension->id());
  }

  FOR_EACH_OBSERVER(Observer, observers_, BrowserActionMoved(extension, index));

  UpdatePrefs();
}

ExtensionToolbarModel::Action ExtensionToolbarModel::ExecuteBrowserAction(
    const Extension* extension,
    Browser* browser,
    GURL* popup_url_out,
    bool should_grant) {
  content::WebContents* web_contents = NULL;
  int tab_id = 0;
  if (!extensions::ExtensionTabUtil::GetDefaultTab(
          browser, &web_contents, &tab_id)) {
    return ACTION_NONE;
  }

  ExtensionAction* browser_action =
      extensions::ExtensionActionManager::Get(profile_)->
      GetBrowserAction(*extension);

  // For browser actions, visibility == enabledness.
  if (!browser_action->GetIsVisible(tab_id))
    return ACTION_NONE;

  if (should_grant) {
    extensions::TabHelper::FromWebContents(web_contents)->
        active_tab_permission_granter()->GrantIfRequested(extension);
  }

  if (browser_action->HasPopup(tab_id)) {
    if (popup_url_out)
      *popup_url_out = browser_action->GetPopupUrl(tab_id);
    return ACTION_SHOW_POPUP;
  }

  extensions::ExtensionActionAPI::BrowserActionExecuted(
      browser->profile(), *browser_action, web_contents);
  return ACTION_NONE;
}

void ExtensionToolbarModel::SetVisibleIconCount(int count) {
  visible_icon_count_ =
      count == static_cast<int>(toolbar_items_.size()) ? -1 : count;
  prefs_->SetInteger(extensions::pref_names::kToolbarSize, visible_icon_count_);
}

void ExtensionToolbarModel::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  ExtensionService* extension_service =
      extensions::ExtensionSystem::Get(profile_)->extension_service();
  if (!extension_service || !extension_service->is_ready())
    return;

  if (type == chrome::NOTIFICATION_EXTENSIONS_READY) {
    InitializeExtensionList(extension_service);
    return;
  }

  const Extension* extension = NULL;
  if (type == chrome::NOTIFICATION_EXTENSION_UNLOADED) {
    extension = content::Details<extensions::UnloadedExtensionInfo>(
        details)->extension;
  } else if (type ==
      chrome::NOTIFICATION_EXTENSION_BROWSER_ACTION_VISIBILITY_CHANGED) {
    extension = extension_service->GetExtensionById(
        *content::Details<const std::string>(details).ptr(), true);
  } else {
    extension = content::Details<const Extension>(details).ptr();
  }
  if (type == chrome::NOTIFICATION_EXTENSION_LOADED) {
    // We don't want to add the same extension twice. It may have already been
    // added by EXTENSION_BROWSER_ACTION_VISIBILITY_CHANGED below, if the user
    // hides the browser action and then disables and enables the extension.
    for (size_t i = 0; i < toolbar_items_.size(); i++) {
      if (toolbar_items_[i].get() == extension)
        return;  // Already exists.
    }
    if (extensions::ExtensionActionAPI::GetBrowserActionVisibility(
            extension_prefs_, extension->id())) {
      AddExtension(extension);
    }
  } else if (type == chrome::NOTIFICATION_EXTENSION_UNLOADED) {
    RemoveExtension(extension);
  } else if (type == chrome::NOTIFICATION_EXTENSION_UNINSTALLED) {
    UninstalledExtension(extension);
  } else if (type ==
      chrome::NOTIFICATION_EXTENSION_BROWSER_ACTION_VISIBILITY_CHANGED) {
    if (extensions::ExtensionActionAPI::GetBrowserActionVisibility(
            extension_prefs_, extension->id())) {
      AddExtension(extension);
    } else {
      RemoveExtension(extension);
    }
  } else {
    NOTREACHED() << "Received unexpected notification";
  }
}

size_t ExtensionToolbarModel::FindNewPositionFromLastKnownGood(
    const Extension* extension) {
  // See if we have last known good position for this extension.
  size_t new_index = 0;
  // Loop through the ID list of known positions, to count the number of visible
  // browser action icons preceding |extension|.
  for (ExtensionIdList::const_iterator iter_id = last_known_positions_.begin();
       iter_id < last_known_positions_.end(); ++iter_id) {
    if ((*iter_id) == extension->id())
      return new_index;  // We've found the right position.
    // Found an id, need to see if it is visible.
    for (ExtensionList::const_iterator iter_ext = toolbar_items_.begin();
         iter_ext < toolbar_items_.end(); ++iter_ext) {
      if ((*iter_ext)->id().compare(*iter_id) == 0) {
        // This extension is visible, update the index value.
        ++new_index;
        break;
      }
    }
  }

  return -1;
}

void ExtensionToolbarModel::AddExtension(const Extension* extension) {
  // We only care about extensions with browser actions.
  if (!extensions::ExtensionActionManager::Get(profile_)->
      GetBrowserAction(*extension)) {
    return;
  }

  size_t new_index = -1;

  // See if we have a last known good position for this extension.
  ExtensionIdList::iterator last_pos = std::find(last_known_positions_.begin(),
                                                 last_known_positions_.end(),
                                                 extension->id());
  if (last_pos != last_known_positions_.end()) {
    new_index = FindNewPositionFromLastKnownGood(extension);
    if (new_index != toolbar_items_.size()) {
      toolbar_items_.insert(toolbar_items_.begin() + new_index,
                            make_scoped_refptr(extension));
    } else {
      toolbar_items_.push_back(make_scoped_refptr(extension));
    }
  } else {
    // This is a never before seen extension, that was added to the end. Make
    // sure to reflect that.
    toolbar_items_.push_back(make_scoped_refptr(extension));
    last_known_positions_.push_back(extension->id());
    new_index = toolbar_items_.size() - 1;
    UpdatePrefs();
  }

  FOR_EACH_OBSERVER(Observer, observers_,
                    BrowserActionAdded(extension, new_index));
}

void ExtensionToolbarModel::RemoveExtension(const Extension* extension) {
  ExtensionList::iterator pos =
      std::find(toolbar_items_.begin(), toolbar_items_.end(), extension);
  if (pos == toolbar_items_.end())
    return;

  toolbar_items_.erase(pos);
  FOR_EACH_OBSERVER(Observer, observers_,
                    BrowserActionRemoved(extension));

  UpdatePrefs();
}

void ExtensionToolbarModel::UninstalledExtension(const Extension* extension) {
  // Remove the extension id from the ordered list, if it exists (the extension
  // might not be represented in the list because it might not have an icon).
  ExtensionIdList::iterator pos =
      std::find(last_known_positions_.begin(),
                last_known_positions_.end(), extension->id());

  if (pos != last_known_positions_.end()) {
    last_known_positions_.erase(pos);
    UpdatePrefs();
  }
}

// Combine the currently enabled extensions that have browser actions (which
// we get from the ExtensionService) with the ordering we get from the
// pref service. For robustness we use a somewhat inefficient process:
// 1. Create a vector of extensions sorted by their pref values. This vector may
// have holes.
// 2. Create a vector of extensions that did not have a pref value.
// 3. Remove holes from the sorted vector and append the unsorted vector.
void ExtensionToolbarModel::InitializeExtensionList(ExtensionService* service) {
  DCHECK(service->is_ready());

  last_known_positions_ = extension_prefs_->GetToolbarOrder();
  Populate(last_known_positions_, service);

  extensions_initialized_ = true;
  FOR_EACH_OBSERVER(Observer, observers_, VisibleCountChanged());
}

void ExtensionToolbarModel::Populate(
    const extensions::ExtensionIdList& positions,
    ExtensionService* service) {
  // Items that have explicit positions.
  ExtensionList sorted;
  sorted.resize(positions.size(), NULL);
  // The items that don't have explicit positions.
  ExtensionList unsorted;

  extensions::ExtensionActionManager* extension_action_manager =
      extensions::ExtensionActionManager::Get(profile_);

  // Create the lists.
  int hidden = 0;
  for (extensions::ExtensionSet::const_iterator it =
           service->extensions()->begin();
       it != service->extensions()->end(); ++it) {
    const Extension* extension = it->get();
    if (!extension_action_manager->GetBrowserAction(*extension))
      continue;
    if (!extensions::ExtensionActionAPI::GetBrowserActionVisibility(
            extension_prefs_, extension->id())) {
      ++hidden;
      continue;
    }

    extensions::ExtensionIdList::const_iterator pos =
        std::find(positions.begin(), positions.end(), extension->id());
    if (pos != positions.end())
      sorted[pos - positions.begin()] = extension;
    else
      unsorted.push_back(make_scoped_refptr(extension));
  }

  // Erase current icons.
  for (size_t i = 0; i < toolbar_items_.size(); i++) {
    FOR_EACH_OBSERVER(
        Observer, observers_, BrowserActionRemoved(toolbar_items_[i].get()));
  }
  toolbar_items_.clear();

  // Merge the lists.
  toolbar_items_.reserve(sorted.size() + unsorted.size());
  for (ExtensionList::const_iterator iter = sorted.begin();
       iter != sorted.end(); ++iter) {
    // It's possible for the extension order to contain items that aren't
    // actually loaded on this machine.  For example, when extension sync is on,
    // we sync the extension order as-is but double-check with the user before
    // syncing NPAPI-containing extensions, so if one of those is not actually
    // synced, we'll get a NULL in the list.  This sort of case can also happen
    // if some error prevents an extension from loading.
    if (iter->get() != NULL)
      toolbar_items_.push_back(*iter);
  }
  toolbar_items_.insert(toolbar_items_.end(), unsorted.begin(),
                        unsorted.end());

  UMA_HISTOGRAM_COUNTS_100(
      "ExtensionToolbarModel.BrowserActionsPermanentlyHidden", hidden);
  UMA_HISTOGRAM_COUNTS_100("ExtensionToolbarModel.BrowserActionsCount",
                           toolbar_items_.size());

  if (!toolbar_items_.empty()) {
    // Visible count can be -1, meaning: 'show all'. Since UMA converts negative
    // values to 0, this would be counted as 'show none' unless we convert it to
    // max.
    UMA_HISTOGRAM_COUNTS_100("ExtensionToolbarModel.BrowserActionsVisible",
                             visible_icon_count_ == -1 ?
                                 base::HistogramBase::kSampleType_MAX :
                                 visible_icon_count_);
  }

  // Inform observers.
  for (size_t i = 0; i < toolbar_items_.size(); i++) {
    FOR_EACH_OBSERVER(
        Observer, observers_, BrowserActionAdded(toolbar_items_[i].get(), i));
  }
}

void ExtensionToolbarModel::UpdatePrefs() {
  if (!extension_prefs_)
    return;

  // Don't observe change caused by self.
  pref_change_registrar_.Remove(extensions::pref_names::kToolbar);
  extension_prefs_->SetToolbarOrder(last_known_positions_);
  pref_change_registrar_.Add(extensions::pref_names::kToolbar,
                             pref_change_callback_);
}

int ExtensionToolbarModel::IncognitoIndexToOriginal(int incognito_index) {
  int original_index = 0, i = 0;
  for (ExtensionList::iterator iter = toolbar_items_.begin();
       iter != toolbar_items_.end();
       ++iter, ++original_index) {
    if (extensions::util::IsIncognitoEnabled((*iter)->id(), profile_)) {
      if (incognito_index == i)
        break;
      ++i;
    }
  }
  return original_index;
}

int ExtensionToolbarModel::OriginalIndexToIncognito(int original_index) {
  int incognito_index = 0, i = 0;
  for (ExtensionList::iterator iter = toolbar_items_.begin();
       iter != toolbar_items_.end();
       ++iter, ++i) {
    if (original_index == i)
      break;
    if (extensions::util::IsIncognitoEnabled((*iter)->id(), profile_))
      ++incognito_index;
  }
  return incognito_index;
}

void ExtensionToolbarModel::OnExtensionToolbarPrefChange() {
  // If extensions are not ready, defer to later Populate() call.
  if (!extensions_initialized_)
    return;

  // Recalculate |last_known_positions_| to be |pref_positions| followed by
  // ones that are only in |last_known_positions_|.
  extensions::ExtensionIdList pref_positions =
      extension_prefs_->GetToolbarOrder();
  size_t pref_position_size = pref_positions.size();
  for (size_t i = 0; i < last_known_positions_.size(); ++i) {
    if (std::find(pref_positions.begin(), pref_positions.end(),
                  last_known_positions_[i]) == pref_positions.end()) {
      pref_positions.push_back(last_known_positions_[i]);
    }
  }
  last_known_positions_.swap(pref_positions);

  // Re-populate.
  Populate(last_known_positions_,
           extensions::ExtensionSystem::Get(profile_)->extension_service());

  if (last_known_positions_.size() > pref_position_size) {
    // Need to update pref because we have extra icons. But can't call
    // UpdatePrefs() directly within observation closure.
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&ExtensionToolbarModel::UpdatePrefs,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

bool ExtensionToolbarModel::ShowBrowserActionPopup(
    const extensions::Extension* extension) {
  ObserverListBase<Observer>::Iterator it(observers_);
  Observer* obs = NULL;
  while ((obs = it.GetNext()) != NULL) {
    // Stop after first popup since it should only show in the active window.
    if (obs->BrowserActionShowPopup(extension))
      return true;
  }
  return false;
}

void ExtensionToolbarModel::EnsureVisibility(
    const extensions::ExtensionIdList& extension_ids) {
  if (visible_icon_count_ == -1)
    return;  // Already showing all.

  // Otherwise, make sure we have enough room to show all the extensions
  // requested.
  if (visible_icon_count_ < static_cast<int>(extension_ids.size())) {
    SetVisibleIconCount(extension_ids.size());

    // Inform observers.
    FOR_EACH_OBSERVER(Observer, observers_, VisibleCountChanged());
  }

  if (visible_icon_count_ == -1)
    return;  // May have been set to max by SetVisibleIconCount.

  // Guillotine's Delight: Move an orange noble to the front of the line.
  for (ExtensionIdList::const_iterator it = extension_ids.begin();
       it != extension_ids.end(); ++it) {
    for (ExtensionList::const_iterator extension = toolbar_items_.begin();
         extension != toolbar_items_.end(); ++extension) {
      if ((*extension)->id() == (*it)) {
        if (extension - toolbar_items_.begin() >= visible_icon_count_)
          MoveBrowserAction(*extension, 0);
        break;
      }
    }
  }
}
