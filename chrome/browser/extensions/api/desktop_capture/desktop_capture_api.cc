// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/desktop_capture/desktop_capture_api.h"

#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "net/base/net_util.h"

namespace extensions {

namespace {

const char kNoTabIdError[] = "targetTab doesn't have id field set.";
const char kNoUrlError[] = "targetTab doesn't have URL field set.";
const char kInvalidOriginError[] = "targetTab.url is not a valid URL.";
const char kInvalidTabIdError[] = "Invalid tab specified.";
const char kTabUrlNotSecure[] =
    "URL scheme for the specified tab is not secure.";
}  // namespace

DesktopCaptureChooseDesktopMediaFunction::
    DesktopCaptureChooseDesktopMediaFunction() {
}

DesktopCaptureChooseDesktopMediaFunction::
    ~DesktopCaptureChooseDesktopMediaFunction() {
}

bool DesktopCaptureChooseDesktopMediaFunction::RunAsync() {
  EXTENSION_FUNCTION_VALIDATE(args_->GetSize() > 0);

  EXTENSION_FUNCTION_VALIDATE(args_->GetInteger(0, &request_id_));
  DesktopCaptureRequestsRegistry::GetInstance()->AddRequest(
      render_view_host()->GetProcess()->GetID(), request_id_, this);

  args_->Remove(0, NULL);

  scoped_ptr<api::desktop_capture::ChooseDesktopMedia::Params> params =
      api::desktop_capture::ChooseDesktopMedia::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params.get());

  // |web_contents| is the WebContents for which the stream is created, and will
  // also be used to determine where to show the picker's UI.
  content::WebContents* web_contents = NULL;
  base::string16 target_name;
  GURL origin;
  if (params->target_tab) {
    if (!params->target_tab->url) {
      error_ = kNoUrlError;
      return false;
    }
    origin = GURL(*(params->target_tab->url)).GetOrigin();

    if (!origin.is_valid()) {
      error_ = kInvalidOriginError;
      return false;
    }

    if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kAllowHttpScreenCapture) &&
        !origin.SchemeIsSecure()) {
      error_ = kTabUrlNotSecure;
      return false;
    }
    target_name = base::UTF8ToUTF16(origin.SchemeIsSecure() ?
        net::GetHostAndOptionalPort(origin) : origin.spec());

    if (!params->target_tab->id) {
      error_ = kNoTabIdError;
      return false;
    }

    if (!ExtensionTabUtil::GetTabById(*(params->target_tab->id), GetProfile(),
                                      true, NULL, NULL, &web_contents, NULL)) {
      error_ = kInvalidTabIdError;
      return false;
    }
    DCHECK(web_contents);
  } else {
    origin = extension()->url();
    target_name = base::UTF8ToUTF16(extension()->name());
    web_contents = content::WebContents::FromRenderViewHost(render_view_host());
    DCHECK(web_contents);
  }

  return Execute(params->sources, web_contents, origin, target_name);
}

DesktopCaptureCancelChooseDesktopMediaFunction::
    DesktopCaptureCancelChooseDesktopMediaFunction() {}

DesktopCaptureCancelChooseDesktopMediaFunction::
    ~DesktopCaptureCancelChooseDesktopMediaFunction() {}

}  // namespace extensions
