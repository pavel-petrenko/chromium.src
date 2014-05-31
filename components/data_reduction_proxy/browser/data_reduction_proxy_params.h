// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_BROWSER_DATA_REDUCTION_PROXY_PARAMS_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_BROWSER_DATA_REDUCTION_PROXY_PARAMS_H_

#include <vector>

#include "base/macros.h"
#include "url/gurl.h"

namespace data_reduction_proxy {

// Provides initialization parameters. Proxy origins, the probe url, and the
// authentication key are taken from flags if available and from preprocessor
// constants otherwise. Only the key may be changed after construction.
class DataReductionProxyParams {
 public:

  static const unsigned int kAllowed = (1 << 0);
  static const unsigned int kFallbackAllowed = (1 << 1);
  static const unsigned int kAlternativeAllowed = (1 << 2);
  static const unsigned int kPromoAllowed = (1 << 3);

  typedef std::vector<GURL> DataReductionProxyList;

  // Returns true if this client is part of the data reduction proxy field
  // trial.
  static bool IsIncludedInFieldTrial();

  // Returns true if this client is part of field trial to use an alternative
  // configuration for the data reduction proxy.
  static bool IsIncludedInAlternativeFieldTrial();

  // Returns true if this client is part of the field trial that should display
  // a promotion for the data reduction proxy.
  static bool IsIncludedInPromoFieldTrial();

  // Returns true if this client is part of a field trial that uses preconnect
  // hinting.
  static bool IsIncludedInPreconnectHintingFieldTrial();

  // Returns true if the authentication key was set on the command line.
  static bool IsKeySetOnCommandLine();

  // Constructs configuration parameters. If |kAllowed|, then the standard
  // data reduction proxy configuration is allowed to be used. If
  // |kfallbackAllowed| a fallback proxy can be used if the primary proxy is
  // bypassed or disabled. If |kAlternativeAllowed| then an alternative proxy
  // configuration is allowed to be used. This alternative configuration would
  // replace the primary and fallback proxy configurations if enabled. Finally
  // if |kPromoAllowed|, the client may show a promotion for the data
  // reduction proxy.
  //
  // A standard configuration has a primary proxy, and a fallback proxy for
  // HTTP traffic. The alternative configuration has a different primary and
  // fallback proxy for HTTP traffic, and an SSL proxy.

  DataReductionProxyParams(int flags);

  virtual ~DataReductionProxyParams();

  // Returns the data reduction proxy primary origin.
  const GURL& origin() const {
    return origin_;
  }

  // Returns the data reduction proxy fallback origin.
  const GURL& fallback_origin() const {
    return fallback_origin_;
  }

  // Returns the data reduction proxy ssl origin that is used with the
  // alternative proxy configuration.
  const GURL& ssl_origin() const {
    return ssl_origin_;
  }

  // Returns the alternative data reduction proxy primary origin.
  const GURL& alt_origin() const {
    return alt_origin_;
  }

  // Returns the alternative data reduction proxy fallback origin.
  const GURL& alt_fallback_origin() const {
    return alt_fallback_origin_;
  }

  // Returns the URL to probe to decide if the primary origin should be used.
  const GURL& probe_url() const {
    return probe_url_;
  }

  // Set the proxy authentication key.
  void set_key(const std::string& key) {
    key_ = key;
  }

  // Returns the proxy authentication key.
  const std::string& key() const {
    return key_;
  }

  // Returns true if the data reduction proxy configuration may be used.
  bool allowed() const {
    return allowed_;
  }

  // Returns true if the fallback proxy may be used.
  bool fallback_allowed() const {
    return fallback_allowed_;
  }

  // Returns true if the alternative data reduction proxy configuration may be
  // used.
  bool alternative_allowed() const {
    return alt_allowed_;
  }

  // Returns true if the data reduction proxy promo may be shown.
  // This is idependent of whether the data reduction proxy is allowed.
  // TODO(bengr): maybe tie to whether proxy is allowed.
  bool promo_allowed() const {
    return promo_allowed_;
  }

  // Given |allowed_|, |fallback_allowed_|, and |alt_allowed_|, returns the
  // list of data reduction proxies that may be used.
  DataReductionProxyList GetAllowedProxies() const;

 protected:
  // Test constructor that optionally won't call Init();
  DataReductionProxyParams(int flags,
                           bool should_call_init);

  // Initialize the values of the proxies, probe URL, and key from command
  // line flags and preprocessor constants, and check that there are
  // corresponding definitions for the allowed configurations.
  bool Init(bool allowed, bool fallback_allowed, bool alt_allowed);

  // Initialize the values of the proxies, probe URL, and key from command
  // line flags and preprocessor constants.
  void InitWithoutChecks();

  // Returns the corresponding string from preprocessor constants if defined,
  // and an empty string otherwise.
  virtual std::string GetDefaultKey() const;
  virtual std::string GetDefaultDevOrigin() const;
  virtual std::string GetDefaultOrigin() const;
  virtual std::string GetDefaultFallbackOrigin() const;
  virtual std::string GetDefaultSSLOrigin() const;
  virtual std::string GetDefaultAltOrigin() const;
  virtual std::string GetDefaultAltFallbackOrigin() const;
  virtual std::string GetDefaultProbeURL() const;

 private:
  GURL origin_;
  GURL fallback_origin_;
  GURL ssl_origin_;
  GURL alt_origin_;
  GURL alt_fallback_origin_;
  GURL probe_url_;

  std::string key_;

  bool allowed_;
  const bool fallback_allowed_;
  bool alt_allowed_;
  const bool promo_allowed_;


  DISALLOW_COPY_AND_ASSIGN(DataReductionProxyParams);
};

}  // namespace data_reduction_proxy
#endif  // COMPONENTS_DATA_REDUCTION_PROXY_BROWSER_DATA_REDUCTION_PROXY_PARAMS_H_
