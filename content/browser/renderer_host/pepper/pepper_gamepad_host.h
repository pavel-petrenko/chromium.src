// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_GAMEPAD_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_GAMEPAD_HOST_H_

#include "base/compiler_specific.h"
#include "content/common/content_export.h"
#include "ppapi/host/resource_host.h"

namespace content {

class BrowserPpapiHost;

class CONTENT_EXPORT PepperGamepadHost : public ppapi::host::ResourceHost {
 public:
  PepperGamepadHost(BrowserPpapiHost* host,
                    PP_Instance instance,
                    PP_Resource resource);
  virtual ~PepperGamepadHost();

  virtual int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) OVERRIDE;

 private:
  int32_t OnMsgRequestMemory(ppapi::host::HostMessageContext* context);

  BrowserPpapiHost* browser_ppapi_host_;

  DISALLOW_COPY_AND_ASSIGN(PepperGamepadHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_GAMEPAD_HOST_H_
