// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file includes code SSLClientSocketNSS::DoVerifyCertComplete() derived
// from AuthCertificateCallback() in
// mozilla/security/manager/ssl/src/nsNSSCallbacks.cpp.

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Netscape security libraries.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Ian McGreer <mcgreer@netscape.com>
 *   Javier Delgadillo <javi@netscape.com>
 *   Kai Engert <kengert@redhat.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "net/socket/ssl_client_socket_nss.h"

#include <certdb.h>
#include <hasht.h>
#include <keyhi.h>
#include <nspr.h>
#include <nss.h>
#include <ocsp.h>
#include <pk11pub.h>
#include <secerr.h>
#include <sechash.h>
#include <ssl.h>
#include <sslerr.h>
#include <sslproto.h>

#include <algorithm>
#include <limits>
#include <map>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/build_time.h"
#include "base/callback_helpers.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/metrics/histogram.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "base/stringprintf.h"
#include "base/thread_task_runner_handle.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "crypto/ec_private_key.h"
#include "crypto/rsa_private_key.h"
#include "crypto/scoped_nss_types.h"
#include "net/base/address_list.h"
#include "net/base/asn1_util.h"
#include "net/base/cert_status_flags.h"
#include "net/base/cert_verifier.h"
#include "net/base/connection_type_histograms.h"
#include "net/base/dns_util.h"
#include "net/base/dnssec_chain_verifier.h"
#include "net/base/transport_security_state.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/net_log.h"
#include "net/base/single_request_cert_verifier.h"
#include "net/base/ssl_cert_request_info.h"
#include "net/base/ssl_connection_status_flags.h"
#include "net/base/ssl_info.h"
#include "net/base/x509_certificate_net_log_param.h"
#include "net/ocsp/nss_ocsp.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/nss_ssl_util.h"
#include "net/socket/ssl_error_params.h"
#include "net/socket/ssl_host_info.h"

#if defined(OS_WIN)
#include <windows.h>
#include <wincrypt.h>
#elif defined(OS_MACOSX)
#include <Security/SecBase.h>
#include <Security/SecCertificate.h>
#include <Security/SecIdentity.h>
#include "base/mac/mac_logging.h"
#elif defined(USE_NSS)
#include <dlfcn.h>
#endif

static const int kRecvBufferSize = 4096;

#if defined(OS_WIN)
// CERT_OCSP_RESPONSE_PROP_ID is only implemented on Vista+, but it can be
// set on Windows XP without error. There is some overhead from the server
// sending the OCSP response if it supports the extension, for the subset of
// XP clients who will request it but be unable to use it, but this is an
// acceptable trade-off for simplicity of implementation.
static bool IsOCSPStaplingSupported() {
  return true;
}
#elif defined(USE_NSS)
typedef SECStatus
(*CacheOCSPResponseFromSideChannelFunction)(
    CERTCertDBHandle *handle, CERTCertificate *cert, PRTime time,
    SECItem *encodedResponse, void *pwArg);

// On Linux, we dynamically link against the system version of libnss3.so. In
// order to continue working on systems without up-to-date versions of NSS we
// lookup CERT_CacheOCSPResponseFromSideChannel with dlsym.

// RuntimeLibNSSFunctionPointers is a singleton which caches the results of any
// runtime symbol resolution that we need.
class RuntimeLibNSSFunctionPointers {
 public:
  CacheOCSPResponseFromSideChannelFunction
  GetCacheOCSPResponseFromSideChannelFunction() {
    return cache_ocsp_response_from_side_channel_;
  }

  static RuntimeLibNSSFunctionPointers* GetInstance() {
    return Singleton<RuntimeLibNSSFunctionPointers>::get();
  }

 private:
  friend struct DefaultSingletonTraits<RuntimeLibNSSFunctionPointers>;

  RuntimeLibNSSFunctionPointers() {
    cache_ocsp_response_from_side_channel_ =
        (CacheOCSPResponseFromSideChannelFunction)
        dlsym(RTLD_DEFAULT, "CERT_CacheOCSPResponseFromSideChannel");
  }

  CacheOCSPResponseFromSideChannelFunction
      cache_ocsp_response_from_side_channel_;
};

static CacheOCSPResponseFromSideChannelFunction
GetCacheOCSPResponseFromSideChannelFunction() {
  return RuntimeLibNSSFunctionPointers::GetInstance()
    ->GetCacheOCSPResponseFromSideChannelFunction();
}

static bool IsOCSPStaplingSupported() {
  return GetCacheOCSPResponseFromSideChannelFunction() != NULL;
}
#else
// TODO(agl): Figure out if we can plumb the OCSP response into Mac's system
// certificate validation functions.
static bool IsOCSPStaplingSupported() {
  return false;
}
#endif

namespace net {

// State machines are easier to debug if you log state transitions.
// Enable these if you want to see what's going on.
#if 1
#define EnterFunction(x)
#define LeaveFunction(x)
#define GotoState(s) next_handshake_state_ = s
#else
#define EnterFunction(x)\
    VLOG(1) << (void *)this << " " << __FUNCTION__ << " enter " << x\
            << "; next_handshake_state " << next_handshake_state_
#define LeaveFunction(x)\
    VLOG(1) << (void *)this << " " << __FUNCTION__ << " leave " << x\
            << "; next_handshake_state " << next_handshake_state_
#define GotoState(s)\
    do {\
      VLOG(1) << (void *)this << " " << __FUNCTION__ << " jump to state " << s;\
      next_handshake_state_ = s;\
    } while (0)
#endif

namespace {

#if defined(OS_WIN)

// This callback is intended to be used with CertFindChainInStore. In addition
// to filtering by extended/enhanced key usage, we do not show expired
// certificates and require digital signature usage in the key usage
// extension.
//
// This matches our behavior on Mac OS X and that of NSS. It also matches the
// default behavior of IE8. See http://support.microsoft.com/kb/890326 and
// http://blogs.msdn.com/b/askie/archive/2009/06/09/my-expired-client-certificates-no-longer-display-when-connecting-to-my-web-server-using-ie8.aspx
BOOL WINAPI ClientCertFindCallback(PCCERT_CONTEXT cert_context,
                                   void* find_arg) {
  VLOG(1) << "Calling ClientCertFindCallback from _nss";
  // Verify the certificate's KU is good.
  BYTE key_usage;
  if (CertGetIntendedKeyUsage(X509_ASN_ENCODING, cert_context->pCertInfo,
                              &key_usage, 1)) {
    if (!(key_usage & CERT_DIGITAL_SIGNATURE_KEY_USAGE))
      return FALSE;
  } else {
    DWORD err = GetLastError();
    // If |err| is non-zero, it's an actual error. Otherwise the extension
    // just isn't present, and we treat it as if everything was allowed.
    if (err) {
      DLOG(ERROR) << "CertGetIntendedKeyUsage failed: " << err;
      return FALSE;
    }
  }

  // Verify the current time is within the certificate's validity period.
  if (CertVerifyTimeValidity(NULL, cert_context->pCertInfo) != 0)
    return FALSE;

  // Verify private key metadata is associated with this certificate.
  DWORD size = 0;
  if (!CertGetCertificateContextProperty(
          cert_context, CERT_KEY_PROV_INFO_PROP_ID, NULL, &size)) {
    return FALSE;
  }

  return TRUE;
}

#endif

// DNSValidationResult enumerates the possible outcomes from processing a
// set of DNS records.
enum DNSValidationResult {
  DNSVR_SUCCESS,   // the cert is immediately acceptable.
  DNSVR_FAILURE,   // the cert is unconditionally rejected.
  DNSVR_CONTINUE,  // perform CA validation as usual.
};

// VerifyCAARecords processes DNSSEC validated RRDATA for a number of DNS CAA
// records and checks them against the given chain.
//    server_cert_nss: the server's leaf certificate.
//    rrdatas: the CAA records for the current domain.
//    port: the TCP port number that we connected to.
DNSValidationResult VerifyCAARecords(
    CERTCertificate* server_cert_nss,
    const std::vector<base::StringPiece>& rrdatas,
    uint16 port) {
  DnsCAARecord::Policy policy;
  const DnsCAARecord::ParseResult r = DnsCAARecord::Parse(rrdatas, &policy);
  if (r == DnsCAARecord::SYNTAX_ERROR || r == DnsCAARecord::UNKNOWN_CRITICAL)
    return DNSVR_FAILURE;
  if (r == DnsCAARecord::DISCARD)
    return DNSVR_CONTINUE;
  DCHECK(r == DnsCAARecord::SUCCESS);

  for (std::vector<DnsCAARecord::Policy::Hash>::const_iterator
       hash = policy.authorized_hashes.begin();
       hash != policy.authorized_hashes.end();
       ++hash) {
    if (hash->target == DnsCAARecord::Policy::SUBJECT_PUBLIC_KEY_INFO &&
        (hash->port == 0 || hash->port == port)) {
      CHECK_LE(hash->data.size(), static_cast<unsigned>(SHA512_LENGTH));
      uint8 calculated_hash[SHA512_LENGTH];  // SHA512 is the largest.
      SECStatus rv = HASH_HashBuf(
          static_cast<HASH_HashType>(hash->algorithm),
          calculated_hash,
          server_cert_nss->derPublicKey.data,
          server_cert_nss->derPublicKey.len);
      DCHECK(rv == SECSuccess);
      const std::string actual_digest(reinterpret_cast<char*>(calculated_hash),
                                      hash->data.size());

      // Note that the parser ensures that hash->data.size() is correct for the
      // given algorithm. An attacker cannot give a zero length hash that
      // always matches.
      if (actual_digest == hash->data) {
        // A DNSSEC secure hash over the public key of the leaf-certificate
        // is sufficient.
        return DNSVR_SUCCESS;
      }
    }
  }

  // If a CAA record was found, but nothing matched, then we reject the
  // certificate.
  return DNSVR_FAILURE;
}

// CheckDNSSECChain tries to validate a DNSSEC chain embedded in
// |server_cert_nss|. It returns true iff a chain is found that proves the
// value of a CAA record that contains a valid public key fingerprint.
// |port| contains the TCP port number that we connected to as CAA records can
// be specific to a given port.
DNSValidationResult CheckDNSSECChain(
    const std::string& hostname,
    CERTCertificate* server_cert_nss,
    uint16 port) {
  if (!server_cert_nss)
    return DNSVR_CONTINUE;

  // CERT_FindCertExtensionByOID isn't exported so we have to install an OID,
  // get a tag for it and find the extension by using that tag.
  static SECOidTag dnssec_chain_tag;
  static bool dnssec_chain_tag_valid;
  if (!dnssec_chain_tag_valid) {
    // It's harmless if multiple threads enter this block concurrently.
    static const uint8 kDNSSECChainOID[] =
        // 1.3.6.1.4.1.11129.2.1.4
        // (iso.org.dod.internet.private.enterprises.google.googleSecurity.
        //  certificateExtensions.dnssecEmbeddedChain)
        {0x2b, 0x06, 0x01, 0x04, 0x01, 0xd6, 0x79, 0x02, 0x01, 0x04};
    SECOidData oid_data;
    memset(&oid_data, 0, sizeof(oid_data));
    oid_data.oid.data = const_cast<uint8*>(kDNSSECChainOID);
    oid_data.oid.len = sizeof(kDNSSECChainOID);
    oid_data.desc = "DNSSEC chain";
    oid_data.supportedExtension = SUPPORTED_CERT_EXTENSION;
    dnssec_chain_tag = SECOID_AddEntry(&oid_data);
    DCHECK_NE(SEC_OID_UNKNOWN, dnssec_chain_tag);
    dnssec_chain_tag_valid = true;
  }

  SECItem dnssec_embedded_chain;
  SECStatus rv = CERT_FindCertExtension(server_cert_nss,
      dnssec_chain_tag, &dnssec_embedded_chain);
  if (rv != SECSuccess)
    return DNSVR_CONTINUE;

  base::StringPiece chain(
      reinterpret_cast<char*>(dnssec_embedded_chain.data),
      dnssec_embedded_chain.len);
  std::string dns_hostname;
  if (!DNSDomainFromDot(hostname, &dns_hostname))
    return DNSVR_CONTINUE;
  DNSSECChainVerifier verifier(dns_hostname, chain);
  DNSSECChainVerifier::Error err = verifier.Verify();
  if (err != DNSSECChainVerifier::OK) {
    LOG(ERROR) << "DNSSEC chain verification failed: " << err;
    return DNSVR_CONTINUE;
  }

  if (verifier.rrtype() != kDNS_CAA)
    return DNSVR_CONTINUE;

  DNSValidationResult r = VerifyCAARecords(
      server_cert_nss, verifier.rrdatas(), port);
  SECITEM_FreeItem(&dnssec_embedded_chain, PR_FALSE);

  return r;
}

bool DomainBoundCertNegotiated(PRFileDesc* socket) {
  // TODO(wtc,mattm): this is temporary while DBC support is changed into
  // Channel ID.
  return false;
}

void DestroyCertificates(CERTCertificate** certs, size_t len) {
  for (size_t i = 0; i < len; i++)
    CERT_DestroyCertificate(certs[i]);
}

// Helper functions to make it possible to log events from within the
// SSLClientSocketNSS::Core.
void AddLogEvent(BoundNetLog* net_log, NetLog::EventType event_type) {
  if (!net_log)
    return;
  net_log->AddEvent(event_type);
}

// Helper function to make it possible to log events from within the
// SSLClientSocketNSS::Core.
void AddLogEventWithCallback(BoundNetLog* net_log,
                             NetLog::EventType event_type,
                             const NetLog::ParametersCallback& callback) {
  if (!net_log)
    return;
  net_log->AddEvent(event_type, callback);
}

// Helper function to make it easier to call BoundNetLog::AddByteTransferEvent
// from within the SSLClientSocketNSS::Core.
// AddByteTransferEvent expects to receive a const char*, which within the
// Core is backed by an IOBuffer. If the "const char*" is bound via
// base::Bind and posted to another thread, and the IOBuffer that backs that
// pointer then goes out of scope on the origin thread, this would result in
// an invalid read of a stale pointer.
// Instead, provide a signature that accepts an IOBuffer*, so that a reference
// to the owning IOBuffer can be bound to the Callback. This ensures that the
// IOBuffer will stay alive long enough to cross threads if needed.
void LogByteTransferEvent(BoundNetLog* net_log, NetLog::EventType event_type,
                          int len, IOBuffer* buffer) {
  if (!net_log)
    return;
  net_log->AddByteTransferEvent(event_type, len, buffer->data());
}

// PeerCertificateChain is a helper object which extracts the certificate
// chain, as given by the server, from an NSS socket and performs the needed
// resource management. The first element of the chain is the leaf certificate
// and the other elements are in the order given by the server.
class PeerCertificateChain {
 public:
  PeerCertificateChain() {}
  PeerCertificateChain(const PeerCertificateChain& other);
  ~PeerCertificateChain();
  PeerCertificateChain& operator=(const PeerCertificateChain& other);

  // Resets the current chain, freeing any resources, and updates the current
  // chain to be a copy of the chain stored in |nss_fd|.
  // If |nss_fd| is NULL, then the current certificate chain will be freed.
  void Reset(PRFileDesc* nss_fd);

  // Returns the current certificate chain as a vector of DER-encoded
  // base::StringPieces. The returned vector remains valid until Reset is
  // called.
  std::vector<base::StringPiece> AsStringPieceVector() const;

  bool empty() const { return certs_.empty(); }
  size_t size() const { return certs_.size(); }

  CERTCertificate* operator[](size_t index) const {
    DCHECK_LT(index, certs_.size());
    return certs_[index];
  }

 private:
  std::vector<CERTCertificate*> certs_;
};

PeerCertificateChain::PeerCertificateChain(
    const PeerCertificateChain& other) {
  *this = other;
}

PeerCertificateChain::~PeerCertificateChain() {
  Reset(NULL);
}

PeerCertificateChain& PeerCertificateChain::operator=(
    const PeerCertificateChain& other) {
  if (this == &other)
    return *this;

  Reset(NULL);
  certs_.reserve(other.certs_.size());
  for (size_t i = 0; i < other.certs_.size(); ++i)
    certs_.push_back(CERT_DupCertificate(other.certs_[i]));

  return *this;
}

void PeerCertificateChain::Reset(PRFileDesc* nss_fd) {
  for (size_t i = 0; i < certs_.size(); ++i)
    CERT_DestroyCertificate(certs_[i]);
  certs_.clear();

  if (nss_fd == NULL)
    return;

  unsigned int num_certs = 0;
  SECStatus rv = SSL_PeerCertificateChain(nss_fd, NULL, &num_certs, 0);
  DCHECK_EQ(SECSuccess, rv);

  // The handshake on |nss_fd| may not have completed.
  if (num_certs == 0)
    return;

  certs_.resize(num_certs);
  const unsigned int expected_num_certs = num_certs;
  rv = SSL_PeerCertificateChain(nss_fd, vector_as_array(&certs_),
                                &num_certs, expected_num_certs);
  DCHECK_EQ(SECSuccess, rv);
  DCHECK_EQ(expected_num_certs, num_certs);
}

std::vector<base::StringPiece>
PeerCertificateChain::AsStringPieceVector() const {
  std::vector<base::StringPiece> v(certs_.size());
  for (unsigned i = 0; i < certs_.size(); i++) {
    v[i] = base::StringPiece(
        reinterpret_cast<const char*>(certs_[i]->derCert.data),
        certs_[i]->derCert.len);
  }

  return v;
}

// HandshakeState is a helper struct used to pass handshake state between
// the NSS task runner and the network task runner.
//
// It contains members that may be read or written on the NSS task runner,
// but which also need to be read from the network task runner. The NSS task
// runner will notify the network task runner whenever this state changes, so
// that the network task runner can safely make a copy, which avoids the need
// for locking.
struct HandshakeState {
  HandshakeState() { Reset(); }

  void Reset() {
    next_proto_status = SSLClientSocket::kNextProtoUnsupported;
    next_proto.clear();
    server_protos.clear();
    domain_bound_cert_type = CLIENT_CERT_INVALID_TYPE;
    client_certs.clear();
    server_cert_chain.Reset(NULL);
    server_cert = NULL;
    predicted_cert_chain_correct = false;
    resumed_handshake = false;
    ssl_connection_status = 0;
  }

  // Set to kNextProtoNegotiated if NPN was successfully negotiated, with the
  // negotiated protocol stored in |next_proto|.
  SSLClientSocket::NextProtoStatus next_proto_status;
  std::string next_proto;
  // If the server supports NPN, the protocols supported by the server.
  std::string server_protos;

  // The type of domain bound cert that was exchanged, or
  // CLIENT_CERT_INVALID_TYPE if no domain bound cert was negotiated or sent.
  SSLClientCertType domain_bound_cert_type;

  // If the peer requests client certificate authentication, the set of
  // certificates that matched the peer's criteria.
  CertificateList client_certs;

  // Set when the handshake fully completes.
  //
  // The server certificate is first received from NSS as an NSS certificate
  // chain (|server_cert_chain|) and then converted into a platform-specific
  // X509Certificate object (|server_cert|). It's possible for some
  // certificates to be successfully parsed by NSS, and not by the platform
  // libraries (i.e.: when running within a sandbox, different parsing
  // algorithms, etc), so it's not safe to assume that |server_cert| will
  // always be non-NULL.
  PeerCertificateChain server_cert_chain;
  scoped_refptr<X509Certificate> server_cert;

  // True if we predicted a certificate chain (via
  // Core::SetPredictedCertificates) and that prediction matched what the
  // server sent.
  bool predicted_cert_chain_correct;

  // True if the current handshake was the result of TLS session resumption.
  bool resumed_handshake;

  // The negotiated security parameters (TLS version, cipher, extensions) of
  // the SSL connection.
  int ssl_connection_status;
};

// Client-side error mapping functions.

// Map NSS error code to network error code.
int MapNSSClientError(PRErrorCode err) {
  switch (err) {
    case SSL_ERROR_BAD_CERT_ALERT:
    case SSL_ERROR_UNSUPPORTED_CERT_ALERT:
    case SSL_ERROR_REVOKED_CERT_ALERT:
    case SSL_ERROR_EXPIRED_CERT_ALERT:
    case SSL_ERROR_CERTIFICATE_UNKNOWN_ALERT:
    case SSL_ERROR_UNKNOWN_CA_ALERT:
    case SSL_ERROR_ACCESS_DENIED_ALERT:
      return ERR_BAD_SSL_CLIENT_AUTH_CERT;
    default:
      return MapNSSError(err);
  }
}

// Map NSS error code from the first SSL handshake to network error code.
int MapNSSClientHandshakeError(PRErrorCode err) {
  switch (err) {
    // If the server closed on us, it is a protocol error.
    // Some TLS-intolerant servers do this when we request TLS.
    case PR_END_OF_FILE_ERROR:
      return ERR_SSL_PROTOCOL_ERROR;
    default:
      return MapNSSClientError(err);
  }
}

}  // namespace

// SSLClientSocketNSS::Core provides a thread-safe, ref-counted core that is
// able to marshal data between NSS functions and an underlying transport
// socket.
//
// All public functions are meant to be called from the network task runner,
// and any callbacks supplied will be invoked there as well, provided that
// Detach() has not been called yet.
//
/////////////////////////////////////////////////////////////////////////////
//
// Threading within SSLClientSocketNSS and SSLClientSocketNSS::Core:
//
// Because NSS may block on either hardware or user input during operations
// such as signing, creating certificates, or locating private keys, the Core
// handles all of the interactions with the underlying NSS SSL socket, so
// that these blocking calls can be executed on a dedicated task runner.
//
// Note that the network task runner and the NSS task runner may be executing
// on the same thread. If that happens, then it's more performant to try to
// complete as much work as possible synchronously, even if it might block,
// rather than continually PostTask-ing to the same thread.
//
// Because NSS functions should only be called on the NSS task runner, while
// I/O resources should only be accessed on the network task runner, most
// public functions are implemented via three methods, each with different
// task runner affinities.
//
// In the single-threaded mode (where the network and NSS task runners run on
// the same thread), these are all attempted synchronously, while in the
// multi-threaded mode, message passing is used.
//
// 1) NSS Task Runner: Execute NSS function (DoPayloadRead, DoPayloadWrite,
//    DoHandshake)
// 2) NSS Task Runner: Prepare data to go from NSS to an IO function:
//    (BufferRecv, BufferSend)
// 3) Network Task Runner: Perform IO on that data (DoBufferRecv,
//    DoBufferSend, DoGetDomainBoundCert, OnGetDomainBoundCertComplete)
// 4) Both Task Runners: Callback for asynchronous completion or to marshal
//    data from the network task runner back to NSS (BufferRecvComplete,
//    BufferSendComplete, OnHandshakeIOComplete)
//
/////////////////////////////////////////////////////////////////////////////
// Single-threaded example
//
// |--------------------------Network Task Runner--------------------------|
//  SSLClientSocketNSS              Core               (Transport Socket)
//       Read()
//         |-------------------------V
//                                 Read()
//                                   |
//                            DoPayloadRead()
//                                   |
//                               BufferRecv()
//                                   |
//                              DoBufferRecv()
//                                   |-------------------------V
//                                                           Read()
//                                   V-------------------------|
//                          BufferRecvComplete()
//                                   |
//                           PostOrRunCallback()
//         V-------------------------|
//    (Read Callback)
//
/////////////////////////////////////////////////////////////////////////////
// Multi-threaded example:
//
// |--------------------Network Task Runner-------------|--NSS Task Runner--|
//  SSLClientSocketNSS          Core            Socket         Core
//       Read()
//         |---------------------V
//                             Read()
//                               |-------------------------------V
//                                                             Read()
//                                                               |
//                                                         DoPayloadRead()
//                                                               |
//                                                          BufferRecv
//                               V-------------------------------|
//                          DoBufferRecv
//                               |----------------V
//                                              Read()
//                               V----------------|
//                        BufferRecvComplete()
//                               |-------------------------------V
//                                                      BufferRecvComplete()
//                                                               |
//                                                       PostOrRunCallback()
//                               V-------------------------------|
//                        PostOrRunCallback()
//         V---------------------|
//    (Read Callback)
//
/////////////////////////////////////////////////////////////////////////////
class SSLClientSocketNSS::Core : public base::RefCountedThreadSafe<Core> {
 public:
  // Creates a new Core.
  //
  // Any calls to NSS are executed on the |nss_task_runner|, while any calls
  // that need to operate on the underlying transport, net log, or server
  // bound certificate fetching will happen on the |network_task_runner|, so
  // that their lifetimes match that of the owning SSLClientSocketNSS.
  //
  // The caller retains ownership of |transport|, |net_log|, and
  // |server_bound_cert_service|, and they will not be accessed once Detach()
  // has been called.
  Core(base::SequencedTaskRunner* network_task_runner,
       base::SingleThreadTaskRunner* nss_task_runner,
       ClientSocketHandle* transport,
       const HostPortPair& host_and_port,
       const SSLConfig& ssl_config,
       BoundNetLog* net_log,
       ServerBoundCertService* server_bound_cert_service);

  // Called on the network task runner.
  // Transfers ownership of |socket|, an NSS SSL socket, and |buffers|, the
  // underlying memio implementation, to the Core. Returns true if the Core
  // was successfully registered with the socket.
  bool Init(PRFileDesc* socket, memio_Private* buffers);

  // Called on the network task runner.
  // Sets the predicted certificate chain that the peer will send, for use
  // with the TLS CachedInfo extension. If called, it must not be called
  // before Init() or after Connect().
  void SetPredictedCertificates(
      const std::vector<std::string>& predicted_certificates);

  // Called on the network task runner.
  //
  // Attempts to perform an SSL handshake. If the handshake cannot be
  // completed synchronously, returns ERR_IO_PENDING, invoking |callback| on
  // the network task runner once the handshake has completed. Otherwise,
  // returns OK on success or a network error code on failure.
  int Connect(const CompletionCallback& callback);

  // Called on the network task runner.
  // Signals that the resources owned by the network task runner are going
  // away. No further callbacks will be invoked on the network task runner.
  // May be called at any time.
  void Detach();

  // Called on the network task runner.
  // Returns the current state of the underlying SSL socket. May be called at
  // any time.
  const HandshakeState& state() const { return network_handshake_state_; }

  // Called on the network task runner.
  // Read() and Write() mirror the net::Socket functions of the same name.
  // If ERR_IO_PENDING is returned, |callback| will be invoked on the network
  // task runner at a later point, unless the caller calls Detach().
  int Read(IOBuffer* buf, int buf_len, const CompletionCallback& callback);
  int Write(IOBuffer* buf, int buf_len, const CompletionCallback& callback);

 private:
  friend class base::RefCountedThreadSafe<Core>;
  ~Core();

  enum State {
    STATE_NONE,
    STATE_HANDSHAKE,
    STATE_GET_DOMAIN_BOUND_CERT_COMPLETE,
  };

  bool OnNSSTaskRunner() const;
  bool OnNetworkTaskRunner() const;

  ////////////////////////////////////////////////////////////////////////////
  // Methods that are ONLY called on the NSS task runner:
  ////////////////////////////////////////////////////////////////////////////

  // Called by NSS during full handshakes to allow the application to
  // verify the certificate. Instead of verifying the certificate in the midst
  // of the handshake, SECSuccess is always returned and the peer's certificate
  // is verified afterwards.
  // This behaviour is an artifact of the original SSLClientSocketWin
  // implementation, which could not verify the peer's certificate until after
  // the handshake had completed, as well as bugs in NSS that prevent
  // SSL_RestartHandshakeAfterCertReq from working.
  static SECStatus OwnAuthCertHandler(void* arg,
                                      PRFileDesc* socket,
                                      PRBool checksig,
                                      PRBool is_server);

  // Callbacks called by NSS when the peer requests client certificate
  // authentication.
  // See the documentation in third_party/nss/ssl/ssl.h for the meanings of
  // the arguments.
#if defined(NSS_PLATFORM_CLIENT_AUTH)
  // When NSS has been integrated with awareness of the underlying system
  // cryptographic libraries, this callback allows the caller to supply a
  // native platform certificate and key for use by NSS. At most, one of
  // either (result_certs, result_private_key) or (result_nss_certificate,
  // result_nss_private_key) should be set.
  // |arg| contains a pointer to the current SSLClientSocketNSS::Core.
  static SECStatus PlatformClientAuthHandler(
      void* arg,
      PRFileDesc* socket,
      CERTDistNames* ca_names,
      CERTCertList** result_certs,
      void** result_private_key,
      CERTCertificate** result_nss_certificate,
      SECKEYPrivateKey** result_nss_private_key);
#else
  static SECStatus ClientAuthHandler(void* arg,
                                     PRFileDesc* socket,
                                     CERTDistNames* ca_names,
                                     CERTCertificate** result_certificate,
                                     SECKEYPrivateKey** result_private_key);
#endif

  // Called by NSS once the handshake has completed.
  // |arg| contains a pointer to the current SSLClientSocketNSS::Core.
  static void HandshakeCallback(PRFileDesc* socket, void* arg);

  // Called by NSS if the peer supports the NPN handshake extension, to allow
  // the application to select the protocol to use.
  // See the documentation for SSLNextProtocolCallback in
  // third_party/nss/ssl/ssl.h for the meanings of the arguments.
  // |arg| contains a pointer to the current SSLClientSocketNSS::Core.
  static SECStatus NextProtoCallback(void* arg,
                                     PRFileDesc* fd,
                                     const unsigned char* protos,
                                     unsigned int protos_len,
                                     unsigned char* proto_out,
                                     unsigned int* proto_out_len,
                                     unsigned int proto_max_len);

  // Handles an NSS error generated while handshaking or performing IO.
  // Returns a network error code mapped from the original NSS error.
  int HandleNSSError(PRErrorCode error, bool handshake_error);

  int DoHandshakeLoop(int last_io_result);
  int DoReadLoop(int result);
  int DoWriteLoop(int result);

  int DoHandshake();
  int DoGetDBCertComplete(int result);

  int DoPayloadRead();
  int DoPayloadWrite();

  bool DoTransportIO();
  int BufferRecv();
  int BufferSend();

  void OnRecvComplete(int result);
  void OnSendComplete(int result);

  void DoConnectCallback(int result);
  void DoReadCallback(int result);
  void DoWriteCallback(int result);

  // Domain bound cert client auth handler.
  // Returns the value the ClientAuthHandler function should return.
  SECStatus DomainBoundClientAuthHandler(
      const SECItem* cert_types,
      CERTCertificate** result_certificate,
      SECKEYPrivateKey** result_private_key);

  // ImportDBCertAndKey is a helper function for turning a DER-encoded cert and
  // key into a CERTCertificate and SECKEYPrivateKey. Returns OK upon success
  // and an error code otherwise.
  // Requires |domain_bound_private_key_| and |domain_bound_cert_| to have been
  // set by a call to ServerBoundCertService->GetDomainBoundCert. The caller
  // takes ownership of the |*cert| and |*key|.
  int ImportDBCertAndKey(CERTCertificate** cert, SECKEYPrivateKey** key);

  // Updates the NSS and platform specific certificates.
  void UpdateServerCert();
  // Updates the nss_handshake_state_ with the negotiated security parameters.
  void UpdateConnectionStatus();
  // Record histograms for DBC support during full handshakes - resumed
  // handshakes are ignored.
  void RecordDomainBoundCertSupport() const;

  ////////////////////////////////////////////////////////////////////////////
  // Methods that are ONLY called on the network task runner:
  ////////////////////////////////////////////////////////////////////////////
  int DoBufferRecv(IOBuffer* buffer, int len);
  int DoBufferSend(IOBuffer* buffer, int len);
  int DoGetDomainBoundCert(const std::string& origin,
                           const std::vector<uint8>& requested_cert_types);

  void OnGetDomainBoundCertComplete(int result);
  void OnHandshakeStateUpdated(const HandshakeState& state);

  ////////////////////////////////////////////////////////////////////////////
  // Methods that are called on both the network task runner and the NSS
  // task runner.
  ////////////////////////////////////////////////////////////////////////////
  void OnHandshakeIOComplete(int result);
  void BufferRecvComplete(IOBuffer* buffer, int result);
  void BufferSendComplete(int result);

  // PostOrRunCallback is a helper function to ensure that |callback| is
  // invoked on the network task runner, but only if Detach() has not yet
  // been called.
  void PostOrRunCallback(const tracked_objects::Location& location,
                         const base::Closure& callback);

  // Uses PostOrRunCallback and |weak_net_log_| to try and log a
  // SSL_CLIENT_CERT_PROVIDED event, with the indicated count.
  void AddCertProvidedEvent(int cert_count);

  ////////////////////////////////////////////////////////////////////////////
  // Members that are ONLY accessed on the network task runner:
  ////////////////////////////////////////////////////////////////////////////

  // True if the owning SSLClientSocketNSS has called Detach(). No further
  // callbacks will be invoked nor access to members owned by the network
  // task runner.
  bool detached_;

  // The underlying transport to use for network IO.
  ClientSocketHandle* transport_;
  base::WeakPtrFactory<BoundNetLog> weak_net_log_factory_;

  // The current handshake state. Mirrors |nss_handshake_state_|.
  HandshakeState network_handshake_state_;

  ServerBoundCertService* server_bound_cert_service_;
  ServerBoundCertService::RequestHandle domain_bound_cert_request_handle_;

  ////////////////////////////////////////////////////////////////////////////
  // Members that are ONLY accessed on the NSS task runner:
  ////////////////////////////////////////////////////////////////////////////
  HostPortPair host_and_port_;
  SSLConfig ssl_config_;

  // NSS SSL socket.
  PRFileDesc* nss_fd_;

  // Buffers for the network end of the SSL state machine
  memio_Private* nss_bufs_;

  // The certificate chain, in DER form, that is expected to be received from
  // the server.
  std::vector<std::string> predicted_certs_;

  State next_handshake_state_;

  // True if domain bound certs were negotiated.
  bool domain_bound_cert_xtn_negotiated_;
  // True if the handshake state machine was interrupted for client auth.
  bool client_auth_cert_needed_;
  // True if NSS has called HandshakeCallback.
  bool handshake_callback_called_;

  HandshakeState nss_handshake_state_;

  bool transport_recv_busy_;
  bool transport_recv_eof_;
  bool transport_send_busy_;

  // Used by Read function.
  scoped_refptr<IOBuffer> user_read_buf_;
  int user_read_buf_len_;

  // Used by Write function.
  scoped_refptr<IOBuffer> user_write_buf_;
  int user_write_buf_len_;

  CompletionCallback user_connect_callback_;
  CompletionCallback user_read_callback_;
  CompletionCallback user_write_callback_;

  ////////////////////////////////////////////////////////////////////////////
  // Members that are accessed on both the network task runner and the NSS
  // task runner.
  ////////////////////////////////////////////////////////////////////////////
  scoped_refptr<base::SequencedTaskRunner> network_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> nss_task_runner_;

  // Dereferenced only on the network task runner, but bound to tasks destined
  // for the network task runner from the NSS task runner.
  base::WeakPtr<BoundNetLog> weak_net_log_;

  // Written on the network task runner by the |server_bound_cert_service_|,
  // prior to invoking OnHandshakeIOComplete.
  // Read on the NSS task runner when once OnHandshakeIOComplete is invoked
  // on the NSS task runner.
  SSLClientCertType domain_bound_cert_type_;
  std::string domain_bound_private_key_;
  std::string domain_bound_cert_;

  DISALLOW_COPY_AND_ASSIGN(Core);
};

SSLClientSocketNSS::Core::Core(
    base::SequencedTaskRunner* network_task_runner,
    base::SingleThreadTaskRunner* nss_task_runner,
    ClientSocketHandle* transport,
    const HostPortPair& host_and_port,
    const SSLConfig& ssl_config,
    BoundNetLog* net_log,
    ServerBoundCertService* server_bound_cert_service)
    : detached_(false),
      transport_(transport),
      weak_net_log_factory_(net_log),
      server_bound_cert_service_(server_bound_cert_service),
      domain_bound_cert_request_handle_(NULL),
      host_and_port_(host_and_port),
      ssl_config_(ssl_config),
      nss_fd_(NULL),
      nss_bufs_(NULL),
      next_handshake_state_(STATE_NONE),
      domain_bound_cert_xtn_negotiated_(false),
      client_auth_cert_needed_(false),
      handshake_callback_called_(false),
      transport_recv_busy_(false),
      transport_recv_eof_(false),
      transport_send_busy_(false),
      user_read_buf_len_(0),
      user_write_buf_len_(0),
      network_task_runner_(network_task_runner),
      nss_task_runner_(nss_task_runner),
      weak_net_log_(weak_net_log_factory_.GetWeakPtr()),
      domain_bound_cert_type_(CLIENT_CERT_INVALID_TYPE) {
}

SSLClientSocketNSS::Core::~Core() {
  // TODO(wtc): Send SSL close_notify alert.
  if (nss_fd_ != NULL) {
    PR_Close(nss_fd_);
    nss_fd_ = NULL;
  }
}

bool SSLClientSocketNSS::Core::Init(PRFileDesc* socket,
                                    memio_Private* buffers) {
  DCHECK(OnNetworkTaskRunner());
  DCHECK(!nss_fd_);
  DCHECK(!nss_bufs_);

  nss_fd_ = socket;
  nss_bufs_ = buffers;

  SECStatus rv = SECSuccess;

  if (!ssl_config_.next_protos.empty()) {
    rv = SSL_SetNextProtoCallback(
        nss_fd_, SSLClientSocketNSS::Core::NextProtoCallback, this);
    if (rv != SECSuccess)
      LogFailedNSSFunction(*weak_net_log_, "SSL_SetNextProtoCallback", "");
  }

  rv = SSL_AuthCertificateHook(
      nss_fd_, SSLClientSocketNSS::Core::OwnAuthCertHandler, this);
  if (rv != SECSuccess) {
    LogFailedNSSFunction(*weak_net_log_, "SSL_AuthCertificateHook", "");
    return false;
  }

#if defined(NSS_PLATFORM_CLIENT_AUTH)
  rv = SSL_GetPlatformClientAuthDataHook(
      nss_fd_, SSLClientSocketNSS::Core::PlatformClientAuthHandler,
      this);
#else
  rv = SSL_GetClientAuthDataHook(
      nss_fd_, SSLClientSocketNSS::Core::ClientAuthHandler, this);
#endif
  if (rv != SECSuccess) {
    LogFailedNSSFunction(*weak_net_log_, "SSL_GetClientAuthDataHook", "");
    return false;
  }

  rv = SSL_HandshakeCallback(
      nss_fd_, SSLClientSocketNSS::Core::HandshakeCallback, this);
  if (rv != SECSuccess) {
    LogFailedNSSFunction(*weak_net_log_, "SSL_HandshakeCallback", "");
    return false;
  }

  return true;
}

void SSLClientSocketNSS::Core::SetPredictedCertificates(
    const std::vector<std::string>& predicted_certs) {
  if (predicted_certs.empty())
    return;

  if (!OnNSSTaskRunner()) {
    DCHECK(!detached_);
    nss_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&Core::SetPredictedCertificates, this, predicted_certs));
    return;
  }

  DCHECK(nss_fd_);

  predicted_certs_ = predicted_certs;

  scoped_array<CERTCertificate*> certs(
      new CERTCertificate*[predicted_certs.size()]);

  for (size_t i = 0; i < predicted_certs.size(); i++) {
    SECItem derCert;
    derCert.data = const_cast<uint8*>(reinterpret_cast<const uint8*>(
        predicted_certs[i].data()));
    derCert.len = predicted_certs[i].size();
    certs[i] = CERT_NewTempCertificate(
        CERT_GetDefaultCertDB(), &derCert, NULL /* no nickname given */,
        PR_FALSE /* not permanent */, PR_TRUE /* copy DER data */);
    if (!certs[i]) {
      DestroyCertificates(&certs[0], i);
      NOTREACHED();
      return;
    }
  }

  SECStatus rv;
#ifdef SSL_ENABLE_CACHED_INFO
  rv = SSL_SetPredictedPeerCertificates(nss_fd_, certs.get(),
                                        predicted_certs.size());
  DCHECK_EQ(SECSuccess, rv);
#else
  rv = SECFailure;  // Not implemented.
#endif
  DestroyCertificates(&certs[0], predicted_certs.size());

  if (rv != SECSuccess) {
    LOG(WARNING) << "SetPredictedCertificates failed: "
                 << host_and_port_.ToString();
  }
}

int SSLClientSocketNSS::Core::Connect(const CompletionCallback& callback) {
  if (!OnNSSTaskRunner()) {
    DCHECK(!detached_);
    bool posted = nss_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(IgnoreResult(&Core::Connect), this, callback));
    return posted ? ERR_IO_PENDING : ERR_ABORTED;
  }

  DCHECK(OnNSSTaskRunner());
  DCHECK_EQ(STATE_NONE, next_handshake_state_);
  DCHECK(user_read_callback_.is_null());
  DCHECK(user_write_callback_.is_null());
  DCHECK(user_connect_callback_.is_null());
  DCHECK(!user_read_buf_);
  DCHECK(!user_write_buf_);

  next_handshake_state_ = STATE_HANDSHAKE;
  int rv = DoHandshakeLoop(OK);
  if (rv == ERR_IO_PENDING) {
    user_connect_callback_ = callback;
  } else if (rv > OK) {
    rv = OK;
  }
  if (rv != ERR_IO_PENDING && !OnNetworkTaskRunner()) {
    PostOrRunCallback(FROM_HERE, base::Bind(callback, rv));
    return ERR_IO_PENDING;
  }

  return rv;
}

void SSLClientSocketNSS::Core::Detach() {
  DCHECK(OnNetworkTaskRunner());

  detached_ = true;
  transport_ = NULL;
  weak_net_log_factory_.InvalidateWeakPtrs();

  network_handshake_state_.Reset();

  if (domain_bound_cert_request_handle_ != NULL) {
    server_bound_cert_service_->CancelRequest(
        domain_bound_cert_request_handle_);
    domain_bound_cert_request_handle_ = NULL;
  }
}

int SSLClientSocketNSS::Core::Read(IOBuffer* buf, int buf_len,
                                   const CompletionCallback& callback) {
  if (!OnNSSTaskRunner()) {
    DCHECK(OnNetworkTaskRunner());
    DCHECK(!detached_);
    DCHECK(transport_);

    bool posted = nss_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(IgnoreResult(&Core::Read), this, make_scoped_refptr(buf),
                   buf_len, callback));
    return posted ? ERR_IO_PENDING : ERR_ABORTED;
  }

  DCHECK(OnNSSTaskRunner());
  DCHECK(handshake_callback_called_);
  DCHECK_EQ(STATE_NONE, next_handshake_state_);
  DCHECK(user_read_callback_.is_null());
  DCHECK(user_connect_callback_.is_null());
  DCHECK(!user_read_buf_);
  DCHECK(nss_bufs_);

  user_read_buf_ = buf;
  user_read_buf_len_ = buf_len;

  int rv = DoReadLoop(OK);
  if (rv == ERR_IO_PENDING) {
    user_read_callback_ = callback;
  } else {
    user_read_buf_ = NULL;
    user_read_buf_len_ = 0;

    if (!OnNetworkTaskRunner()) {
      PostOrRunCallback(FROM_HERE, base::Bind(callback, rv));
      return ERR_IO_PENDING;
    }
  }

  return rv;
}

int SSLClientSocketNSS::Core::Write(IOBuffer* buf, int buf_len,
                                    const CompletionCallback& callback) {
  if (!OnNSSTaskRunner()) {
    DCHECK(OnNetworkTaskRunner());
    DCHECK(!detached_);
    DCHECK(transport_);

    bool posted = nss_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(IgnoreResult(&Core::Write), this, make_scoped_refptr(buf),
                   buf_len, callback));
    int rv = posted ? ERR_IO_PENDING : ERR_ABORTED;
    return rv;
  }

  DCHECK(OnNSSTaskRunner());
  DCHECK(handshake_callback_called_);
  DCHECK_EQ(STATE_NONE, next_handshake_state_);
  DCHECK(user_write_callback_.is_null());
  DCHECK(user_connect_callback_.is_null());
  DCHECK(!user_write_buf_);
  DCHECK(nss_bufs_);

  user_write_buf_ = buf;
  user_write_buf_len_ = buf_len;

  int rv = DoWriteLoop(OK);
  if (rv == ERR_IO_PENDING) {
    user_write_callback_ = callback;
  } else {
    user_write_buf_ = NULL;
    user_write_buf_len_ = 0;

    if (!OnNetworkTaskRunner()) {
      PostOrRunCallback(FROM_HERE, base::Bind(callback, rv));
      return ERR_IO_PENDING;
    }
  }

  return rv;
}

bool SSLClientSocketNSS::Core::OnNSSTaskRunner() const {
  return nss_task_runner_->RunsTasksOnCurrentThread();
}

bool SSLClientSocketNSS::Core::OnNetworkTaskRunner() const {
  return network_task_runner_->RunsTasksOnCurrentThread();
}

// static
SECStatus SSLClientSocketNSS::Core::OwnAuthCertHandler(
    void* arg,
    PRFileDesc* socket,
    PRBool checksig,
    PRBool is_server) {
#ifdef SSL_ENABLE_FALSE_START
  Core* core = reinterpret_cast<Core*>(arg);
  if (!core->handshake_callback_called_) {
    // Only need to turn off False Start in the initial handshake. Also, it is
    // unsafe to call SSL_OptionSet in a renegotiation because the "first
    // handshake" lock isn't already held, which will result in an assertion
    // failure in the ssl_Get1stHandshakeLock call in SSL_OptionSet.
    PRBool npn;
    SECStatus rv = SSL_HandshakeNegotiatedExtension(socket,
                                                    ssl_next_proto_nego_xtn,
                                                    &npn);
    if (rv != SECSuccess || !npn) {
      // If the server doesn't support NPN, then we don't do False Start with
      // it.
      SSL_OptionSet(socket, SSL_ENABLE_FALSE_START, PR_FALSE);
    }
  }
#endif

  // Tell NSS to not verify the certificate.
  return SECSuccess;
}

#if defined(NSS_PLATFORM_CLIENT_AUTH)
// static
SECStatus SSLClientSocketNSS::Core::PlatformClientAuthHandler(
    void* arg,
    PRFileDesc* socket,
    CERTDistNames* ca_names,
    CERTCertList** result_certs,
    void** result_private_key,
    CERTCertificate** result_nss_certificate,
    SECKEYPrivateKey** result_nss_private_key) {
  Core* core = reinterpret_cast<Core*>(arg);
  DCHECK(core->OnNSSTaskRunner());

  core->PostOrRunCallback(
      FROM_HERE,
      base::Bind(&AddLogEvent, core->weak_net_log_,
                 NetLog::TYPE_SSL_CLIENT_CERT_REQUESTED));

  const SECItem* cert_types = SSL_GetRequestedClientCertificateTypes(socket);

  // Check if a domain-bound certificate is requested.
  if (DomainBoundCertNegotiated(socket)) {
    return core->DomainBoundClientAuthHandler(cert_types,
                                              result_nss_certificate,
                                              result_nss_private_key);
  }

  core->client_auth_cert_needed_ = !core->ssl_config_.send_client_cert;
#if defined(OS_WIN)
  if (core->ssl_config_.send_client_cert) {
    if (core->ssl_config_.client_cert) {
      PCCERT_CONTEXT cert_context =
          core->ssl_config_.client_cert->os_cert_handle();

      HCRYPTPROV_OR_NCRYPT_KEY_HANDLE crypt_prov = 0;
      DWORD key_spec = 0;
      BOOL must_free = FALSE;
      BOOL acquired_key = CryptAcquireCertificatePrivateKey(
          cert_context, CRYPT_ACQUIRE_CACHE_FLAG, NULL,
          &crypt_prov, &key_spec, &must_free);

      if (acquired_key) {
        // Since we passed CRYPT_ACQUIRE_CACHE_FLAG, |must_free| must be false
        // according to the MSDN documentation.
        CHECK_EQ(must_free, FALSE);
        DCHECK_NE(key_spec, CERT_NCRYPT_KEY_SPEC);

        SECItem der_cert;
        der_cert.type = siDERCertBuffer;
        der_cert.data = cert_context->pbCertEncoded;
        der_cert.len  = cert_context->cbCertEncoded;

        // TODO(rsleevi): Error checking for NSS allocation errors.
        CERTCertDBHandle* db_handle = CERT_GetDefaultCertDB();
        CERTCertificate* user_cert = CERT_NewTempCertificate(
            db_handle, &der_cert, NULL, PR_FALSE, PR_TRUE);
        if (!user_cert) {
          // Importing the certificate can fail for reasons including a serial
          // number collision. See crbug.com/97355.
          core->AddCertProvidedEvent(0);
          return SECFailure;
        }
        CERTCertList* cert_chain = CERT_NewCertList();
        CERT_AddCertToListTail(cert_chain, user_cert);

        // Add the intermediates.
        X509Certificate::OSCertHandles intermediates =
            core->ssl_config_.client_cert->GetIntermediateCertificates();
        for (X509Certificate::OSCertHandles::const_iterator it =
            intermediates.begin(); it != intermediates.end(); ++it) {
          der_cert.data = (*it)->pbCertEncoded;
          der_cert.len = (*it)->cbCertEncoded;

          CERTCertificate* intermediate = CERT_NewTempCertificate(
              db_handle, &der_cert, NULL, PR_FALSE, PR_TRUE);
          if (!intermediate) {
            CERT_DestroyCertList(cert_chain);
            core->AddCertProvidedEvent(0);
            return SECFailure;
          }
          CERT_AddCertToListTail(cert_chain, intermediate);
        }
        PCERT_KEY_CONTEXT key_context = reinterpret_cast<PCERT_KEY_CONTEXT>(
            PORT_ZAlloc(sizeof(CERT_KEY_CONTEXT)));
        key_context->cbSize = sizeof(*key_context);
        // NSS will free this context when no longer in use, but the
        // |must_free| result from CryptAcquireCertificatePrivateKey was false
        // so we increment the refcount to negate NSS's future decrement.
        CryptContextAddRef(crypt_prov, NULL, 0);
        key_context->hCryptProv = crypt_prov;
        key_context->dwKeySpec = key_spec;
        *result_private_key = key_context;
        *result_certs = cert_chain;

        int cert_count = 1 + intermediates.size();
        core->AddCertProvidedEvent(cert_count);
        return SECSuccess;
      }
      LOG(WARNING) << "Client cert found without private key";
    }

    // Send no client certificate.
    core->AddCertProvidedEvent(0);
    return SECFailure;
  }

  core->nss_handshake_state_.client_certs.clear();

  std::vector<CERT_NAME_BLOB> issuer_list(ca_names->nnames);
  for (int i = 0; i < ca_names->nnames; ++i) {
    issuer_list[i].cbData = ca_names->names[i].len;
    issuer_list[i].pbData = ca_names->names[i].data;
  }

  // Client certificates of the user are in the "MY" system certificate store.
  HCERTSTORE my_cert_store = CertOpenSystemStore(NULL, L"MY");
  if (!my_cert_store) {
    PLOG(ERROR) << "Could not open the \"MY\" system certificate store";

    core->AddCertProvidedEvent(0);
    return SECFailure;
  }

  // Enumerate the client certificates.
  CERT_CHAIN_FIND_BY_ISSUER_PARA find_by_issuer_para;
  memset(&find_by_issuer_para, 0, sizeof(find_by_issuer_para));
  find_by_issuer_para.cbSize = sizeof(find_by_issuer_para);
  find_by_issuer_para.pszUsageIdentifier = szOID_PKIX_KP_CLIENT_AUTH;
  find_by_issuer_para.cIssuer = ca_names->nnames;
  find_by_issuer_para.rgIssuer = ca_names->nnames ? &issuer_list[0] : NULL;
  find_by_issuer_para.pfnFindCallback = ClientCertFindCallback;

  PCCERT_CHAIN_CONTEXT chain_context = NULL;
  DWORD find_flags = CERT_CHAIN_FIND_BY_ISSUER_CACHE_ONLY_FLAG |
                     CERT_CHAIN_FIND_BY_ISSUER_CACHE_ONLY_URL_FLAG;

  for (;;) {
    // Find a certificate chain.
    chain_context = CertFindChainInStore(my_cert_store,
                                         X509_ASN_ENCODING,
                                         find_flags,
                                         CERT_CHAIN_FIND_BY_ISSUER,
                                         &find_by_issuer_para,
                                         chain_context);
    if (!chain_context) {
      DWORD err = GetLastError();
      if (err != CRYPT_E_NOT_FOUND)
        DLOG(ERROR) << "CertFindChainInStore failed: " << err;
      break;
    }

    // Get the leaf certificate.
    PCCERT_CONTEXT cert_context =
        chain_context->rgpChain[0]->rgpElement[0]->pCertContext;
    // Create a copy the handle, so that we can close the "MY" certificate store
    // before returning from this function.
    PCCERT_CONTEXT cert_context2;
    BOOL ok = CertAddCertificateContextToStore(NULL, cert_context,
                                               CERT_STORE_ADD_USE_EXISTING,
                                               &cert_context2);
    if (!ok) {
      NOTREACHED();
      continue;
    }

    // Copy the rest of the chain. Copying the chain stops gracefully if an
    // error is encountered, with the partial chain being used as the
    // intermediates, as opposed to failing to consider the client certificate
    // at all.
    net::X509Certificate::OSCertHandles intermediates;
    for (DWORD i = 1; i < chain_context->rgpChain[0]->cElement; i++) {
      PCCERT_CONTEXT intermediate_copy;
      ok = CertAddCertificateContextToStore(
          NULL, chain_context->rgpChain[0]->rgpElement[i]->pCertContext,
          CERT_STORE_ADD_USE_EXISTING, &intermediate_copy);
      if (!ok) {
        NOTREACHED();
        break;
      }
      intermediates.push_back(intermediate_copy);
    }

    scoped_refptr<X509Certificate> cert = X509Certificate::CreateFromHandle(
        cert_context2, intermediates);
    core->nss_handshake_state_.client_certs.push_back(cert);

    X509Certificate::FreeOSCertHandle(cert_context2);
    for (net::X509Certificate::OSCertHandles::iterator it =
        intermediates.begin(); it != intermediates.end(); ++it) {
      net::X509Certificate::FreeOSCertHandle(*it);
    }
  }

  BOOL ok = CertCloseStore(my_cert_store, CERT_CLOSE_STORE_CHECK_FLAG);
  DCHECK(ok);

  // Update the network task runner's view of the handshake state now that
  // client certs have been detected.
  core->PostOrRunCallback(
      FROM_HERE, base::Bind(&Core::OnHandshakeStateUpdated, core,
                            core->nss_handshake_state_));

  // Tell NSS to suspend the client authentication.  We will then abort the
  // handshake by returning ERR_SSL_CLIENT_AUTH_CERT_NEEDED.
  return SECWouldBlock;
#elif defined(OS_MACOSX)
  if (core->ssl_config_.send_client_cert) {
    if (core->ssl_config_.client_cert) {
      OSStatus os_error = noErr;
      SecIdentityRef identity = NULL;
      SecKeyRef private_key = NULL;
      CFArrayRef chain =
          core->ssl_config_.client_cert->CreateClientCertificateChain();
      if (chain) {
        identity = reinterpret_cast<SecIdentityRef>(
            const_cast<void*>(CFArrayGetValueAtIndex(chain, 0)));
      }
      if (identity)
        os_error = SecIdentityCopyPrivateKey(identity, &private_key);

      if (chain && identity && os_error == noErr) {
        // TODO(rsleevi): Error checking for NSS allocation errors.
        *result_certs = CERT_NewCertList();
        *result_private_key = private_key;

        for (CFIndex i = 0; i < CFArrayGetCount(chain); ++i) {
          CSSM_DATA cert_data;
          SecCertificateRef cert_ref;
          if (i == 0) {
            cert_ref = core->ssl_config_.client_cert->os_cert_handle();
          } else {
            cert_ref = reinterpret_cast<SecCertificateRef>(
                const_cast<void*>(CFArrayGetValueAtIndex(chain, i)));
          }
          os_error = SecCertificateGetData(cert_ref, &cert_data);
          if (os_error != noErr)
            break;

          SECItem der_cert;
          der_cert.type = siDERCertBuffer;
          der_cert.data = cert_data.Data;
          der_cert.len = cert_data.Length;
          CERTCertificate* nss_cert = CERT_NewTempCertificate(
              CERT_GetDefaultCertDB(), &der_cert, NULL, PR_FALSE, PR_TRUE);
          if (!nss_cert) {
            // In the event of an NSS error we make up an OS error and reuse
            // the error handling, below.
            os_error = errSecCreateChainFailed;
            break;
          }
          CERT_AddCertToListTail(*result_certs, nss_cert);
        }
      }
      if (os_error == noErr) {
        int cert_count = 0;
        if (chain) {
          cert_count = CFArrayGetCount(chain);
          CFRelease(chain);
        }
        core->AddCertProvidedEvent(cert_count);
        return SECSuccess;
      }
      OSSTATUS_LOG(WARNING, os_error)
          << "Client cert found, but could not be used";
      if (*result_certs) {
        CERT_DestroyCertList(*result_certs);
        *result_certs = NULL;
      }
      if (*result_private_key)
        *result_private_key = NULL;
      if (private_key)
        CFRelease(private_key);
      if (chain)
        CFRelease(chain);
    }

    // Send no client certificate.
    core->AddCertProvidedEvent(0);
    return SECFailure;
  }

  core->nss_handshake_state_.client_certs.clear();

  // First, get the cert issuer names allowed by the server.
  std::vector<CertPrincipal> valid_issuers;
  int n = ca_names->nnames;
  for (int i = 0; i < n; i++) {
    // Parse each name into a CertPrincipal object.
    CertPrincipal p;
    if (p.ParseDistinguishedName(ca_names->names[i].data,
                                 ca_names->names[i].len)) {
      valid_issuers.push_back(p);
    }
  }

  // Now get the available client certs whose issuers are allowed by the server.
  X509Certificate::GetSSLClientCertificates(
      core->host_and_port_.host(), valid_issuers,
      &core->nss_handshake_state_.client_certs);

  // Update the network task runner's view of the handshake state now that
  // client certs have been detected.
  core->PostOrRunCallback(
      FROM_HERE, base::Bind(&Core::OnHandshakeStateUpdated, core,
                            core->nss_handshake_state_));

  // Tell NSS to suspend the client authentication.  We will then abort the
  // handshake by returning ERR_SSL_CLIENT_AUTH_CERT_NEEDED.
  return SECWouldBlock;
#else
  return SECFailure;
#endif
}

#else  // NSS_PLATFORM_CLIENT_AUTH

// static
// Based on Mozilla's NSS_GetClientAuthData.
SECStatus SSLClientSocketNSS::Core::ClientAuthHandler(
    void* arg,
    PRFileDesc* socket,
    CERTDistNames* ca_names,
    CERTCertificate** result_certificate,
    SECKEYPrivateKey** result_private_key) {
  Core* core = reinterpret_cast<Core*>(arg);
  DCHECK(core->OnNSSTaskRunner());

  core->PostOrRunCallback(
      FROM_HERE,
      base::Bind(&AddLogEvent, core->weak_net_log_,
                 NetLog::TYPE_SSL_CLIENT_CERT_REQUESTED));

  const SECItem* cert_types = SSL_GetRequestedClientCertificateTypes(socket);

  // Check if a domain-bound certificate is requested.
  if (DomainBoundCertNegotiated(socket)) {
    return core->DomainBoundClientAuthHandler(
        cert_types, result_certificate, result_private_key);
  }

  // Regular client certificate requested.
  core->client_auth_cert_needed_ = !core->ssl_config_.send_client_cert;
  void* wincx  = SSL_RevealPinArg(socket);

  // Second pass: a client certificate should have been selected.
  if (core->ssl_config_.send_client_cert) {
    if (core->ssl_config_.client_cert) {
      CERTCertificate* cert = CERT_DupCertificate(
          core->ssl_config_.client_cert->os_cert_handle());
      SECKEYPrivateKey* privkey = PK11_FindKeyByAnyCert(cert, wincx);
      if (privkey) {
        // TODO(jsorianopastor): We should wait for server certificate
        // verification before sending our credentials.  See
        // http://crbug.com/13934.
        *result_certificate = cert;
        *result_private_key = privkey;
        // A cert_count of -1 means the number of certificates is unknown.
        // NSS will construct the certificate chain.
        core->AddCertProvidedEvent(-1);

        return SECSuccess;
      }
      LOG(WARNING) << "Client cert found without private key";
    }
    // Send no client certificate.
    core->AddCertProvidedEvent(0);
    return SECFailure;
  }

  core->nss_handshake_state_.client_certs.clear();

  // Iterate over all client certificates.
  CERTCertList* client_certs = CERT_FindUserCertsByUsage(
      CERT_GetDefaultCertDB(), certUsageSSLClient,
      PR_FALSE, PR_FALSE, wincx);
  if (client_certs) {
    for (CERTCertListNode* node = CERT_LIST_HEAD(client_certs);
         !CERT_LIST_END(node, client_certs);
         node = CERT_LIST_NEXT(node)) {
      // Only offer unexpired certificates.
      if (CERT_CheckCertValidTimes(node->cert, PR_Now(), PR_TRUE) !=
          secCertTimeValid) {
        continue;
      }
      // Filter by issuer.
      //
      // TODO(davidben): This does a binary comparison of the DER-encoded
      // issuers. We should match according to RFC 5280 sec. 7.1. We should find
      // an appropriate NSS function or add one if needbe.
      if (ca_names->nnames &&
          NSS_CmpCertChainWCANames(node->cert, ca_names) != SECSuccess) {
        continue;
      }

      X509Certificate* x509_cert = X509Certificate::CreateFromHandle(
          node->cert, net::X509Certificate::OSCertHandles());
      core->nss_handshake_state_.client_certs.push_back(x509_cert);
    }
    CERT_DestroyCertList(client_certs);
  }

  // Update the network task runner's view of the handshake state now that
  // client certs have been detected.
  core->PostOrRunCallback(
      FROM_HERE, base::Bind(&Core::OnHandshakeStateUpdated, core,
                            core->nss_handshake_state_));

  // Tell NSS to suspend the client authentication.  We will then abort the
  // handshake by returning ERR_SSL_CLIENT_AUTH_CERT_NEEDED.
  return SECWouldBlock;
}
#endif  // NSS_PLATFORM_CLIENT_AUTH

// static
void SSLClientSocketNSS::Core::HandshakeCallback(
    PRFileDesc* socket,
    void* arg) {
  Core* core = reinterpret_cast<Core*>(arg);
  DCHECK(core->OnNSSTaskRunner());

  core->handshake_callback_called_ = true;

  HandshakeState* nss_state = &core->nss_handshake_state_;

  PRBool last_handshake_resumed;
  SECStatus rv = SSL_HandshakeResumedSession(socket, &last_handshake_resumed);
  if (rv == SECSuccess && last_handshake_resumed) {
    nss_state->resumed_handshake = true;
  } else {
    nss_state->resumed_handshake = false;
  }

  core->RecordDomainBoundCertSupport();
  core->UpdateServerCert();
  core->UpdateConnectionStatus();

  // We need to see if the predicted certificate chain (from
  // SetPredictedCertificates) matches the actual certificate chain.
  nss_state->predicted_cert_chain_correct = false;
  if (!core->predicted_certs_.empty()) {
    PeerCertificateChain& certs = nss_state->server_cert_chain;
    nss_state->predicted_cert_chain_correct =
        certs.size() == core->predicted_certs_.size();

    if (nss_state->predicted_cert_chain_correct) {
      for (unsigned i = 0; i < certs.size(); i++) {
        if (certs[i]->derCert.len != core->predicted_certs_[i].size() ||
            memcmp(certs[i]->derCert.data, core->predicted_certs_[i].data(),
                   certs[i]->derCert.len) != 0) {
          nss_state->predicted_cert_chain_correct = false;
          break;
        }
      }
    }
  }

  // Update the network task runners view of the handshake state whenever
  // a handshake has completed.
  core->PostOrRunCallback(
      FROM_HERE, base::Bind(&Core::OnHandshakeStateUpdated, core,
                            *nss_state));
}

// static
SECStatus SSLClientSocketNSS::Core::NextProtoCallback(
    void* arg,
    PRFileDesc* nss_fd,
    const unsigned char* protos,
    unsigned int protos_len,
    unsigned char* proto_out,
    unsigned int* proto_out_len,
    unsigned int proto_max_len) {
  Core* core = reinterpret_cast<Core*>(arg);
  DCHECK(core->OnNSSTaskRunner());

  HandshakeState* nss_state = &core->nss_handshake_state_;

  // For each protocol in server preference, see if we support it.
  for (unsigned int i = 0; i < protos_len; ) {
    const size_t len = protos[i];
    for (std::vector<std::string>::const_iterator
         j = core->ssl_config_.next_protos.begin();
         j != core->ssl_config_.next_protos.end(); j++) {
      // Having very long elements in the |next_protos| vector isn't a disaster
      // because they'll never be selected, but it does indicate an error
      // somewhere.
      DCHECK_LT(j->size(), 256u);

      if (j->size() == len &&
          memcmp(&protos[i + 1], j->data(), len) == 0) {
        nss_state->next_proto_status = kNextProtoNegotiated;
        nss_state->next_proto = *j;
        break;
      }
    }

    if (nss_state->next_proto_status == kNextProtoNegotiated)
      break;

    // NSS ensures that the data in |protos| is well formed, so this will not
    // cause a jump past the end of the buffer.
    i += len + 1;
  }

  nss_state->server_protos.assign(
      reinterpret_cast<const char*>(protos), protos_len);

  // If we didn't find a protocol, we select the first one from our list.
  if (nss_state->next_proto_status != kNextProtoNegotiated) {
    nss_state->next_proto_status = kNextProtoNoOverlap;
    nss_state->next_proto = core->ssl_config_.next_protos[0];
  }

  if (nss_state->next_proto.size() > proto_max_len) {
    PORT_SetError(SEC_ERROR_OUTPUT_LEN);
    return SECFailure;
  }
  memcpy(proto_out, nss_state->next_proto.data(),
         nss_state->next_proto.size());
  *proto_out_len = nss_state->next_proto.size();

  // Update the network task runner's view of the handshake state now that
  // NPN negotiation has occurred.
  core->PostOrRunCallback(
      FROM_HERE, base::Bind(&Core::OnHandshakeStateUpdated, core,
                            *nss_state));

  return SECSuccess;
}

int SSLClientSocketNSS::Core::HandleNSSError(PRErrorCode nss_error,
                                             bool handshake_error) {
  DCHECK(OnNSSTaskRunner());

  int net_error = handshake_error ? MapNSSClientHandshakeError(nss_error) :
                                    MapNSSClientError(nss_error);

#if defined(OS_WIN)
  // On Windows, a handle to the HCRYPTPROV is cached in the X509Certificate
  // os_cert_handle() as an optimization. However, if the certificate
  // private key is stored on a smart card, and the smart card is removed,
  // the cached HCRYPTPROV will not be able to obtain the HCRYPTKEY again,
  // preventing client certificate authentication. Because the
  // X509Certificate may outlive the individual SSLClientSocketNSS, due to
  // caching in X509Certificate, this failure ends up preventing client
  // certificate authentication with the same certificate for all future
  // attempts, even after the smart card has been re-inserted. By setting
  // the CERT_KEY_PROV_HANDLE_PROP_ID to NULL, the cached HCRYPTPROV will
  // typically be freed. This allows a new HCRYPTPROV to be obtained from
  // the certificate on the next attempt, which should succeed if the smart
  // card has been re-inserted, or will typically prompt the user to
  // re-insert the smart card if not.
  if ((net_error == ERR_SSL_CLIENT_AUTH_CERT_NO_PRIVATE_KEY ||
       net_error == ERR_SSL_CLIENT_AUTH_SIGNATURE_FAILED) &&
      ssl_config_.send_client_cert && ssl_config_.client_cert) {
    CertSetCertificateContextProperty(
        ssl_config_.client_cert->os_cert_handle(),
        CERT_KEY_PROV_HANDLE_PROP_ID, 0, NULL);
  }
#endif

  return net_error;
}

int SSLClientSocketNSS::Core::DoHandshakeLoop(int last_io_result) {
  DCHECK(OnNSSTaskRunner());

  int rv = last_io_result;
  do {
    // Default to STATE_NONE for next state.
    State state = next_handshake_state_;
    GotoState(STATE_NONE);

    switch (state) {
      case STATE_HANDSHAKE:
        rv = DoHandshake();
        break;
      case STATE_GET_DOMAIN_BOUND_CERT_COMPLETE:
        rv = DoGetDBCertComplete(rv);
        break;
      case STATE_NONE:
      default:
        rv = ERR_UNEXPECTED;
        LOG(DFATAL) << "unexpected state " << state;
        break;
    }

    // Do the actual network I/O
    bool network_moved = DoTransportIO();
    if (network_moved && next_handshake_state_ == STATE_HANDSHAKE) {
      // In general we exit the loop if rv is ERR_IO_PENDING.  In this
      // special case we keep looping even if rv is ERR_IO_PENDING because
      // the transport IO may allow DoHandshake to make progress.
      DCHECK(rv == OK || rv == ERR_IO_PENDING);
      rv = OK;  // This causes us to stay in the loop.
    }
  } while (rv != ERR_IO_PENDING && next_handshake_state_ != STATE_NONE);
  return rv;
}

int SSLClientSocketNSS::Core::DoReadLoop(int result) {
  DCHECK(OnNSSTaskRunner());
  DCHECK(handshake_callback_called_);
  DCHECK_EQ(STATE_NONE, next_handshake_state_);

  if (result < 0)
    return result;

  if (!nss_bufs_) {
    LOG(DFATAL) << "!nss_bufs_";
    int rv = ERR_UNEXPECTED;
    PostOrRunCallback(
        FROM_HERE,
        base::Bind(&AddLogEventWithCallback, weak_net_log_,
                   NetLog::TYPE_SSL_READ_ERROR,
                   CreateNetLogSSLErrorCallback(rv, 0)));
    return rv;
  }

  bool network_moved;
  int rv;
  do {
    rv = DoPayloadRead();
    network_moved = DoTransportIO();
  } while (rv == ERR_IO_PENDING && network_moved);

  return rv;
}

int SSLClientSocketNSS::Core::DoWriteLoop(int result) {
  DCHECK(OnNSSTaskRunner());
  DCHECK(handshake_callback_called_);
  DCHECK_EQ(STATE_NONE, next_handshake_state_);

  if (result < 0)
    return result;

  if (!nss_bufs_) {
    LOG(DFATAL) << "!nss_bufs_";
    int rv = ERR_UNEXPECTED;
    PostOrRunCallback(
        FROM_HERE,
        base::Bind(&AddLogEventWithCallback, weak_net_log_,
                   NetLog::TYPE_SSL_READ_ERROR,
                   CreateNetLogSSLErrorCallback(rv, 0)));
    return rv;
  }

  bool network_moved;
  int rv;
  do {
    rv = DoPayloadWrite();
    network_moved = DoTransportIO();
  } while (rv == ERR_IO_PENDING && network_moved);

  LeaveFunction(rv);
  return rv;
}

int SSLClientSocketNSS::Core::DoHandshake() {
  DCHECK(OnNSSTaskRunner());

  int net_error = net::OK;
  SECStatus rv = SSL_ForceHandshake(nss_fd_);

  // TODO(rkn): Handle the case in which server-bound cert generation takes
  // too long and the server has closed the connection. Report some new error
  // code so that the higher level code will attempt to delete the socket and
  // redo the handshake.
  if (client_auth_cert_needed_) {
    if (domain_bound_cert_xtn_negotiated_) {
      GotoState(STATE_GET_DOMAIN_BOUND_CERT_COMPLETE);
      net_error = ERR_IO_PENDING;
    } else {
      net_error = ERR_SSL_CLIENT_AUTH_CERT_NEEDED;
      PostOrRunCallback(
          FROM_HERE,
          base::Bind(&AddLogEventWithCallback, weak_net_log_,
                     NetLog::TYPE_SSL_HANDSHAKE_ERROR,
                     CreateNetLogSSLErrorCallback(net_error, 0)));

      // If the handshake already succeeded (because the server requests but
      // doesn't require a client cert), we need to invalidate the SSL session
      // so that we won't try to resume the non-client-authenticated session in
      // the next handshake.  This will cause the server to ask for a client
      // cert again.
      if (rv == SECSuccess && SSL_InvalidateSession(nss_fd_) != SECSuccess)
        LOG(WARNING) << "Couldn't invalidate SSL session: " << PR_GetError();
    }
  } else if (rv == SECSuccess) {
    if (!handshake_callback_called_) {
      // Workaround for https://bugzilla.mozilla.org/show_bug.cgi?id=562434 -
      // SSL_ForceHandshake returned SECSuccess prematurely.
      rv = SECFailure;
      net_error = ERR_SSL_PROTOCOL_ERROR;
      PostOrRunCallback(
          FROM_HERE,
          base::Bind(&AddLogEventWithCallback, weak_net_log_,
                     NetLog::TYPE_SSL_HANDSHAKE_ERROR,
                     CreateNetLogSSLErrorCallback(net_error, 0)));
    } else {
  #if defined(SSL_ENABLE_OCSP_STAPLING)
      // TODO(agl): figure out how to plumb an OCSP response into the Mac
      // system library and update IsOCSPStaplingSupported for Mac.
      if (!nss_handshake_state_.predicted_cert_chain_correct &&
          IsOCSPStaplingSupported()) {
        unsigned int len = 0;
        SSL_GetStapledOCSPResponse(nss_fd_, NULL, &len);
        if (len) {
          const unsigned int orig_len = len;
          scoped_array<uint8> ocsp_response(new uint8[orig_len]);
          SSL_GetStapledOCSPResponse(nss_fd_, ocsp_response.get(), &len);
          DCHECK_EQ(orig_len, len);

  #if defined(OS_WIN)
          if (nss_handshake_state_.server_cert) {
            CRYPT_DATA_BLOB ocsp_response_blob;
            ocsp_response_blob.cbData = len;
            ocsp_response_blob.pbData = ocsp_response.get();
            BOOL ok = CertSetCertificateContextProperty(
                nss_handshake_state_.server_cert->os_cert_handle(),
                CERT_OCSP_RESPONSE_PROP_ID,
                CERT_SET_PROPERTY_IGNORE_PERSIST_ERROR_FLAG,
                &ocsp_response_blob);
            if (!ok) {
              VLOG(1) << "Failed to set OCSP response property: "
                      << GetLastError();
            }
          }
  #elif defined(USE_NSS)
          CacheOCSPResponseFromSideChannelFunction cache_ocsp_response =
              GetCacheOCSPResponseFromSideChannelFunction();
          SECItem ocsp_response_item;
          ocsp_response_item.type = siBuffer;
          ocsp_response_item.data = ocsp_response.get();
          ocsp_response_item.len = len;

          cache_ocsp_response(
              CERT_GetDefaultCertDB(),
              nss_handshake_state_.server_cert_chain[0], PR_Now(),
              &ocsp_response_item, NULL);
  #endif
        }
      }
  #endif
    }
    // Done!
  } else {
    PRErrorCode prerr = PR_GetError();
    net_error = HandleNSSError(prerr, true);

    // If not done, stay in this state
    if (net_error == ERR_IO_PENDING) {
      GotoState(STATE_HANDSHAKE);
    } else {
      PostOrRunCallback(
          FROM_HERE,
          base::Bind(&AddLogEventWithCallback, weak_net_log_,
                     NetLog::TYPE_SSL_HANDSHAKE_ERROR,
                     CreateNetLogSSLErrorCallback(net_error, prerr)));
    }
  }

  return net_error;
}

int SSLClientSocketNSS::Core::DoGetDBCertComplete(int result) {
  SECStatus rv;
  PostOrRunCallback(
      FROM_HERE,
      base::Bind(&BoundNetLog::EndEventWithNetErrorCode, weak_net_log_,
                 NetLog::TYPE_SSL_GET_DOMAIN_BOUND_CERT, result));

  client_auth_cert_needed_ = false;
  domain_bound_cert_type_ = CLIENT_CERT_INVALID_TYPE;

  if (result != OK) {
    // Failed to get a DBC.  Proceed without.
    rv = SSL_RestartHandshakeAfterCertReq(nss_fd_, NULL, NULL, NULL);
    if (rv != SECSuccess)
      return MapNSSError(PORT_GetError());

    GotoState(STATE_HANDSHAKE);
    return OK;
  }

  CERTCertificate* cert;
  SECKEYPrivateKey* key;
  int error = ImportDBCertAndKey(&cert, &key);
  if (error != OK)
    return error;

  CERTCertificateList* cert_chain =
      CERT_CertChainFromCert(cert, certUsageSSLClient, PR_FALSE);

  AddCertProvidedEvent(cert_chain->len);

  rv = SSL_RestartHandshakeAfterCertReq(nss_fd_, cert, key, cert_chain);
  if (rv != SECSuccess)
    return MapNSSError(PORT_GetError());

  GotoState(STATE_HANDSHAKE);
  return OK;
}

int SSLClientSocketNSS::Core::DoPayloadRead() {
  DCHECK(OnNSSTaskRunner());
  DCHECK(user_read_buf_);
  DCHECK_GT(user_read_buf_len_, 0);

  int rv = PR_Read(nss_fd_, user_read_buf_->data(), user_read_buf_len_);
  if (client_auth_cert_needed_) {
    // We don't need to invalidate the non-client-authenticated SSL session
    // because the server will renegotiate anyway.
    rv = ERR_SSL_CLIENT_AUTH_CERT_NEEDED;
    PostOrRunCallback(
        FROM_HERE,
        base::Bind(&AddLogEventWithCallback, weak_net_log_,
                   NetLog::TYPE_SSL_READ_ERROR,
                   CreateNetLogSSLErrorCallback(rv, 0)));
    return rv;
  }
  if (rv >= 0) {
    PostOrRunCallback(
        FROM_HERE,
        base::Bind(&LogByteTransferEvent, weak_net_log_,
                   NetLog::TYPE_SSL_SOCKET_BYTES_RECEIVED, rv,
                   scoped_refptr<IOBuffer>(user_read_buf_)));
    return rv;
  }
  PRErrorCode prerr = PR_GetError();
  if (prerr == PR_WOULD_BLOCK_ERROR)
    return ERR_IO_PENDING;

  rv = HandleNSSError(prerr, false);
  PostOrRunCallback(
      FROM_HERE,
      base::Bind(&AddLogEventWithCallback, weak_net_log_,
                 NetLog::TYPE_SSL_READ_ERROR,
                 CreateNetLogSSLErrorCallback(rv, prerr)));
  return rv;
}

int SSLClientSocketNSS::Core::DoPayloadWrite() {
  DCHECK(OnNSSTaskRunner());

  DCHECK(user_write_buf_);

  int rv = PR_Write(nss_fd_, user_write_buf_->data(), user_write_buf_len_);
  if (rv >= 0) {
    PostOrRunCallback(
        FROM_HERE,
        base::Bind(&LogByteTransferEvent, weak_net_log_,
                   NetLog::TYPE_SSL_SOCKET_BYTES_SENT, rv,
                   scoped_refptr<IOBuffer>(user_write_buf_)));
    return rv;
  }
  PRErrorCode prerr = PR_GetError();
  if (prerr == PR_WOULD_BLOCK_ERROR)
    return ERR_IO_PENDING;

  rv = HandleNSSError(prerr, false);
  PostOrRunCallback(
      FROM_HERE,
      base::Bind(&AddLogEventWithCallback, weak_net_log_,
                 NetLog::TYPE_SSL_WRITE_ERROR,
                 CreateNetLogSSLErrorCallback(rv, prerr)));
  return rv;
}

// Do as much network I/O as possible between the buffer and the
// transport socket. Return true if some I/O performed, false
// otherwise (error or ERR_IO_PENDING).
bool SSLClientSocketNSS::Core::DoTransportIO() {
  DCHECK(OnNSSTaskRunner());

  bool network_moved = false;
  if (nss_bufs_ != NULL) {
    int rv;
    // Read and write as much data as we can. The loop is neccessary
    // because Write() may return synchronously.
    do {
      rv = BufferSend();
      if (rv > 0)
        network_moved = true;
    } while (rv > 0);
    if (!transport_recv_eof_ && BufferRecv() >= 0)
      network_moved = true;
  }
  return network_moved;
}

int SSLClientSocketNSS::Core::BufferRecv() {
  DCHECK(OnNSSTaskRunner());

  if (transport_recv_busy_)
    return ERR_IO_PENDING;

  char* buf;
  int nb = memio_GetReadParams(nss_bufs_, &buf);
  int rv;
  if (!nb) {
    // buffer too full to read into, so no I/O possible at moment
    rv = ERR_IO_PENDING;
  } else {
    scoped_refptr<IOBuffer> read_buffer(new IOBuffer(nb));
    if (OnNetworkTaskRunner()) {
      rv = DoBufferRecv(read_buffer, nb);
    } else {
      bool posted = network_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(IgnoreResult(&Core::DoBufferRecv), this, read_buffer,
                     nb));
      rv = posted ? ERR_IO_PENDING : ERR_ABORTED;
    }

    if (rv == ERR_IO_PENDING) {
      transport_recv_busy_ = true;
    } else {
      if (rv > 0) {
        memcpy(buf, read_buffer->data(), rv);
      } else if (rv == 0) {
        transport_recv_eof_ = true;
      }
      memio_PutReadResult(nss_bufs_, MapErrorToNSS(rv));
    }
  }
  return rv;
}

// Return 0 for EOF,
// > 0 for bytes transferred immediately,
// < 0 for error (or the non-error ERR_IO_PENDING).
int SSLClientSocketNSS::Core::BufferSend() {
  DCHECK(OnNSSTaskRunner());

  if (transport_send_busy_)
    return ERR_IO_PENDING;

  const char* buf1;
  const char* buf2;
  unsigned int len1, len2;
  memio_GetWriteParams(nss_bufs_, &buf1, &len1, &buf2, &len2);
  const unsigned int len = len1 + len2;

  int rv = 0;
  if (len) {
    scoped_refptr<IOBuffer> send_buffer(new IOBuffer(len));
    memcpy(send_buffer->data(), buf1, len1);
    memcpy(send_buffer->data() + len1, buf2, len2);

    if (OnNetworkTaskRunner()) {
      rv = DoBufferSend(send_buffer, len);
    } else {
      bool posted = network_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(IgnoreResult(&Core::DoBufferSend), this, send_buffer,
                     len));
      rv = posted ? ERR_IO_PENDING : ERR_ABORTED;
    }

    if (rv == ERR_IO_PENDING) {
      transport_send_busy_ = true;
    } else {
      memio_PutWriteResult(nss_bufs_, MapErrorToNSS(rv));
    }
  }

  return rv;
}

void SSLClientSocketNSS::Core::OnRecvComplete(int result) {
  DCHECK(OnNSSTaskRunner());

  if (next_handshake_state_ == STATE_HANDSHAKE) {
    OnHandshakeIOComplete(result);
    return;
  }

  // Network layer received some data, check if client requested to read
  // decrypted data.
  if (!user_read_buf_)
    return;

  int rv = DoReadLoop(result);
  if (rv != ERR_IO_PENDING)
    DoReadCallback(rv);
}

void SSLClientSocketNSS::Core::OnSendComplete(int result) {
  DCHECK(OnNSSTaskRunner());

  if (next_handshake_state_ == STATE_HANDSHAKE) {
    OnHandshakeIOComplete(result);
    return;
  }

  // OnSendComplete may need to call DoPayloadRead while the renegotiation
  // handshake is in progress.
  int rv_read = ERR_IO_PENDING;
  int rv_write = ERR_IO_PENDING;
  bool network_moved;
  do {
      if (user_read_buf_)
          rv_read = DoPayloadRead();
      if (user_write_buf_)
          rv_write = DoPayloadWrite();
      network_moved = DoTransportIO();
  } while (rv_read == ERR_IO_PENDING &&
           rv_write == ERR_IO_PENDING &&
           (user_read_buf_ || user_write_buf_) &&
           network_moved);

  if (user_read_buf_ && rv_read != ERR_IO_PENDING)
      DoReadCallback(rv_read);
  if (user_write_buf_ && rv_write != ERR_IO_PENDING)
      DoWriteCallback(rv_write);
}

// As part of Connect(), the SSLClientSocketNSS object performs an SSL
// handshake. This requires network IO, which in turn calls
// BufferRecvComplete() with a non-zero byte count. This byte count eventually
// winds its way through the state machine and ends up being passed to the
// callback. For Read() and Write(), that's what we want. But for Connect(),
// the caller expects OK (i.e. 0) for success.
void SSLClientSocketNSS::Core::DoConnectCallback(int rv) {
  DCHECK(OnNSSTaskRunner());
  DCHECK_NE(rv, ERR_IO_PENDING);
  DCHECK(!user_connect_callback_.is_null());

  base::Closure c = base::Bind(
      base::ResetAndReturn(&user_connect_callback_),
      rv > OK ? OK : rv);
  PostOrRunCallback(FROM_HERE, c);
}

void SSLClientSocketNSS::Core::DoReadCallback(int rv) {
  DCHECK(OnNSSTaskRunner());
  DCHECK_NE(ERR_IO_PENDING, rv);
  DCHECK(!user_read_callback_.is_null());

  user_read_buf_ = NULL;
  user_read_buf_len_ = 0;
  base::Closure c = base::Bind(
      base::ResetAndReturn(&user_read_callback_),
      rv);
  PostOrRunCallback(FROM_HERE, c);
}

void SSLClientSocketNSS::Core::DoWriteCallback(int rv) {
  DCHECK(OnNSSTaskRunner());
  DCHECK_NE(ERR_IO_PENDING, rv);
  DCHECK(!user_write_callback_.is_null());

  // Since Run may result in Write being called, clear |user_write_callback_|
  // up front.
  user_write_buf_ = NULL;
  user_write_buf_len_ = 0;
  base::Closure c = base::Bind(
      base::ResetAndReturn(&user_write_callback_),
      rv);
  PostOrRunCallback(FROM_HERE, c);
}

SECStatus SSLClientSocketNSS::Core::DomainBoundClientAuthHandler(
    const SECItem* cert_types,
    CERTCertificate** result_certificate,
    SECKEYPrivateKey** result_private_key) {
  DCHECK(OnNSSTaskRunner());

  domain_bound_cert_xtn_negotiated_ = true;

  // We have negotiated the domain-bound certificate extension.
  std::string origin = "https://" + host_and_port_.ToString();
  std::vector<uint8> requested_cert_types(cert_types->data,
                                          cert_types->data + cert_types->len);
  int error = ERR_UNEXPECTED;
  if (OnNetworkTaskRunner()) {
    error = DoGetDomainBoundCert(origin, requested_cert_types);
  } else {
    bool posted = network_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(IgnoreResult(&Core::DoGetDomainBoundCert), this, origin,
                   requested_cert_types));
    error = posted ? ERR_IO_PENDING : ERR_ABORTED;
  }

  if (error == ERR_IO_PENDING) {
    // Asynchronous case.
    client_auth_cert_needed_ = true;
    return SECWouldBlock;
  }

  PostOrRunCallback(
      FROM_HERE,
      base::Bind(&BoundNetLog::EndEventWithNetErrorCode, weak_net_log_,
                 NetLog::TYPE_SSL_GET_DOMAIN_BOUND_CERT, error));
  SECStatus rv = SECSuccess;
  if (error == OK) {
    // Synchronous success.
    int result = ImportDBCertAndKey(result_certificate, result_private_key);
    if (result != OK) {
      domain_bound_cert_type_ = CLIENT_CERT_INVALID_TYPE;
      rv = SECFailure;
    }
  } else {
    rv = SECFailure;
  }

  int cert_count = (rv == SECSuccess) ? 1 : 0;
  AddCertProvidedEvent(cert_count);
  return rv;
}

int SSLClientSocketNSS::Core::ImportDBCertAndKey(CERTCertificate** cert,
                                                 SECKEYPrivateKey** key) {
  // Set the certificate.
  SECItem cert_item;
  cert_item.data = (unsigned char*) domain_bound_cert_.data();
  cert_item.len = domain_bound_cert_.size();
  *cert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
                                  &cert_item,
                                  NULL,
                                  PR_FALSE,
                                  PR_TRUE);
  if (*cert == NULL)
    return MapNSSError(PORT_GetError());

  // Set the private key.
  switch (domain_bound_cert_type_) {
    case CLIENT_CERT_ECDSA_SIGN: {
      SECKEYPublicKey* public_key = NULL;
      if (!crypto::ECPrivateKey::ImportFromEncryptedPrivateKeyInfo(
          ServerBoundCertService::kEPKIPassword,
          reinterpret_cast<const unsigned char*>(
              domain_bound_private_key_.data()),
          domain_bound_private_key_.size(),
          &(*cert)->subjectPublicKeyInfo,
          false,
          false,
          key,
          &public_key)) {
        int error = MapNSSError(PORT_GetError());
        CERT_DestroyCertificate(*cert);
        *cert = NULL;
        return error;
      }
      SECKEY_DestroyPublicKey(public_key);
      break;
    }

    default:
      NOTREACHED();
      return ERR_INVALID_ARGUMENT;
  }

  return OK;
}

void SSLClientSocketNSS::Core::UpdateServerCert() {
  nss_handshake_state_.server_cert_chain.Reset(nss_fd_);
  nss_handshake_state_.server_cert = X509Certificate::CreateFromDERCertChain(
      nss_handshake_state_.server_cert_chain.AsStringPieceVector());
  if (nss_handshake_state_.server_cert) {
    // Since this will be called asynchronously on another thread, it needs to
    // own a reference to the certificate.
    NetLog::ParametersCallback net_log_callback =
        base::Bind(&NetLogX509CertificateCallback,
                   nss_handshake_state_.server_cert);
    PostOrRunCallback(
        FROM_HERE,
        base::Bind(&AddLogEventWithCallback, weak_net_log_,
                   NetLog::TYPE_SSL_CERTIFICATES_RECEIVED,
                   net_log_callback));
  }
}

void SSLClientSocketNSS::Core::UpdateConnectionStatus() {
  SSLChannelInfo channel_info;
  SECStatus ok = SSL_GetChannelInfo(nss_fd_,
                                    &channel_info, sizeof(channel_info));
  if (ok == SECSuccess &&
      channel_info.length == sizeof(channel_info) &&
      channel_info.cipherSuite) {
    nss_handshake_state_.ssl_connection_status |=
        (static_cast<int>(channel_info.cipherSuite) &
         SSL_CONNECTION_CIPHERSUITE_MASK) <<
        SSL_CONNECTION_CIPHERSUITE_SHIFT;

    nss_handshake_state_.ssl_connection_status |=
        (static_cast<int>(channel_info.compressionMethod) &
         SSL_CONNECTION_COMPRESSION_MASK) <<
        SSL_CONNECTION_COMPRESSION_SHIFT;

    // NSS 3.12.x doesn't have version macros for TLS 1.1 and 1.2 (because NSS
    // doesn't support them yet), so we use 0x0302 and 0x0303 directly.
    int version = SSL_CONNECTION_VERSION_UNKNOWN;
    if (channel_info.protocolVersion < SSL_LIBRARY_VERSION_3_0) {
      // All versions less than SSL_LIBRARY_VERSION_3_0 are treated as SSL
      // version 2.
      version = SSL_CONNECTION_VERSION_SSL2;
    } else if (channel_info.protocolVersion == SSL_LIBRARY_VERSION_3_0) {
      version = SSL_CONNECTION_VERSION_SSL3;
    } else if (channel_info.protocolVersion == SSL_LIBRARY_VERSION_3_1_TLS) {
      version = SSL_CONNECTION_VERSION_TLS1;
    } else if (channel_info.protocolVersion == 0x0302) {
      version = SSL_CONNECTION_VERSION_TLS1_1;
    } else if (channel_info.protocolVersion == 0x0303) {
      version = SSL_CONNECTION_VERSION_TLS1_2;
    }
    nss_handshake_state_.ssl_connection_status |=
        (version & SSL_CONNECTION_VERSION_MASK) <<
        SSL_CONNECTION_VERSION_SHIFT;
  }

  // SSL_HandshakeNegotiatedExtension was added in NSS 3.12.6.
  // Since SSL_MAX_EXTENSIONS was added at the same time, we can test
  // SSL_MAX_EXTENSIONS for the presence of SSL_HandshakeNegotiatedExtension.
#if defined(SSL_MAX_EXTENSIONS)
  PRBool peer_supports_renego_ext;
  ok = SSL_HandshakeNegotiatedExtension(nss_fd_, ssl_renegotiation_info_xtn,
                                        &peer_supports_renego_ext);
  if (ok == SECSuccess) {
    if (!peer_supports_renego_ext) {
      nss_handshake_state_.ssl_connection_status |=
          SSL_CONNECTION_NO_RENEGOTIATION_EXTENSION;
      // Log an informational message if the server does not support secure
      // renegotiation (RFC 5746).
      VLOG(1) << "The server " << host_and_port_.ToString()
              << " does not support the TLS renegotiation_info extension.";
    }
    UMA_HISTOGRAM_ENUMERATION("Net.RenegotiationExtensionSupported",
                              peer_supports_renego_ext, 2);
  }
#endif

  if (ssl_config_.version_fallback) {
    nss_handshake_state_.ssl_connection_status |=
        SSL_CONNECTION_VERSION_FALLBACK;
  }
}

void SSLClientSocketNSS::Core::RecordDomainBoundCertSupport() const {
  if (nss_handshake_state_.resumed_handshake)
    return;

  // Since this enum is used for a histogram, do not change or re-use values.
  enum {
    DISABLED = 0,
    CLIENT_ONLY = 1,
    CLIENT_AND_SERVER = 2,
    DOMAIN_BOUND_CERT_USAGE_MAX
  } supported = DISABLED;
#ifdef SSL_ENABLE_OB_CERTS
  if (domain_bound_cert_xtn_negotiated_)
    supported = CLIENT_AND_SERVER;
  else if (ssl_config_.domain_bound_certs_enabled)
    supported = CLIENT_ONLY;
#endif
  UMA_HISTOGRAM_ENUMERATION("DomainBoundCerts.Support", supported,
                            DOMAIN_BOUND_CERT_USAGE_MAX);
}

int SSLClientSocketNSS::Core::DoBufferRecv(IOBuffer* read_buffer, int len) {
  DCHECK(OnNetworkTaskRunner());
  DCHECK_GT(len, 0);

  if (detached_)
    return ERR_ABORTED;

  int rv = transport_->socket()->Read(
      read_buffer, len,
      base::Bind(&Core::BufferRecvComplete, base::Unretained(this),
                 scoped_refptr<IOBuffer>(read_buffer)));

  if (!OnNSSTaskRunner() && rv != ERR_IO_PENDING) {
    nss_task_runner_->PostTask(
        FROM_HERE, base::Bind(&Core::BufferRecvComplete, this,
                              scoped_refptr<IOBuffer>(read_buffer), rv));
    return rv;
  }

  return rv;
}

int SSLClientSocketNSS::Core::DoBufferSend(IOBuffer* send_buffer, int len) {
  DCHECK(OnNetworkTaskRunner());
  DCHECK_GT(len, 0);

  if (detached_)
    return ERR_ABORTED;

  int rv = transport_->socket()->Write(
        send_buffer, len,
        base::Bind(&Core::BufferSendComplete,
                   base::Unretained(this)));

  if (!OnNSSTaskRunner() && rv != ERR_IO_PENDING) {
    nss_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&Core::BufferSendComplete, this, rv));
    return rv;
  }

  return rv;
}

int SSLClientSocketNSS::Core::DoGetDomainBoundCert(
    const std::string& origin,
    const std::vector<uint8>& requested_cert_types) {
  DCHECK(OnNetworkTaskRunner());

  if (detached_)
    return ERR_FAILED;

  weak_net_log_->BeginEvent(NetLog::TYPE_SSL_GET_DOMAIN_BOUND_CERT);

  int rv = server_bound_cert_service_->GetDomainBoundCert(
      origin,
      requested_cert_types,
      &domain_bound_cert_type_,
      &domain_bound_private_key_,
      &domain_bound_cert_,
      base::Bind(&Core::OnGetDomainBoundCertComplete, base::Unretained(this)),
      &domain_bound_cert_request_handle_);

  if (rv != ERR_IO_PENDING && !OnNSSTaskRunner()) {
    nss_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&Core::OnHandshakeIOComplete, this, rv));
    return ERR_IO_PENDING;
  }

  return rv;
}

void SSLClientSocketNSS::Core::OnHandshakeStateUpdated(
    const HandshakeState& state) {
  network_handshake_state_ = state;
}

void SSLClientSocketNSS::Core::BufferSendComplete(int result) {
  if (!OnNSSTaskRunner()) {
    if (detached_)
      return;

    nss_task_runner_->PostTask(
        FROM_HERE, base::Bind(&Core::BufferSendComplete, this, result));
    return;
  }

  DCHECK(OnNSSTaskRunner());

  memio_PutWriteResult(nss_bufs_, MapErrorToNSS(result));
  transport_send_busy_ = false;
  OnSendComplete(result);
}

void SSLClientSocketNSS::Core::OnHandshakeIOComplete(int result) {
  if (!OnNSSTaskRunner()) {
    if (detached_)
      return;

    nss_task_runner_->PostTask(
        FROM_HERE, base::Bind(&Core::OnHandshakeIOComplete, this, result));
    return;
  }

  DCHECK(OnNSSTaskRunner());

  int rv = DoHandshakeLoop(result);
  if (rv != ERR_IO_PENDING)
    DoConnectCallback(rv);
}

void SSLClientSocketNSS::Core::OnGetDomainBoundCertComplete(int result) {
  DCHECK(OnNetworkTaskRunner());

  domain_bound_cert_request_handle_ = NULL;
  OnHandshakeIOComplete(result);
}

void SSLClientSocketNSS::Core::BufferRecvComplete(
    IOBuffer* read_buffer,
    int result) {
  DCHECK(read_buffer);

  if (!OnNSSTaskRunner()) {
    if (detached_)
      return;

    nss_task_runner_->PostTask(
        FROM_HERE, base::Bind(&Core::BufferRecvComplete, this,
                              scoped_refptr<IOBuffer>(read_buffer), result));
    return;
  }

  DCHECK(OnNSSTaskRunner());

  if (result > 0) {
    char* buf;
    int nb = memio_GetReadParams(nss_bufs_, &buf);
    CHECK_GE(nb, result);
    memcpy(buf, read_buffer->data(), result);
  } else if (result == 0) {
    transport_recv_eof_ = true;
  }

  memio_PutReadResult(nss_bufs_, MapErrorToNSS(result));
  transport_recv_busy_ = false;
  OnRecvComplete(result);
}

void SSLClientSocketNSS::Core::PostOrRunCallback(
    const tracked_objects::Location& location,
    const base::Closure& task) {
  if (!OnNetworkTaskRunner()) {
    network_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&Core::PostOrRunCallback, this, location, task));
    return;
  }

  if (detached_ || task.is_null())
    return;
  task.Run();
}

void SSLClientSocketNSS::Core::AddCertProvidedEvent(int cert_count) {
  PostOrRunCallback(
      FROM_HERE,
      base::Bind(&AddLogEventWithCallback, weak_net_log_,
                 NetLog::TYPE_SSL_CLIENT_CERT_PROVIDED,
                 NetLog::IntegerCallback("cert_count", cert_count)));
}

SSLClientSocketNSS::SSLClientSocketNSS(
    base::SingleThreadTaskRunner* nss_task_runner,
    ClientSocketHandle* transport_socket,
    const HostPortPair& host_and_port,
    const SSLConfig& ssl_config,
    SSLHostInfo* ssl_host_info,
    const SSLClientSocketContext& context)
    : nss_task_runner_(nss_task_runner),
      transport_(transport_socket),
      host_and_port_(host_and_port),
      ssl_config_(ssl_config),
      server_cert_verify_result_(NULL),
      cert_verifier_(context.cert_verifier),
      server_bound_cert_service_(context.server_bound_cert_service),
      ssl_session_cache_shard_(context.ssl_session_cache_shard),
      completed_handshake_(false),
      next_handshake_state_(STATE_NONE),
      nss_fd_(NULL),
      net_log_(transport_socket->socket()->NetLog()),
      ssl_host_info_(ssl_host_info),
      transport_security_state_(context.transport_security_state),
      valid_thread_id_(base::kInvalidThreadId) {
  EnterFunction("");
  InitCore();
  LeaveFunction("");
}

SSLClientSocketNSS::~SSLClientSocketNSS() {
  EnterFunction("");
  Disconnect();
  LeaveFunction("");
}

// static
void SSLClientSocket::ClearSessionCache() {
  // SSL_ClearSessionCache can't be called before NSS is initialized.  Don't
  // bother initializing NSS just to clear an empty SSL session cache.
  if (!NSS_IsInitialized())
    return;

  SSL_ClearSessionCache();
}

void SSLClientSocketNSS::GetSSLInfo(SSLInfo* ssl_info) {
  EnterFunction("");
  ssl_info->Reset();
  if (core_->state().server_cert_chain.empty() ||
      !core_->state().server_cert_chain[0]) {
    return;
  }

  ssl_info->cert_status = server_cert_verify_result_->cert_status;
  ssl_info->cert = server_cert_verify_result_->verified_cert;
  ssl_info->connection_status =
      core_->state().ssl_connection_status;
  ssl_info->public_key_hashes = server_cert_verify_result_->public_key_hashes;
  for (std::vector<SHA1Fingerprint>::const_iterator
       i = side_pinned_public_keys_.begin();
       i != side_pinned_public_keys_.end(); i++) {
    ssl_info->public_key_hashes.push_back(*i);
  }
  ssl_info->is_issued_by_known_root =
      server_cert_verify_result_->is_issued_by_known_root;
  ssl_info->client_cert_sent = WasDomainBoundCertSent() ||
      (ssl_config_.send_client_cert && ssl_config_.client_cert);

  PRUint16 cipher_suite = SSLConnectionStatusToCipherSuite(
      core_->state().ssl_connection_status);
  SSLCipherSuiteInfo cipher_info;
  SECStatus ok = SSL_GetCipherSuiteInfo(cipher_suite,
                                        &cipher_info, sizeof(cipher_info));
  if (ok == SECSuccess) {
    ssl_info->security_bits = cipher_info.effectiveKeyBits;
  } else {
    ssl_info->security_bits = -1;
    LOG(DFATAL) << "SSL_GetCipherSuiteInfo returned " << PR_GetError()
                << " for cipherSuite " << cipher_suite;
  }

  ssl_info->handshake_type = core_->state().resumed_handshake ?
      SSLInfo::HANDSHAKE_RESUME : SSLInfo::HANDSHAKE_FULL;

  LeaveFunction("");
}

void SSLClientSocketNSS::GetSSLCertRequestInfo(
    SSLCertRequestInfo* cert_request_info) {
  EnterFunction("");
  // TODO(rch): switch SSLCertRequestInfo.host_and_port to a HostPortPair
  cert_request_info->host_and_port = host_and_port_.ToString();
  cert_request_info->client_certs = core_->state().client_certs;
  LeaveFunction(cert_request_info->client_certs.size());
}

int SSLClientSocketNSS::ExportKeyingMaterial(const base::StringPiece& label,
                                             bool has_context,
                                             const base::StringPiece& context,
                                             unsigned char* out,
                                             unsigned int outlen) {
  if (!IsConnected())
    return ERR_SOCKET_NOT_CONNECTED;

  // SSL_ExportKeyingMaterial may block the current thread if |core_| is in
  // the midst of a handshake.
  SECStatus result = SSL_ExportKeyingMaterial(
      nss_fd_, label.data(), label.size(), has_context,
      reinterpret_cast<const unsigned char*>(context.data()),
      context.length(), out, outlen);
  if (result != SECSuccess) {
    LogFailedNSSFunction(net_log_, "SSL_ExportKeyingMaterial", "");
    return MapNSSError(PORT_GetError());
  }
  return OK;
}

SSLClientSocket::NextProtoStatus
SSLClientSocketNSS::GetNextProto(std::string* proto,
                                 std::string* server_protos) {
  *proto = core_->state().next_proto;
  *server_protos = core_->state().server_protos;
  return core_->state().next_proto_status;
}

int SSLClientSocketNSS::Connect(const CompletionCallback& callback) {
  EnterFunction("");
  DCHECK(transport_.get());
  DCHECK_EQ(STATE_NONE, next_handshake_state_);
  DCHECK(user_connect_callback_.is_null());
  DCHECK(!callback.is_null());

  EnsureThreadIdAssigned();

  net_log_.BeginEvent(NetLog::TYPE_SSL_CONNECT);

  int rv = Init();
  if (rv != OK) {
    net_log_.EndEventWithNetErrorCode(NetLog::TYPE_SSL_CONNECT, rv);
    return rv;
  }

  rv = InitializeSSLOptions();
  if (rv != OK) {
    net_log_.EndEventWithNetErrorCode(NetLog::TYPE_SSL_CONNECT, rv);
    return rv;
  }

  rv = InitializeSSLPeerName();
  if (rv != OK) {
    net_log_.EndEventWithNetErrorCode(NetLog::TYPE_SSL_CONNECT, rv);
    return rv;
  }

  if (ssl_config_.cached_info_enabled && ssl_host_info_.get()) {
    GotoState(STATE_LOAD_SSL_HOST_INFO);
  } else {
    GotoState(STATE_HANDSHAKE);
  }

  rv = DoHandshakeLoop(OK);
  if (rv == ERR_IO_PENDING) {
    user_connect_callback_ = callback;
  } else {
    net_log_.EndEventWithNetErrorCode(NetLog::TYPE_SSL_CONNECT, rv);
  }

  LeaveFunction("");
  return rv > OK ? OK : rv;
}

void SSLClientSocketNSS::Disconnect() {
  EnterFunction("");

  CHECK(CalledOnValidThread());

  // Shut down anything that may call us back.
  core_->Detach();
  verifier_.reset();
  transport_->socket()->Disconnect();

  // Reset object state.
  user_connect_callback_.Reset();
  local_server_cert_verify_result_.Reset();
  server_cert_verify_result_ = NULL;
  completed_handshake_   = false;
  start_cert_verification_time_ = base::TimeTicks();
  InitCore();

  LeaveFunction("");
}

bool SSLClientSocketNSS::IsConnected() const {
  // Ideally, we should also check if we have received the close_notify alert
  // message from the server, and return false in that case.  We're not doing
  // that, so this function may return a false positive.  Since the upper
  // layer (HttpNetworkTransaction) needs to handle a persistent connection
  // closed by the server when we send a request anyway, a false positive in
  // exchange for simpler code is a good trade-off.
  EnterFunction("");
  bool ret = completed_handshake_ && transport_->socket()->IsConnected();
  LeaveFunction("");
  return ret;
}

bool SSLClientSocketNSS::IsConnectedAndIdle() const {
  // Unlike IsConnected, this method doesn't return a false positive.
  //
  // Strictly speaking, we should check if we have received the close_notify
  // alert message from the server, and return false in that case.  Although
  // the close_notify alert message means EOF in the SSL layer, it is just
  // bytes to the transport layer below, so
  // transport_->socket()->IsConnectedAndIdle() returns the desired false
  // when we receive close_notify.
  EnterFunction("");
  bool ret = completed_handshake_ && transport_->socket()->IsConnectedAndIdle();
  LeaveFunction("");
  return ret;
}

int SSLClientSocketNSS::GetPeerAddress(IPEndPoint* address) const {
  return transport_->socket()->GetPeerAddress(address);
}

int SSLClientSocketNSS::GetLocalAddress(IPEndPoint* address) const {
  return transport_->socket()->GetLocalAddress(address);
}

const BoundNetLog& SSLClientSocketNSS::NetLog() const {
  return net_log_;
}

void SSLClientSocketNSS::SetSubresourceSpeculation() {
  if (transport_.get() && transport_->socket()) {
    transport_->socket()->SetSubresourceSpeculation();
  } else {
    NOTREACHED();
  }
}

void SSLClientSocketNSS::SetOmniboxSpeculation() {
  if (transport_.get() && transport_->socket()) {
    transport_->socket()->SetOmniboxSpeculation();
  } else {
    NOTREACHED();
  }
}

bool SSLClientSocketNSS::WasEverUsed() const {
  if (transport_.get() && transport_->socket()) {
    return transport_->socket()->WasEverUsed();
  }
  NOTREACHED();
  return false;
}

bool SSLClientSocketNSS::UsingTCPFastOpen() const {
  if (transport_.get() && transport_->socket()) {
    return transport_->socket()->UsingTCPFastOpen();
  }
  NOTREACHED();
  return false;
}

int64 SSLClientSocketNSS::NumBytesRead() const {
  if (transport_.get() && transport_->socket()) {
    return transport_->socket()->NumBytesRead();
  }
  NOTREACHED();
  return -1;
}

base::TimeDelta SSLClientSocketNSS::GetConnectTimeMicros() const {
  if (transport_.get() && transport_->socket()) {
    return transport_->socket()->GetConnectTimeMicros();
  }
  NOTREACHED();
  return base::TimeDelta::FromMicroseconds(-1);
}

int SSLClientSocketNSS::Read(IOBuffer* buf, int buf_len,
                             const CompletionCallback& callback) {
  DCHECK(core_);
  DCHECK(!callback.is_null());

  EnterFunction(buf_len);
  int rv = core_->Read(buf, buf_len, callback);
  LeaveFunction(rv);

  return rv;
}

int SSLClientSocketNSS::Write(IOBuffer* buf, int buf_len,
                              const CompletionCallback& callback) {
  DCHECK(core_);
  DCHECK(!callback.is_null());

  EnterFunction(buf_len);
  int rv = core_->Write(buf, buf_len, callback);
  LeaveFunction(rv);

  return rv;
}

bool SSLClientSocketNSS::SetReceiveBufferSize(int32 size) {
  return transport_->socket()->SetReceiveBufferSize(size);
}

bool SSLClientSocketNSS::SetSendBufferSize(int32 size) {
  return transport_->socket()->SetSendBufferSize(size);
}

int SSLClientSocketNSS::Init() {
  EnterFunction("");
  // Initialize the NSS SSL library in a threadsafe way.  This also
  // initializes the NSS base library.
  EnsureNSSSSLInit();
  if (!NSS_IsInitialized())
    return ERR_UNEXPECTED;
#if !defined(OS_MACOSX) && !defined(OS_WIN)
  if (ssl_config_.cert_io_enabled) {
    // We must call EnsureNSSHttpIOInit() here, on the IO thread, to get the IO
    // loop by MessageLoopForIO::current().
    // X509Certificate::Verify() runs on a worker thread of CertVerifier.
    EnsureNSSHttpIOInit();
  }
#endif

  LeaveFunction("");
  return OK;
}

void SSLClientSocketNSS::InitCore() {
  core_ = new Core(base::ThreadTaskRunnerHandle::Get(), nss_task_runner_,
                   transport_.get(), host_and_port_, ssl_config_, &net_log_,
                   server_bound_cert_service_);
}

int SSLClientSocketNSS::InitializeSSLOptions() {
  // Transport connected, now hook it up to nss
  // TODO(port): specify rx and tx buffer sizes separately
  nss_fd_ = memio_CreateIOLayer(kRecvBufferSize);
  if (nss_fd_ == NULL) {
    return ERR_OUT_OF_MEMORY;  // TODO(port): map NSPR error code.
  }

  // Grab pointer to buffers
  memio_Private* nss_bufs = memio_GetSecret(nss_fd_);

  /* Create SSL state machine */
  /* Push SSL onto our fake I/O socket */
  nss_fd_ = SSL_ImportFD(NULL, nss_fd_);
  if (nss_fd_ == NULL) {
    LogFailedNSSFunction(net_log_, "SSL_ImportFD", "");
    return ERR_OUT_OF_MEMORY;  // TODO(port): map NSPR/NSS error code.
  }
  // TODO(port): set more ssl options!  Check errors!

  int rv;

  rv = SSL_OptionSet(nss_fd_, SSL_SECURITY, PR_TRUE);
  if (rv != SECSuccess) {
    LogFailedNSSFunction(net_log_, "SSL_OptionSet", "SSL_SECURITY");
    return ERR_UNEXPECTED;
  }

  rv = SSL_OptionSet(nss_fd_, SSL_ENABLE_SSL2, PR_FALSE);
  if (rv != SECSuccess) {
    LogFailedNSSFunction(net_log_, "SSL_OptionSet", "SSL_ENABLE_SSL2");
    return ERR_UNEXPECTED;
  }

  // Don't do V2 compatible hellos because they don't support TLS extensions.
  rv = SSL_OptionSet(nss_fd_, SSL_V2_COMPATIBLE_HELLO, PR_FALSE);
  if (rv != SECSuccess) {
    LogFailedNSSFunction(net_log_, "SSL_OptionSet", "SSL_V2_COMPATIBLE_HELLO");
    return ERR_UNEXPECTED;
  }

  SSLVersionRange version_range;
  version_range.min = ssl_config_.version_min;
  version_range.max = ssl_config_.version_max;
  rv = SSL_VersionRangeSet(nss_fd_, &version_range);
  if (rv != SECSuccess) {
    LogFailedNSSFunction(net_log_, "SSL_VersionRangeSet", "");
    return ERR_NO_SSL_VERSIONS_ENABLED;
  }

  for (std::vector<uint16>::const_iterator it =
           ssl_config_.disabled_cipher_suites.begin();
       it != ssl_config_.disabled_cipher_suites.end(); ++it) {
    // This will fail if the specified cipher is not implemented by NSS, but
    // the failure is harmless.
    SSL_CipherPrefSet(nss_fd_, *it, PR_FALSE);
  }

#ifdef SSL_ENABLE_SESSION_TICKETS
  // Support RFC 5077
  rv = SSL_OptionSet(nss_fd_, SSL_ENABLE_SESSION_TICKETS, PR_TRUE);
  if (rv != SECSuccess) {
    LogFailedNSSFunction(
        net_log_, "SSL_OptionSet", "SSL_ENABLE_SESSION_TICKETS");
  }
#else
  #error "You need to install NSS-3.12 or later to build chromium"
#endif

#ifdef SSL_ENABLE_DEFLATE
  // Some web servers have been found to break if TLS is used *or* if DEFLATE
  // is advertised. Thus, if TLS is disabled (probably because we are doing
  // SSLv3 fallback), we disable DEFLATE also.
  // See http://crbug.com/31628
  rv = SSL_OptionSet(nss_fd_, SSL_ENABLE_DEFLATE,
                     ssl_config_.version_max >= SSL_PROTOCOL_VERSION_TLS1);
  if (rv != SECSuccess)
    LogFailedNSSFunction(net_log_, "SSL_OptionSet", "SSL_ENABLE_DEFLATE");
#endif

#ifdef SSL_ENABLE_FALSE_START
  rv = SSL_OptionSet(nss_fd_, SSL_ENABLE_FALSE_START,
                     ssl_config_.false_start_enabled);
  if (rv != SECSuccess)
    LogFailedNSSFunction(net_log_, "SSL_OptionSet", "SSL_ENABLE_FALSE_START");
#endif

#ifdef SSL_ENABLE_RENEGOTIATION
  // We allow servers to request renegotiation. Since we're a client,
  // prohibiting this is rather a waste of time. Only servers are in a
  // position to prevent renegotiation attacks.
  // http://extendedsubset.com/?p=8

  rv = SSL_OptionSet(nss_fd_, SSL_ENABLE_RENEGOTIATION,
                     SSL_RENEGOTIATE_TRANSITIONAL);
  if (rv != SECSuccess) {
    LogFailedNSSFunction(
        net_log_, "SSL_OptionSet", "SSL_ENABLE_RENEGOTIATION");
  }
#endif  // SSL_ENABLE_RENEGOTIATION

#ifdef SSL_CBC_RANDOM_IV
  rv = SSL_OptionSet(nss_fd_, SSL_CBC_RANDOM_IV,
                     ssl_config_.false_start_enabled);
  if (rv != SECSuccess)
    LogFailedNSSFunction(net_log_, "SSL_OptionSet", "SSL_CBC_RANDOM_IV");
#endif

#ifdef SSL_ENABLE_OCSP_STAPLING
  if (IsOCSPStaplingSupported()) {
    rv = SSL_OptionSet(nss_fd_, SSL_ENABLE_OCSP_STAPLING, PR_TRUE);
    if (rv != SECSuccess) {
      LogFailedNSSFunction(net_log_, "SSL_OptionSet",
                           "SSL_ENABLE_OCSP_STAPLING");
    }
  }
#endif

#ifdef SSL_ENABLE_CACHED_INFO
  rv = SSL_OptionSet(nss_fd_, SSL_ENABLE_CACHED_INFO,
                     ssl_config_.cached_info_enabled);
  if (rv != SECSuccess)
    LogFailedNSSFunction(net_log_, "SSL_OptionSet", "SSL_ENABLE_CACHED_INFO");
#endif

#ifdef SSL_ENABLE_OB_CERTS
  rv = SSL_OptionSet(nss_fd_, SSL_ENABLE_OB_CERTS,
                     ssl_config_.domain_bound_certs_enabled);
  if (rv != SECSuccess)
    LogFailedNSSFunction(net_log_, "SSL_OptionSet", "SSL_ENABLE_OB_CERTS");
#endif

#ifdef SSL_ENCRYPT_CLIENT_CERTS
  // For now, enable the encrypted client certificates extension only if
  // server-bound certificates are enabled.
  rv = SSL_OptionSet(nss_fd_, SSL_ENCRYPT_CLIENT_CERTS,
                     ssl_config_.domain_bound_certs_enabled);
  if (rv != SECSuccess)
    LogFailedNSSFunction(net_log_, "SSL_OptionSet", "SSL_ENCRYPT_CLIENT_CERTS");
#endif

  rv = SSL_OptionSet(nss_fd_, SSL_HANDSHAKE_AS_CLIENT, PR_TRUE);
  if (rv != SECSuccess) {
    LogFailedNSSFunction(net_log_, "SSL_OptionSet", "SSL_HANDSHAKE_AS_CLIENT");
    return ERR_UNEXPECTED;
  }

  if (!core_->Init(nss_fd_, nss_bufs))
    return ERR_UNEXPECTED;

  // Tell SSL the hostname we're trying to connect to.
  SSL_SetURL(nss_fd_, host_and_port_.host().c_str());

  // Tell SSL we're a client; needed if not letting NSPR do socket I/O
  SSL_ResetHandshake(nss_fd_, PR_FALSE);

  return OK;
}

int SSLClientSocketNSS::InitializeSSLPeerName() {
  // Tell NSS who we're connected to
  IPEndPoint peer_address;
  int err = transport_->socket()->GetPeerAddress(&peer_address);
  if (err != OK)
    return err;

  SockaddrStorage storage;
  if (!peer_address.ToSockAddr(storage.addr, &storage.addr_len))
    return ERR_UNEXPECTED;

  PRNetAddr peername;
  memset(&peername, 0, sizeof(peername));
  DCHECK_LE(static_cast<size_t>(storage.addr_len), sizeof(peername));
  size_t len = std::min(static_cast<size_t>(storage.addr_len),
                        sizeof(peername));
  memcpy(&peername, storage.addr, len);

  // Adjust the address family field for BSD, whose sockaddr
  // structure has a one-byte length and one-byte address family
  // field at the beginning.  PRNetAddr has a two-byte address
  // family field at the beginning.
  peername.raw.family = storage.addr->sa_family;

  memio_SetPeerName(nss_fd_, &peername);

  // Set the peer ID for session reuse.  This is necessary when we create an
  // SSL tunnel through a proxy -- GetPeerName returns the proxy's address
  // rather than the destination server's address in that case.
  std::string peer_id = host_and_port_.ToString();
  // If the ssl_session_cache_shard_ is non-empty, we append it to the peer id.
  // This will cause session cache misses between sockets with different values
  // of ssl_session_cache_shard_ and this is used to partition the session cache
  // for incognito mode.
  if (!ssl_session_cache_shard_.empty()) {
    peer_id += "/" + ssl_session_cache_shard_;
  }
  SECStatus rv = SSL_SetSockPeerID(nss_fd_, const_cast<char*>(peer_id.c_str()));
  if (rv != SECSuccess)
    LogFailedNSSFunction(net_log_, "SSL_SetSockPeerID", peer_id.c_str());

  return OK;
}

void SSLClientSocketNSS::DoConnectCallback(int rv) {
  EnterFunction(rv);
  DCHECK_NE(ERR_IO_PENDING, rv);
  DCHECK(!user_connect_callback_.is_null());

  base::ResetAndReturn(&user_connect_callback_).Run(rv > OK ? OK : rv);
  LeaveFunction("");
}

void SSLClientSocketNSS::OnHandshakeIOComplete(int result) {
  EnterFunction(result);
  int rv = DoHandshakeLoop(result);
  if (rv != ERR_IO_PENDING) {
    net_log_.EndEventWithNetErrorCode(NetLog::TYPE_SSL_CONNECT, rv);
    DoConnectCallback(rv);
  }
  LeaveFunction("");
}

void SSLClientSocketNSS::LoadSSLHostInfo() {
  const SSLHostInfo::State& state(ssl_host_info_->state());

  if (state.certs.empty())
    return;

  const std::vector<std::string>& certs_in = state.certs;
  core_->SetPredictedCertificates(certs_in);
}

int SSLClientSocketNSS::DoLoadSSLHostInfo() {
  EnterFunction("");
  int rv = ssl_host_info_->WaitForDataReady(
      base::Bind(&SSLClientSocketNSS::OnHandshakeIOComplete,
                 base::Unretained(this)));
  GotoState(STATE_HANDSHAKE);

  if (rv == OK) {
    LoadSSLHostInfo();
  } else {
    DCHECK_EQ(ERR_IO_PENDING, rv);
    GotoState(STATE_LOAD_SSL_HOST_INFO);
  }

  LeaveFunction("");
  return rv;
}

int SSLClientSocketNSS::DoHandshakeLoop(int last_io_result) {
  EnterFunction(last_io_result);
  int rv = last_io_result;
  do {
    // Default to STATE_NONE for next state.
    // (This is a quirk carried over from the windows
    // implementation.  It makes reading the logs a bit harder.)
    // State handlers can and often do call GotoState just
    // to stay in the current state.
    State state = next_handshake_state_;
    GotoState(STATE_NONE);
    switch (state) {
      case STATE_LOAD_SSL_HOST_INFO:
        DCHECK(rv == OK || rv == ERR_IO_PENDING);
        rv = DoLoadSSLHostInfo();
        break;
      case STATE_HANDSHAKE:
        rv = DoHandshake();
        break;
      case STATE_HANDSHAKE_COMPLETE:
        rv = DoHandshakeComplete(rv);
        break;
      case STATE_VERIFY_DNSSEC:
        rv = DoVerifyDNSSEC(rv);
        break;
      case STATE_VERIFY_CERT:
        DCHECK(rv == OK);
        rv = DoVerifyCert(rv);
        break;
      case STATE_VERIFY_CERT_COMPLETE:
        rv = DoVerifyCertComplete(rv);
        break;
      case STATE_NONE:
      default:
        rv = ERR_UNEXPECTED;
        LOG(DFATAL) << "unexpected state " << state;
        break;
    }
  } while (rv != ERR_IO_PENDING && next_handshake_state_ != STATE_NONE);
  LeaveFunction("");
  return rv;
}

int SSLClientSocketNSS::DoHandshake() {
  EnterFunction("");
  int rv = core_->Connect(
      base::Bind(&SSLClientSocketNSS::OnHandshakeIOComplete,
                 base::Unretained(this)));
  GotoState(STATE_HANDSHAKE_COMPLETE);

  LeaveFunction(rv);
  return rv;
}

int SSLClientSocketNSS::DoHandshakeComplete(int result) {
  EnterFunction(result);

  if (result == OK) {
    SaveSSLHostInfo();
    // SSL handshake is completed. Let's verify the certificate.
    GotoState(STATE_VERIFY_DNSSEC);
    // Done!
  }
  if (core_->state().domain_bound_cert_type != CLIENT_CERT_INVALID_TYPE)
    set_domain_bound_cert_type(core_->state().domain_bound_cert_type);

  LeaveFunction(result);
  return result;
}


int SSLClientSocketNSS::DoVerifyDNSSEC(int result) {
  DCHECK(!core_->state().server_cert_chain.empty());
  DCHECK(core_->state().server_cert_chain[0]);

  DNSValidationResult r = CheckDNSSECChain(
      host_and_port_.host(), core_->state().server_cert_chain[0],
      host_and_port_.port());
  if (r == DNSVR_SUCCESS) {
    local_server_cert_verify_result_.cert_status |= CERT_STATUS_IS_DNSSEC;
    local_server_cert_verify_result_.verified_cert =
        core_->state().server_cert;
    server_cert_verify_result_ = &local_server_cert_verify_result_;
    GotoState(STATE_VERIFY_CERT_COMPLETE);
    return OK;
  }

  GotoState(STATE_VERIFY_CERT);

  return OK;
}

int SSLClientSocketNSS::DoVerifyCert(int result) {
  DCHECK(!core_->state().server_cert_chain.empty());
  DCHECK(core_->state().server_cert_chain[0]);

  GotoState(STATE_VERIFY_CERT_COMPLETE);

  // If the certificate is expected to be bad we can use the expectation as
  // the cert status.
  base::StringPiece der_cert(
      reinterpret_cast<char*>(
          core_->state().server_cert_chain[0]->derCert.data),
      core_->state().server_cert_chain[0]->derCert.len);
  CertStatus cert_status;
  if (ssl_config_.IsAllowedBadCert(der_cert, &cert_status)) {
    DCHECK(start_cert_verification_time_.is_null());
    VLOG(1) << "Received an expected bad cert with status: " << cert_status;
    server_cert_verify_result_ = &local_server_cert_verify_result_;
    local_server_cert_verify_result_.Reset();
    local_server_cert_verify_result_.cert_status = cert_status;
    local_server_cert_verify_result_.verified_cert =
        core_->state().server_cert;
    return OK;
  }

  // We may have failed to create X509Certificate object if we are
  // running inside sandbox.
  if (!core_->state().server_cert) {
    server_cert_verify_result_ = &local_server_cert_verify_result_;
    local_server_cert_verify_result_.Reset();
    local_server_cert_verify_result_.cert_status = CERT_STATUS_INVALID;
    return ERR_CERT_INVALID;
  }

  start_cert_verification_time_ = base::TimeTicks::Now();

  if (ssl_host_info_.get() && !ssl_host_info_->state().certs.empty() &&
      core_->state().predicted_cert_chain_correct) {
    // If the SSLHostInfo had a prediction for the certificate chain of this
    // server then it will have optimistically started a verification of that
    // chain. So, if the prediction was correct, we should wait for that
    // verification to finish rather than start our own.
    net_log_.AddEvent(NetLog::TYPE_SSL_VERIFICATION_MERGED);
    UMA_HISTOGRAM_ENUMERATION("Net.SSLVerificationMerged", 1 /* true */, 2);
    base::TimeTicks end_time = ssl_host_info_->verification_end_time();
    if (end_time.is_null())
      end_time = base::TimeTicks::Now();
    UMA_HISTOGRAM_TIMES("Net.SSLVerificationMergedMsSaved",
                        end_time - ssl_host_info_->verification_start_time());
    server_cert_verify_result_ = &ssl_host_info_->cert_verify_result();
    return ssl_host_info_->WaitForCertVerification(
        base::Bind(&SSLClientSocketNSS::OnHandshakeIOComplete,
                   base::Unretained(this)));
  } else {
    UMA_HISTOGRAM_ENUMERATION("Net.SSLVerificationMerged", 0 /* false */, 2);
  }

  int flags = 0;
  if (ssl_config_.rev_checking_enabled)
    flags |= X509Certificate::VERIFY_REV_CHECKING_ENABLED;
  if (ssl_config_.verify_ev_cert)
    flags |= X509Certificate::VERIFY_EV_CERT;
  if (ssl_config_.cert_io_enabled)
    flags |= X509Certificate::VERIFY_CERT_IO_ENABLED;
  verifier_.reset(new SingleRequestCertVerifier(cert_verifier_));
  server_cert_verify_result_ = &local_server_cert_verify_result_;
  return verifier_->Verify(
      core_->state().server_cert, host_and_port_.host(), flags,
      SSLConfigService::GetCRLSet(), &local_server_cert_verify_result_,
      base::Bind(&SSLClientSocketNSS::OnHandshakeIOComplete,
                 base::Unretained(this)),
      net_log_);
}

// Derived from AuthCertificateCallback() in
// mozilla/source/security/manager/ssl/src/nsNSSCallbacks.cpp.
int SSLClientSocketNSS::DoVerifyCertComplete(int result) {
  verifier_.reset();

  if (!start_cert_verification_time_.is_null()) {
    base::TimeDelta verify_time =
        base::TimeTicks::Now() - start_cert_verification_time_;
    if (result == OK)
        UMA_HISTOGRAM_TIMES("Net.SSLCertVerificationTime", verify_time);
    else
        UMA_HISTOGRAM_TIMES("Net.SSLCertVerificationTimeError", verify_time);
  }

  // We used to remember the intermediate CA certs in the NSS database
  // persistently.  However, NSS opens a connection to the SQLite database
  // during NSS initialization and doesn't close the connection until NSS
  // shuts down.  If the file system where the database resides is gone,
  // the database connection goes bad.  What's worse, the connection won't
  // recover when the file system comes back.  Until this NSS or SQLite bug
  // is fixed, we need to  avoid using the NSS database for non-essential
  // purposes.  See https://bugzilla.mozilla.org/show_bug.cgi?id=508081 and
  // http://crbug.com/15630 for more info.

  // TODO(hclam): Skip logging if server cert was expected to be bad because
  // |server_cert_verify_result_| doesn't contain all the information about
  // the cert.
  if (result == OK)
    LogConnectionTypeMetrics();

  completed_handshake_ = true;

#if defined(OFFICIAL_BUILD) && !defined(OS_ANDROID)
  // Take care of any mandates for public key pinning.
  //
  // Pinning is only enabled for official builds to make sure that others don't
  // end up with pins that cannot be easily updated.
  //
  // TODO(agl): we might have an issue here where a request for foo.example.com
  // merges into a SPDY connection to www.example.com, and gets a different
  // certificate.

  const CertStatus cert_status = server_cert_verify_result_->cert_status;
  if ((result == OK || (IsCertificateError(result) &&
                        IsCertStatusMinorError(cert_status))) &&
      server_cert_verify_result_->is_issued_by_known_root &&
      transport_security_state_) {
    bool sni_available =
        ssl_config_.version_max >= SSL_PROTOCOL_VERSION_TLS1 ||
        ssl_config_.version_fallback;
    const std::string& host = host_and_port_.host();

    TransportSecurityState::DomainState domain_state;
    if (transport_security_state_->GetDomainState(host, sni_available,
                                                  &domain_state) &&
        domain_state.HasPins()) {
      if (!domain_state.IsChainOfPublicKeysPermitted(
               server_cert_verify_result_->public_key_hashes)) {
        const base::Time build_time = base::GetBuildTime();
        // Pins are not enforced if the build is sufficiently old. Chrome
        // users should get updates every six weeks or so, but it's possible
        // that some users will stop getting updates for some reason. We
        // don't want those users building up as a pool of people with bad
        // pins.
        if ((base::Time::Now() - build_time).InDays() < 70 /* 10 weeks */) {
          result = ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN;
          UMA_HISTOGRAM_BOOLEAN("Net.PublicKeyPinSuccess", false);
          TransportSecurityState::ReportUMAOnPinFailure(host);
        }
      } else {
        UMA_HISTOGRAM_BOOLEAN("Net.PublicKeyPinSuccess", true);
      }
    }
  }
#endif

  // Exit DoHandshakeLoop and return the result to the caller to Connect.
  DCHECK_EQ(STATE_NONE, next_handshake_state_);
  return result;
}

void SSLClientSocketNSS::LogConnectionTypeMetrics() const {
  UpdateConnectionTypeHistograms(CONNECTION_SSL);
  if (server_cert_verify_result_->has_md5)
    UpdateConnectionTypeHistograms(CONNECTION_SSL_MD5);
  if (server_cert_verify_result_->has_md2)
    UpdateConnectionTypeHistograms(CONNECTION_SSL_MD2);
  if (server_cert_verify_result_->has_md4)
    UpdateConnectionTypeHistograms(CONNECTION_SSL_MD4);
  if (server_cert_verify_result_->has_md5_ca)
    UpdateConnectionTypeHistograms(CONNECTION_SSL_MD5_CA);
  if (server_cert_verify_result_->has_md2_ca)
    UpdateConnectionTypeHistograms(CONNECTION_SSL_MD2_CA);
  int ssl_version = SSLConnectionStatusToVersion(
      core_->state().ssl_connection_status);
  switch (ssl_version) {
    case SSL_CONNECTION_VERSION_SSL2:
      UpdateConnectionTypeHistograms(CONNECTION_SSL_SSL2);
      break;
    case SSL_CONNECTION_VERSION_SSL3:
      UpdateConnectionTypeHistograms(CONNECTION_SSL_SSL3);
      break;
    case SSL_CONNECTION_VERSION_TLS1:
      UpdateConnectionTypeHistograms(CONNECTION_SSL_TLS1);
      break;
    case SSL_CONNECTION_VERSION_TLS1_1:
      UpdateConnectionTypeHistograms(CONNECTION_SSL_TLS1_1);
      break;
    case SSL_CONNECTION_VERSION_TLS1_2:
      UpdateConnectionTypeHistograms(CONNECTION_SSL_TLS1_2);
      break;
  };
}

// SaveSSLHostInfo saves the certificate chain of the connection so that we can
// start verification faster in the future.
void SSLClientSocketNSS::SaveSSLHostInfo() {
  if (!ssl_host_info_.get())
    return;

  // If the SSLHostInfo hasn't managed to load from disk yet then we can't save
  // anything.
  if (ssl_host_info_->WaitForDataReady(net::CompletionCallback()) != OK)
    return;

  SSLHostInfo::State* state = ssl_host_info_->mutable_state();

  state->certs.clear();
  const PeerCertificateChain& certs = core_->state().server_cert_chain;
  for (unsigned i = 0; i < certs.size(); i++) {
    if (certs[i] == NULL ||
        certs[i]->derCert.len > std::numeric_limits<uint16>::max()) {
      return;
    }

    state->certs.push_back(std::string(
          reinterpret_cast<char*>(certs[i]->derCert.data),
          certs[i]->derCert.len));
  }

  ssl_host_info_->Persist();
}

void SSLClientSocketNSS::EnsureThreadIdAssigned() const {
  base::AutoLock auto_lock(lock_);
  if (valid_thread_id_ != base::kInvalidThreadId)
    return;
  valid_thread_id_ = base::PlatformThread::CurrentId();
}

bool SSLClientSocketNSS::CalledOnValidThread() const {
  EnsureThreadIdAssigned();
  base::AutoLock auto_lock(lock_);
  return valid_thread_id_ == base::PlatformThread::CurrentId();
}

ServerBoundCertService* SSLClientSocketNSS::GetServerBoundCertService() const {
  return server_bound_cert_service_;
}

}  // namespace net
