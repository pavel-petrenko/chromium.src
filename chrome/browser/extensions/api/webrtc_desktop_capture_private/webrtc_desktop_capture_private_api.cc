// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/webrtc_desktop_capture_private/webrtc_desktop_capture_private_api.h"

#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/host_desktop.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/api/tabs.h"
#include "chrome/common/extensions/api/webrtc_desktop_capture_private.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "net/base/net_util.h"

namespace extensions {

namespace {

const char kInvalidRequestInfoError[] = "Invalid RequestInfo specified.";
const char kTargetNotFoundError[] = "The specified target is not found.";
const char kUrlNotSecure[] =
    "URL scheme for the specified target is not secure.";

}  // namespace

WebrtcDesktopCapturePrivateChooseDesktopMediaFunction::
    WebrtcDesktopCapturePrivateChooseDesktopMediaFunction() {
}

WebrtcDesktopCapturePrivateChooseDesktopMediaFunction::
    ~WebrtcDesktopCapturePrivateChooseDesktopMediaFunction() {
}

bool WebrtcDesktopCapturePrivateChooseDesktopMediaFunction::RunAsync() {
  using Params =
      extensions::api::webrtc_desktop_capture_private::ChooseDesktopMedia
          ::Params;
  EXTENSION_FUNCTION_VALIDATE(args_->GetSize() > 0);

  EXTENSION_FUNCTION_VALIDATE(args_->GetInteger(0, &request_id_));
  DesktopCaptureRequestsRegistry::GetInstance()->AddRequest(
      render_view_host()->GetProcess()->GetID(), request_id_, this);

  args_->Remove(0, NULL);

  scoped_ptr<Params> params = Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params.get());

  if (!params->request.guest_process_id || !params->request.render_view_id) {
    error_ = kInvalidRequestInfoError;
    return false;
  }

  content::RenderViewHost* rvh = content::RenderViewHost::FromID(
      *params->request.guest_process_id.get(),
      *params->request.render_view_id.get());

  if (!rvh) {
    error_ = kTargetNotFoundError;
    return false;
  }

  content::RenderFrameHost* rfh = rvh->GetMainFrame();
  if (!rfh) {
    error_ = kTargetNotFoundError;
    return false;
  }

  GURL origin = rfh->GetLastCommittedURL().GetOrigin();
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAllowHttpScreenCapture) &&
      !origin.SchemeIsSecure()) {
    error_ = kUrlNotSecure;
    return false;
  }
  base::string16 target_name = base::UTF8ToUTF16(origin.SchemeIsSecure() ?
      net::GetHostAndOptionalPort(origin) : origin.spec());

  content::WebContents* web_contents =
      content::WebContents::FromRenderViewHost(rvh);
  if (!web_contents) {
    error_ = kTargetNotFoundError;
    return false;
  }

  using Sources = std::vector<api::desktop_capture::DesktopCaptureSourceType>;
  Sources* sources = reinterpret_cast<Sources*>(&params->sources);
  return Execute(*sources, web_contents, origin, target_name);
}

WebrtcDesktopCapturePrivateCancelChooseDesktopMediaFunction::
    WebrtcDesktopCapturePrivateCancelChooseDesktopMediaFunction() {}

WebrtcDesktopCapturePrivateCancelChooseDesktopMediaFunction::
    ~WebrtcDesktopCapturePrivateCancelChooseDesktopMediaFunction() {}

}  // namespace extensions
