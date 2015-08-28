// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/ssl/ssl_platform_key.h"

#include <openssl/digest.h>
#include <openssl/evp.h>

#include "base/logging.h"
#include "base/macros.h"
#include "base/stl_util.h"
#include "crypto/scoped_openssl_types.h"
#include "net/base/net_errors.h"
#include "net/ssl/openssl_client_key_store.h"
#include "net/ssl/ssl_private_key.h"
#include "net/ssl/threaded_ssl_private_key.h"

namespace net {

namespace {

class SSLPlatformKeyAndroid : public ThreadedSSLPrivateKey::Delegate {
 public:
  SSLPlatformKeyAndroid(crypto::ScopedEVP_PKEY key, SSLPrivateKey::Type type)
      : key_(key.Pass()), type_(type) {}

  ~SSLPlatformKeyAndroid() override {}

  SSLPrivateKey::Type GetType() override { return type_; }

  bool SupportsHash(SSLPrivateKey::Hash hash) override { return true; }

  size_t GetMaxSignatureLengthInBytes() override {
    return EVP_PKEY_size(key_.get());
  }

  Error SignDigest(SSLPrivateKey::Hash hash,
                   const base::StringPiece& input,
                   std::vector<uint8_t>* signature) override {
    crypto::ScopedEVP_PKEY_CTX ctx =
        crypto::ScopedEVP_PKEY_CTX(EVP_PKEY_CTX_new(key_.get(), NULL));
    if (!ctx)
      return ERR_SSL_CLIENT_AUTH_SIGNATURE_FAILED;
    if (!EVP_PKEY_sign_init(ctx.get()))
      return ERR_SSL_CLIENT_AUTH_SIGNATURE_FAILED;

    if (type_ == SSLPrivateKey::Type::RSA) {
      const EVP_MD* digest = nullptr;
      switch (hash) {
        case SSLPrivateKey::Hash::MD5_SHA1:
          digest = EVP_md5_sha1();
          break;
        case SSLPrivateKey::Hash::SHA1:
          digest = EVP_sha1();
          break;
        case SSLPrivateKey::Hash::SHA256:
          digest = EVP_sha256();
          break;
        case SSLPrivateKey::Hash::SHA384:
          digest = EVP_sha384();
          break;
        case SSLPrivateKey::Hash::SHA512:
          digest = EVP_sha512();
          break;
        default:
          return ERR_SSL_CLIENT_AUTH_SIGNATURE_FAILED;
      }
      DCHECK(digest);
      if (!EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_PADDING))
        return ERR_SSL_CLIENT_AUTH_SIGNATURE_FAILED;
      if (!EVP_PKEY_CTX_set_signature_md(ctx.get(), digest))
        return ERR_SSL_CLIENT_AUTH_SIGNATURE_FAILED;
    }

    uint8_t* input_ptr =
        const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(input.data()));
    size_t input_len = input.size();
    size_t sig_len = 0;
    if (!EVP_PKEY_sign(ctx.get(), NULL, &sig_len, input_ptr, input_len))
      return ERR_SSL_CLIENT_AUTH_SIGNATURE_FAILED;
    signature->resize(sig_len);
    uint8_t* sig = const_cast<uint8_t*>(
        reinterpret_cast<const uint8_t*>(vector_as_array(signature)));
    if (!sig)
      return ERR_SSL_CLIENT_AUTH_SIGNATURE_FAILED;
    if (!EVP_PKEY_sign(ctx.get(), sig, &sig_len, input_ptr, input_len))
      return ERR_SSL_CLIENT_AUTH_SIGNATURE_FAILED;

    signature->resize(sig_len);

    return OK;
  }

 private:
  crypto::ScopedEVP_PKEY key_;
  SSLPrivateKey::Type type_;

  DISALLOW_COPY_AND_ASSIGN(SSLPlatformKeyAndroid);
};

}  // namespace

scoped_ptr<SSLPrivateKey> FetchClientCertPrivateKey(
    X509Certificate* certificate,
    scoped_refptr<base::SequencedTaskRunner> task_runner) {
  crypto::ScopedEVP_PKEY key =
      OpenSSLClientKeyStore::GetInstance()->FetchClientCertPrivateKey(
          certificate);
  if (!key)
    return nullptr;

  SSLPrivateKey::Type type;
  switch (EVP_PKEY_id(key.get())) {
    case EVP_PKEY_RSA:
      type = SSLPrivateKey::Type::RSA;
      break;
    case EVP_PKEY_EC:
      type = SSLPrivateKey::Type::ECDSA;
      break;
    default:
      LOG(ERROR) << "Unknown key type: " << EVP_PKEY_id(key.get());
      return nullptr;
  }
  return make_scoped_ptr(new ThreadedSSLPrivateKey(
      make_scoped_ptr(new SSLPlatformKeyAndroid(key.Pass(), type)),
      task_runner.Pass()));
}

}  // namespace net
