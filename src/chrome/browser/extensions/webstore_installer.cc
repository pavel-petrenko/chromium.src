// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/webstore_installer.h"

#include "base/string_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/crx_installer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/tabs/tab_strip_model.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_constants.h"
#include "content/browser/tab_contents/navigation_controller.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "googleurl/src/gurl.h"
#include "net/base/escape.h"

namespace {

const char kInvalidIdError[] = "Invalid id";
const char kNoBrowserError[] = "No browser found";

const char kInlineInstallSource[] = "inline";
const char kDefaultInstallSource[] = "";

GURL GetWebstoreInstallURL(
    const std::string& extension_id, const std::string& install_source) {
  std::vector<std::string> params;
  params.push_back("id=" + extension_id);
  if (!install_source.empty()) {
    params.push_back("installsource=" + install_source);
  }
  params.push_back("lang=" + g_browser_process->GetApplicationLocale());
  params.push_back("uc");
  std::string url_string = extension_urls::GetWebstoreUpdateUrl(true).spec();

  GURL url(url_string + "?response=redirect&x=" +
      net::EscapeQueryParamValue(JoinString(params, '&'), true));
  DCHECK(url.is_valid());

  return url;
}

}  // namespace


WebstoreInstaller::WebstoreInstaller(Profile* profile,
                                     Delegate* delegate,
                                     NavigationController* controller,
                                     const std::string& id,
                                     int flags)
    : profile_(profile),
      delegate_(delegate),
      controller_(controller),
      id_(id),
      flags_(flags) {
  download_url_ = GetWebstoreInstallURL(id, flags & FLAG_INLINE_INSTALL ?
      kInlineInstallSource : kDefaultInstallSource);

  registrar_.Add(this, chrome::NOTIFICATION_EXTENSION_INSTALLED,
                 content::Source<Profile>(profile));
  registrar_.Add(this, chrome::NOTIFICATION_EXTENSION_INSTALL_ERROR,
                 content::Source<CrxInstaller>(NULL));
}

WebstoreInstaller::~WebstoreInstaller() {}

void WebstoreInstaller::Start() {
  AddRef();  // Balanced in ReportSuccess and ReportFailure.

  if (!Extension::IdIsValid(id_)) {
    ReportFailure(kInvalidIdError);
    return;
  }

  // TODO(mihaip): For inline installs, we pretend like the referrer is the
  // gallery, even though this could be an inline install, in order to pass the
  // checks in ExtensionService::IsDownloadFromGallery. We should instead pass
  // the real referrer, track if this is an inline install in the whitelist
  // entry and look that up when checking that this is a valid download.
  GURL referrer = controller_->GetActiveEntry()->url();
  if (flags_ & FLAG_INLINE_INSTALL)
    referrer = GURL(extension_urls::GetWebstoreItemDetailURLPrefix() + id_);

  // The download url for the given extension is contained in |download_url_|.
  // We will navigate the current tab to this url to start the download. The
  // download system will then pass the crx to the CrxInstaller.
  controller_->LoadURL(download_url_,
                       referrer,
                       content::PAGE_TRANSITION_LINK,
                       std::string());

}

void WebstoreInstaller::Observe(int type,
                                const content::NotificationSource& source,
                                const content::NotificationDetails& details) {
  switch (type) {
    case chrome::NOTIFICATION_EXTENSION_INSTALLED: {
      CHECK(profile_->IsSameProfile(content::Source<Profile>(source).ptr()));
      const Extension* extension =
          content::Details<const Extension>(details).ptr();
      if (id_ == extension->id())
        ReportSuccess();
      break;
    }

    case chrome::NOTIFICATION_EXTENSION_INSTALL_ERROR: {
      CrxInstaller* crx_installer = content::Source<CrxInstaller>(source).ptr();
      CHECK(crx_installer);
      if (!profile_->IsSameProfile(crx_installer->profile()))
        return;

      const std::string* error =
          content::Details<const std::string>(details).ptr();
      if (download_url_ == crx_installer->original_download_url())
        ReportFailure(*error);
      break;
    }

    default:
      NOTREACHED();
  }
}

void WebstoreInstaller::ReportFailure(const std::string& error) {
  if (delegate_)
    delegate_->OnExtensionInstallFailure(id_, error);

  Release();  // Balanced in Start().
}

void WebstoreInstaller::ReportSuccess() {
  if (delegate_)
    delegate_->OnExtensionInstallSuccess(id_);

  Release();  // Balanced in Start().
}
