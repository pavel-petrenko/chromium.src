// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NACL_COMMON_NACL_BROWSER_DELEGATE_H_
#define COMPONENTS_NACL_COMMON_NACL_BROWSER_DELEGATE_H_

#include <string>

#include "base/callback_forward.h"

class GURL;

namespace base {
class FilePath;
}

namespace ppapi {
namespace host {
class HostFactory;
}
}

namespace content {
class BrowserPpapiHost;
}

// Encapsulates the dependencies of NaCl code on chrome/, to avoid a direct
// dependency on chrome/.
class NaClBrowserDelegate {
 public:
  virtual ~NaClBrowserDelegate() {}

  // Show an infobar to the user.
  virtual void ShowNaClInfobar(int render_process_id, int render_view_id,
                               int error_id) = 0;
  // Returns whether dialogs are allowed. This is used to decide if to add the
  // command line switch kNoErrorDialogs.
  virtual bool DialogsAreSuppressed() = 0;
  // Returns true on success, false otherwise. On success |cache_dir| contains
  // the cache directory. On failure, it is not changed.
  virtual bool GetCacheDirectory(base::FilePath* cache_dir) = 0;
  // Returns true on success, false otherwise. On success |plugin_dir| contains
  // the directory where the plugins are located. On failure, it is not
  // changed.
  virtual bool GetPluginDirectory(base::FilePath* plugin_dir) = 0;
  // Returns true on success, false otherwise. On success |pnacl_dir| contains
  // the directory where the PNaCl files are located. On failure, it is not
  // changed.
  virtual bool GetPnaclDirectory(base::FilePath* pnacl_dir) = 0;
  // Returns true on success, false otherwise. On success |user_dir| contains
  // the user data directory. On failure, it is not changed.
  virtual bool GetUserDirectory(base::FilePath* user_dir) = 0;
  // Returns the version as a string. This string is used to invalidate
  // validator cache entries when Chromium is upgraded
  virtual std::string GetVersionString() const = 0;
  // Returns a HostFactory that hides the details of its embedder.
  virtual ppapi::host::HostFactory* CreatePpapiHostFactory(
      content::BrowserPpapiHost* ppapi_host) = 0;
  // Returns true on success, false otherwise. On success, map |url| to a
  // full pathname of a file in the local filesystem. |file_path| should not be
  // changed on failure. This mapping should be a best effort, for example,
  // "chrome-extension:" could be mapped to the location of unpacked
  // extensions. If this method is called in a blocking thread you should set
  // |use_blocking_api| to true, so calling blocking file API is allowed
  // otherwise non blocking API will be used (which only handles a subset of the
  // urls checking only the url scheme against kExtensionScheme).
  virtual bool MapUrlToLocalFilePath(const GURL& url,
                                     bool use_blocking_api,
                                     base::FilePath* file_path) = 0;
  // Set match patterns which will be checked before enabling debug stub.
  virtual void SetDebugPatterns(std::string debug_patterns) = 0;

  // Returns whether NaCl application with this manifest URL should be debugged.
  virtual bool URLMatchesDebugPatterns(const GURL& manifest_url) = 0;
};

#endif  // COMPONENTS_NACL_COMMON_NACL_BROWSER_DELEGATE_H_
