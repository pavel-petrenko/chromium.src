// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_POLICY_CERT_SERVICE_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_POLICY_CERT_SERVICE_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/policy/user_network_configuration_updater.h"
#include "components/browser_context_keyed_service/browser_context_keyed_service.h"

namespace chromeos {
class UserManager;
}

namespace net {
class X509Certificate;
typedef std::vector<scoped_refptr<X509Certificate> > CertificateList;
}

namespace policy {

class PolicyCertVerifier;

// This service is the counterpart of PolicyCertVerifier on the UI thread. It's
// responsible for pushing the current list of trust anchors to the CertVerifier
// and marking the profile's prefs if any of the trust anchors was used.
// Except for unit tests, PolicyCertVerifier should only be created through this
// class.
class PolicyCertService
    : public BrowserContextKeyedService,
      public UserNetworkConfigurationUpdater::WebTrustedCertsObserver {
 public:
  PolicyCertService(const std::string& user_id,
                    UserNetworkConfigurationUpdater* net_conf_updater,
                    chromeos::UserManager* user_manager);
  virtual ~PolicyCertService();

  // Creates an associated PolicyCertVerifier. The returned object must only be
  // used on the IO thread and must outlive this object.
  scoped_ptr<PolicyCertVerifier> CreatePolicyCertVerifier();

  // Returns true if the profile that owns this service has used certificates
  // installed via policy to establish a secure connection before. This means
  // that it may have cached content from an untrusted source.
  bool UsedPolicyCertificates() const;

  bool has_policy_certificates() const { return has_trust_anchors_; }

  // UserNetworkConfigurationUpdater::WebTrustedCertsObserver:
  virtual void OnTrustAnchorsChanged(const net::CertificateList& trust_anchors)
      OVERRIDE;

  // BrowserContextKeyedService:
  virtual void Shutdown() OVERRIDE;

  static scoped_ptr<PolicyCertService> CreateForTesting(
      const std::string& user_id,
      PolicyCertVerifier* verifier,
      chromeos::UserManager* user_manager);

 private:
  PolicyCertService(const std::string& user_id,
                    PolicyCertVerifier* verifier,
                    chromeos::UserManager* user_manager);

  PolicyCertVerifier* cert_verifier_;
  std::string user_id_;
  UserNetworkConfigurationUpdater* net_conf_updater_;
  chromeos::UserManager* user_manager_;
  bool has_trust_anchors_;

  // Weak pointers to handle callbacks from PolicyCertVerifier on the IO thread.
  // The factory and the created WeakPtrs must only be used on the UI thread.
  base::WeakPtrFactory<PolicyCertService> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PolicyCertService);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_POLICY_CERT_SERVICE_H_
