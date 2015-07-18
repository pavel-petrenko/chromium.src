// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/cert/internal/signature_algorithm.h"

#include "base/files/file_util.h"
#include "base/strings/string_number_conversions.h"
#include "net/base/test_data_directory.h"
#include "net/cert/pem_tokenizer.h"
#include "net/der/input.h"
#include "net/der/parser.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

// Creates a SignatureAlgorithm given the DER as a byte array. Returns true on
// success and fills |*out| with a non-null pointer.
template <size_t N>
bool ParseDer(const uint8_t(&data)[N], scoped_ptr<SignatureAlgorithm>* out) {
  *out = SignatureAlgorithm::CreateFromDer(der::Input(data, N));
  return *out;
}

// Parses a SignatureAlgorithm given an empty DER input.
TEST(SignatureAlgorithmTest, ParseDer_Empty) {
  scoped_ptr<SignatureAlgorithm> algorithm =
      SignatureAlgorithm::CreateFromDer(der::Input());
  ASSERT_FALSE(algorithm);
}

// Parses a SignatureAlgorithm given invalid DER input.
TEST(SignatureAlgorithmTest, ParseDer_Bogus) {
  const uint8_t kData[] = {0x00};
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a sha1WithRSAEncryption which contains a NULL parameters field.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.5
//       NULL
TEST(SignatureAlgorithmTest, ParseDer_sha1WithRSAEncryption_NullParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0D,  // SEQUENCE (13 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x05,
      0x05, 0x00,  // NULL (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_TRUE(ParseDer(kData, &algorithm));

  EXPECT_EQ(SignatureAlgorithmId::RsaPkcs1, algorithm->algorithm());
  EXPECT_EQ(DigestAlgorithm::Sha1, algorithm->digest());
}

// Parses a sha1WithRSAEncryption which contains no parameters field.
//
//   SEQUENCE (1 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.5
TEST(SignatureAlgorithmTest, ParseDer_sha1WithRSAEncryption_NoParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0B,  // SEQUENCE (11 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x05,
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a sha1WithRSAEncryption which contains an unexpected parameters
// field. Instead of being NULL it is an integer.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.5
//       INTEGER  0
TEST(SignatureAlgorithmTest, ParseDer_sha1WithRSAEncryption_NonNullParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0E,  // SEQUENCE (14 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x05,
      0x02, 0x01, 0x00,  // INTEGER (1 byte)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a sha1WithRSASignature which contains a NULL parameters field.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.3.14.3.2.29
//       NULL
TEST(SignatureAlgorithmTest, ParseDer_sha1WithRSASignature_NullParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x09,  // SEQUENCE (9 bytes)
      0x06, 0x05,  // OBJECT IDENTIFIER (5 bytes)
      0x2b, 0x0e, 0x03, 0x02, 0x1d,
      0x05, 0x00,  // NULL (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_TRUE(ParseDer(kData, &algorithm));

  EXPECT_EQ(SignatureAlgorithmId::RsaPkcs1, algorithm->algorithm());
  EXPECT_EQ(DigestAlgorithm::Sha1, algorithm->digest());
}

// Parses a sha1WithRSASignature which contains no parameters field.
//
//   SEQUENCE (1 elem)
//       OBJECT IDENTIFIER  1.3.14.3.2.29
TEST(SignatureAlgorithmTest, ParseDer_sha1WithRSASignature_NoParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x07,  // SEQUENCE (7 bytes)
      0x06, 0x05,  // OBJECT IDENTIFIER (5 bytes)
      0x2b, 0x0e, 0x03, 0x02, 0x1d,
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a sha1WithRSAEncryption which contains values after the sequence.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.5
//       NULL
//   INTEGER  0
TEST(SignatureAlgorithmTest, ParseDer_sha1WithRsaEncryption_DataAfterSequence) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0D,  // SEQUENCE (13 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x05,
      0x05, 0x00,  // NULL (0 bytes)
      0x02, 0x01, 0x00,  // INTEGER (1 byte)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a sha1WithRSAEncryption which contains a bad NULL parameters field.
// Normally NULL is encoded as {0x05, 0x00} (tag for NULL and length of 0). Here
// NULL is encoded as having a length of 1 instead, followed by data 0x09.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.5
//       NULL
TEST(SignatureAlgorithmTest, ParseDer_sha1WithRSAEncryption_BadNullParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0E,  // SEQUENCE (13 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x05,
      0x05, 0x01, 0x09,  // NULL (1 byte)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a sha1WithRSAEncryption which contains a NULL parameters field,
// followed by an integer.
//
//   SEQUENCE (3 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.5
//       NULL
//       INTEGER  0
TEST(SignatureAlgorithmTest,
     ParseDer_sha1WithRSAEncryption_NullParamsThenInteger) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x10,  // SEQUENCE (16 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x05,
      0x05, 0x00,  // NULL (0 bytes)
      0x02, 0x01, 0x00,  // INTEGER (1 byte)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a SignatureAlgorithm given DER which does not encode a sequence.
//
//   INTEGER 0
TEST(SignatureAlgorithmTest, ParseDer_NotASequence) {
  // clang-format off
  const uint8_t kData[] = {
      0x02, 0x01, 0x00,  // INTEGER (1 byte)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a sha256WithRSAEncryption which contains a NULL parameters field.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.11
//       NULL
TEST(SignatureAlgorithmTest, ParseDer_sha256WithRSAEncryption_NullParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0D,  // SEQUENCE (13 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b,
      0x05, 0x00,  // NULL (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_TRUE(ParseDer(kData, &algorithm));

  EXPECT_EQ(SignatureAlgorithmId::RsaPkcs1, algorithm->algorithm());
  EXPECT_EQ(DigestAlgorithm::Sha256, algorithm->digest());
}

// Parses a sha256WithRSAEncryption which contains no parameters field.
//
//   SEQUENCE (1 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.11
TEST(SignatureAlgorithmTest, ParseDer_sha256WithRSAEncryption_NoParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0B,  // SEQUENCE (11 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b,
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a sha384WithRSAEncryption which contains a NULL parameters field.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.12
//       NULL
TEST(SignatureAlgorithmTest, ParseDer_sha384WithRSAEncryption_NullParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0D,  // SEQUENCE (13 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0c,
      0x05, 0x00,  // NULL (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_TRUE(ParseDer(kData, &algorithm));

  EXPECT_EQ(SignatureAlgorithmId::RsaPkcs1, algorithm->algorithm());
  EXPECT_EQ(DigestAlgorithm::Sha384, algorithm->digest());
}

// Parses a sha384WithRSAEncryption which contains no parameters field.
//
//   SEQUENCE (1 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.12
TEST(SignatureAlgorithmTest, ParseDer_sha384WithRSAEncryption_NoParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0B,  // SEQUENCE (11 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0c,
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a sha512WithRSAEncryption which contains a NULL parameters field.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.13
//       NULL
TEST(SignatureAlgorithmTest, ParseDer_sha512WithRSAEncryption_NullParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0D,  // SEQUENCE (13 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0d,
      0x05, 0x00,  // NULL (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_TRUE(ParseDer(kData, &algorithm));

  EXPECT_EQ(SignatureAlgorithmId::RsaPkcs1, algorithm->algorithm());
  EXPECT_EQ(DigestAlgorithm::Sha512, algorithm->digest());
}

// Parses a sha512WithRSAEncryption which contains no parameters field.
//
//   SEQUENCE (1 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.13
TEST(SignatureAlgorithmTest, ParseDer_sha512WithRSAEncryption_NoParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0B,  // SEQUENCE (11 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0d,
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a sha224WithRSAEncryption which contains a NULL parameters field.
// This fails because the parsing code does not enumerate this OID (even though
// it is in fact valid).
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.14
//       NULL
TEST(SignatureAlgorithmTest, ParseDer_sha224WithRSAEncryption_NullParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0D,  // SEQUENCE (13 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0e,
      0x05, 0x00,  // NULL (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a ecdsa-with-SHA1 which contains no parameters field.
//
//   SEQUENCE (1 elem)
//       OBJECT IDENTIFIER  1.2.840.10045.4.1
TEST(SignatureAlgorithmTest, ParseDer_ecdsaWithSHA1_NoParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x09,  // SEQUENCE (9 bytes)
      0x06, 0x07,  // OBJECT IDENTIFIER (7 bytes)
      0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x01,
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_TRUE(ParseDer(kData, &algorithm));

  EXPECT_EQ(SignatureAlgorithmId::Ecdsa, algorithm->algorithm());
  EXPECT_EQ(DigestAlgorithm::Sha1, algorithm->digest());
}

// Parses a ecdsa-with-SHA1 which contains a NULL parameters field.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.10045.4.1
//       NULL
TEST(SignatureAlgorithmTest, ParseDer_ecdsaWithSHA1_NullParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0B,  // SEQUENCE (11 bytes)
      0x06, 0x07,  // OBJECT IDENTIFIER (7 bytes)
      0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x01,
      0x05, 0x00,  // NULL (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a ecdsa-with-SHA256 which contains no parameters field.
//
//   SEQUENCE (1 elem)
//       OBJECT IDENTIFIER  1.2.840.10045.4.3.2
TEST(SignatureAlgorithmTest, ParseDer_ecdsaWithSHA256_NoParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0A,  // SEQUENCE (10 bytes)
      0x06, 0x08,  // OBJECT IDENTIFIER (8 bytes)
      0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02,
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_TRUE(ParseDer(kData, &algorithm));

  EXPECT_EQ(SignatureAlgorithmId::Ecdsa, algorithm->algorithm());
  EXPECT_EQ(DigestAlgorithm::Sha256, algorithm->digest());
}

// Parses a ecdsa-with-SHA256 which contains a NULL parameters field.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.10045.4.3.2
//       NULL
TEST(SignatureAlgorithmTest, ParseDer_ecdsaWithSHA256_NullParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0C,  // SEQUENCE (12 bytes)
      0x06, 0x08,  // OBJECT IDENTIFIER (8 bytes)
      0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02,
      0x05, 0x00,  // NULL (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a ecdsa-with-SHA384 which contains no parameters field.
//
//   SEQUENCE (1 elem)
//       OBJECT IDENTIFIER  1.2.840.10045.4.3.3
TEST(SignatureAlgorithmTest, ParseDer_ecdsaWithSHA384_NoParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0A,  // SEQUENCE (10 bytes)
      0x06, 0x08,  // OBJECT IDENTIFIER (8 bytes)
      0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x03,
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_TRUE(ParseDer(kData, &algorithm));

  EXPECT_EQ(SignatureAlgorithmId::Ecdsa, algorithm->algorithm());
  EXPECT_EQ(DigestAlgorithm::Sha384, algorithm->digest());
}

// Parses a ecdsa-with-SHA384 which contains a NULL parameters field.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.10045.4.3.3
//       NULL
TEST(SignatureAlgorithmTest, ParseDer_ecdsaWithSHA384_NullParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0C,  // SEQUENCE (12 bytes)
      0x06, 0x08,  // OBJECT IDENTIFIER (8 bytes)
      0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x03,
      0x05, 0x00,  // NULL (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a ecdsa-with-SHA512 which contains no parameters field.
//
//   SEQUENCE (1 elem)
//       OBJECT IDENTIFIER  1.2.840.10045.4.3.4
TEST(SignatureAlgorithmTest, ParseDer_ecdsaWithSHA512_NoParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0A,  // SEQUENCE (10 bytes)
      0x06, 0x08,  // OBJECT IDENTIFIER (8 bytes)
      0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x04,
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_TRUE(ParseDer(kData, &algorithm));

  EXPECT_EQ(SignatureAlgorithmId::Ecdsa, algorithm->algorithm());
  EXPECT_EQ(DigestAlgorithm::Sha512, algorithm->digest());
}

// Parses a ecdsa-with-SHA512 which contains a NULL parameters field.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.10045.4.3.4
//       NULL
TEST(SignatureAlgorithmTest, ParseDer_ecdsaWithSHA512_NullParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0C,  // SEQUENCE (12 bytes)
      0x06, 0x08,  // OBJECT IDENTIFIER (8 bytes)
      0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x04,
      0x05, 0x00,  // NULL (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Tests that two RSA algorithms with different digests are not equal.
TEST(SignatureAlgorithmTest, Equals_RsaWithDifferentDigest) {
  scoped_ptr<SignatureAlgorithm> alg1 =
      SignatureAlgorithm::CreateRsaPkcs1(DigestAlgorithm::Sha1);

  scoped_ptr<SignatureAlgorithm> alg2 =
      SignatureAlgorithm::CreateRsaPkcs1(DigestAlgorithm::Sha256);

  ASSERT_FALSE(alg1->Equals(*alg2));
}

// Tests that two ECDSA algorithms with different digests are not equal.
TEST(SignatureAlgorithmTest, Equals_EcdsaWithDifferentDigest) {
  scoped_ptr<SignatureAlgorithm> alg1 =
      SignatureAlgorithm::CreateEcdsa(DigestAlgorithm::Sha1);

  scoped_ptr<SignatureAlgorithm> alg2 =
      SignatureAlgorithm::CreateEcdsa(DigestAlgorithm::Sha256);

  ASSERT_FALSE(alg1->Equals(*alg2));
}

// Tests that an ECDSA algorithm is not equal to an RSA algorithm (even though
// digests match).
TEST(SignatureAlgorithmTest, Equals_EcdsaNotEqualRsa) {
  scoped_ptr<SignatureAlgorithm> alg1 =
      SignatureAlgorithm::CreateEcdsa(DigestAlgorithm::Sha256);

  scoped_ptr<SignatureAlgorithm> alg2 =
      SignatureAlgorithm::CreateRsaPkcs1(DigestAlgorithm::Sha256);

  ASSERT_FALSE(alg1->Equals(*alg2));
}

// Tests that two identical ECDSA algorithms are equal - both use SHA-256.
TEST(SignatureAlgorithmTest, Equals_EcdsaMatch) {
  scoped_ptr<SignatureAlgorithm> alg1 =
      SignatureAlgorithm::CreateEcdsa(DigestAlgorithm::Sha256);

  scoped_ptr<SignatureAlgorithm> alg2 =
      SignatureAlgorithm::CreateEcdsa(DigestAlgorithm::Sha256);

  ASSERT_TRUE(alg1->Equals(*alg2));
}

// Tests that two identical RSA algorithms are equal - both use SHA-512
TEST(SignatureAlgorithmTest, Equals_RsaPkcs1Match) {
  scoped_ptr<SignatureAlgorithm> alg1 =
      SignatureAlgorithm::CreateRsaPkcs1(DigestAlgorithm::Sha512);

  scoped_ptr<SignatureAlgorithm> alg2 =
      SignatureAlgorithm::CreateRsaPkcs1(DigestAlgorithm::Sha512);

  ASSERT_TRUE(alg1->Equals(*alg2));
}

// Tests that two RSASSA-PSS algorithms are equal.
TEST(SignatureAlgorithmTest, Equals_RsaPssMatch) {
  scoped_ptr<SignatureAlgorithm> alg1 = SignatureAlgorithm::CreateRsaPss(
      DigestAlgorithm::Sha256, DigestAlgorithm::Sha1, 21);

  scoped_ptr<SignatureAlgorithm> alg2 = SignatureAlgorithm::CreateRsaPss(
      DigestAlgorithm::Sha256, DigestAlgorithm::Sha1, 21);

  ASSERT_TRUE(alg1->Equals(*alg2));
}

// Tests that two RSASSA-PSS algorithms with different hashes are not equal.
TEST(SignatureAlgorithmTest, Equals_RsaPssWithDifferentDigest) {
  scoped_ptr<SignatureAlgorithm> alg1 = SignatureAlgorithm::CreateRsaPss(
      DigestAlgorithm::Sha1, DigestAlgorithm::Sha1, 20);

  scoped_ptr<SignatureAlgorithm> alg2 = SignatureAlgorithm::CreateRsaPss(
      DigestAlgorithm::Sha256, DigestAlgorithm::Sha1, 20);

  ASSERT_FALSE(alg1->Equals(*alg2));
}

// Tests that two RSASSA-PSS algorithms with different mask gens are not equal.
TEST(SignatureAlgorithmTest, Equals_RsaPssWithDifferentMaskGen) {
  scoped_ptr<SignatureAlgorithm> alg1 = SignatureAlgorithm::CreateRsaPss(
      DigestAlgorithm::Sha256, DigestAlgorithm::Sha1, 20);

  scoped_ptr<SignatureAlgorithm> alg2 = SignatureAlgorithm::CreateRsaPss(
      DigestAlgorithm::Sha256, DigestAlgorithm::Sha256, 20);

  ASSERT_FALSE(alg1->Equals(*alg2));
}

// Tests that two RSASSA-PSS algorithms with different salts
TEST(SignatureAlgorithmTest, Equals_RsaPssWithDifferentSalt) {
  scoped_ptr<SignatureAlgorithm> alg1 = SignatureAlgorithm::CreateRsaPss(
      DigestAlgorithm::Sha1, DigestAlgorithm::Sha1, 20);

  scoped_ptr<SignatureAlgorithm> alg2 = SignatureAlgorithm::CreateRsaPss(
      DigestAlgorithm::Sha1, DigestAlgorithm::Sha1, 16);

  ASSERT_FALSE(alg1->Equals(*alg2));
}

// Tests that the parmeters returned for an ECDSA algorithm are null for
// non-ECDSA algorithms.
TEST(SignatureAlgorithmTest, ParamsAreNullForWrongType_Ecdsa) {
  scoped_ptr<SignatureAlgorithm> alg1 =
      SignatureAlgorithm::CreateEcdsa(DigestAlgorithm::Sha1);

  EXPECT_FALSE(alg1->ParamsForRsaPss());
}

// Tests that the parmeters returned for an RSA PKCS#1 v1.5 algorithm are null
// for non-RSA PKCS#1 v1.5 algorithms.
TEST(SignatureAlgorithmTest, ParamsAreNullForWrongType_RsaPkcs1) {
  scoped_ptr<SignatureAlgorithm> alg1 =
      SignatureAlgorithm::CreateRsaPkcs1(DigestAlgorithm::Sha1);

  EXPECT_FALSE(alg1->ParamsForRsaPss());
}

// Parses a rsaPss algorithm that uses SHA1 and a salt length of 20.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.10
//       SEQUENCE (4 elem)
//           [0] (1 elem)
//               SEQUENCE (2 elem)
//                   OBJECT IDENTIFIER  1.3.14.3.2.26
//                   NULL
//           [1] (1 elem)
//               SEQUENCE (2 elem)
//                   OBJECT IDENTIFIER  1.2.840.113549.1.1.8
//                   SEQUENCE (2 elem)
//                       OBJECT IDENTIFIER  1.3.14.3.2.26
//                       NULL
//           [2] (1 elem)
//               INTEGER  20
//           [3] (1 elem)
//               INTEGER  1
TEST(SignatureAlgorithmTest, ParseDer_rsaPss) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x3E,  // SEQUENCE (62 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A,
      0x30, 0x31,  // SEQUENCE (49 bytes)
      0xA0, 0x0B,  // [0] (11 bytes)
      0x30, 0x09,  // SEQUENCE (9 bytes)
      0x06, 0x05,  // OBJECT IDENTIFIER (5 bytes)
      0x2B, 0x0E, 0x03, 0x02, 0x1A,
      0x05, 0x00,  // NULL (0 bytes)
      0xA1, 0x18,  // [1] (24 bytes)
      0x30, 0x16,  // SEQUENCE (22 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x08,
      0x30, 0x09,  // SEQUENCE (9 bytes)
      0x06, 0x05,  // OBJECT IDENTIFIER (5 bytes)
      0x2B, 0x0E, 0x03, 0x02, 0x1A,
      0x05, 0x00,  // NULL (0 bytes)
      0xA2, 0x03,  // [2] (3 bytes)
      0x02, 0x01,  // INTEGER (1 byte)
      0x14,
      0xA3, 0x03,  // [3] (3 bytes)
      0x02, 0x01,  // INTEGER (1 byte)
      0x01,
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_TRUE(ParseDer(kData, &algorithm));

  ASSERT_EQ(SignatureAlgorithmId::RsaPss, algorithm->algorithm());
  EXPECT_EQ(DigestAlgorithm::Sha1, algorithm->digest());

  const RsaPssParameters* params = algorithm->ParamsForRsaPss();

  ASSERT_TRUE(params);
  EXPECT_EQ(DigestAlgorithm::Sha1, params->mgf1_hash());
  EXPECT_EQ(20u, params->salt_length());
}

// Parses a rsaPss algorithm that has an empty parameters. It should use all the
// default values (SHA1 and salt length of 20).
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.10
//       SEQUENCE (0 elem)
TEST(SignatureAlgorithmTest, ParseDer_rsaPss_EmptyParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0D,  // SEQUENCE (13 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A,
      0x30, 0x00,  // SEQUENCE (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_TRUE(ParseDer(kData, &algorithm));

  ASSERT_EQ(SignatureAlgorithmId::RsaPss, algorithm->algorithm());
  EXPECT_EQ(DigestAlgorithm::Sha1, algorithm->digest());

  const RsaPssParameters* params = algorithm->ParamsForRsaPss();

  ASSERT_TRUE(params);
  EXPECT_EQ(DigestAlgorithm::Sha1, params->mgf1_hash());
  EXPECT_EQ(20u, params->salt_length());
}

// Parses a rsaPss algorithm that has NULL parameters. This fails.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.10
//       NULL
TEST(SignatureAlgorithmTest, ParseDer_rsaPss_NullParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0D,  // SEQUENCE (13 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A,
      0x05, 0x00,  // NULL (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a rsaPss algorithm that has no parameters. This fails.
//
//   SEQUENCE (1 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.10
TEST(SignatureAlgorithmTest, ParseDer_rsaPss_NoParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0B,  // SEQUENCE (11 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A,
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a rsaPss algorithm that has data after the parameters sequence.
//
//   SEQUENCE (3 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.10
//       SEQUENCE (0 elem)
//       NULL
TEST(SignatureAlgorithmTest, ParseDer_rsaPss_DataAfterParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x0F,  // SEQUENCE (15 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A,
      0x30, 0x00,  // SEQUENCE (0 bytes)
      0x05, 0x00,  // NULL (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a rsaPss algorithm that uses defaults (by ommitting the values) for
// everything except the salt length.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.10
//       SEQUENCE (1 elem)
//           [2] (1 elem)
//               INTEGER  23
TEST(SignatureAlgorithmTest, ParseDer_rsaPss_DefaultsExceptForSaltLength) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x12,  // SEQUENCE (62 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A,
      0x30, 0x05,  // SEQUENCE (5 bytes)
      0xA2, 0x03,  // [2] (3 bytes)
      0x02, 0x01,  // INTEGER (1 byte)
      0x17,
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_TRUE(ParseDer(kData, &algorithm));

  ASSERT_EQ(SignatureAlgorithmId::RsaPss, algorithm->algorithm());
  EXPECT_EQ(DigestAlgorithm::Sha1, algorithm->digest());

  const RsaPssParameters* params = algorithm->ParamsForRsaPss();

  ASSERT_TRUE(params);
  EXPECT_EQ(DigestAlgorithm::Sha1, params->mgf1_hash());
  EXPECT_EQ(23u, params->salt_length());
}

// Parses a rsaPss algorithm that has unrecognized data (NULL) within the
// parameters sequence.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.10
//       SEQUENCE (2 elem)
//           [2] (1 elem)
//               INTEGER  23
//           NULL
TEST(SignatureAlgorithmTest, ParseDer_rsaPss_NullInsideParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x14,  // SEQUENCE (62 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A,
      0x30, 0x07,  // SEQUENCE (5 bytes)
      0xA2, 0x03,  // [2] (3 bytes)
      0x02, 0x01,  // INTEGER (1 byte)
      0x17,
      0x05, 0x00,  // NULL (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a rsaPss algorithm that has an unsupported trailer value (2). Only
// trailer values of 1 are allowed by RFC 4055.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.10
//       SEQUENCE (1 elem)
//           [3] (1 elem)
//               INTEGER  2
TEST(SignatureAlgorithmTest, ParseDer_rsaPss_UnsupportedTrailer) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x12,  // SEQUENCE (18 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A,
      0x30, 0x05,  // SEQUENCE (5 bytes)
      0xA3, 0x03,  // [3] (3 bytes)
      0x02, 0x01,  // INTEGER (1 byte)
      0x02,
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a rsaPss algorithm that has extra data appearing after the trailer in
// the [3] section.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.10
//       SEQUENCE (1 elem)
//           [3] (2 elem)
//               INTEGER  1
//               NULL
TEST(SignatureAlgorithmTest, ParseDer_rsaPss_BadTrailer) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x14,  // SEQUENCE (20 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A,
      0x30, 0x07,  // SEQUENCE (7 bytes)
      0xA3, 0x05,  // [3] (5 bytes)
      0x02, 0x01,  // INTEGER (1 byte)
      0x01,
      0x05, 0x00,  // NULL (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a rsaPss algorithm that uses SHA384 for the hash, and leaves the rest
// as defaults (including the mask gen).
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.10
//       SEQUENCE (1 elem)
//           [0] (1 elem)
//               SEQUENCE (2 elem)
//                   OBJECT IDENTIFIER  2.16.840.1.101.3.4.2.2
//                   NULL
TEST(SignatureAlgorithmTest, ParseDer_rsaPss_NonDefaultHash) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x1E,  // SEQUENCE (30 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A,
      0x30, 0x11,  // SEQUENCE (17 bytes)
      0xA0, 0x0F,  // [0] (15 bytes)
      0x30, 0x0D,  // SEQUENCE (13 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02,
      0x05, 0x00,  // NULL (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_TRUE(ParseDer(kData, &algorithm));

  ASSERT_EQ(SignatureAlgorithmId::RsaPss, algorithm->algorithm());
  EXPECT_EQ(DigestAlgorithm::Sha384, algorithm->digest());

  const RsaPssParameters* params = algorithm->ParamsForRsaPss();

  ASSERT_TRUE(params);
  EXPECT_EQ(DigestAlgorithm::Sha1, params->mgf1_hash());
  EXPECT_EQ(20u, params->salt_length());
}

// Parses a rsaPss algorithm that uses SHA384 for the hash, however in the
// AlgorithmIdentifier for the hash function the parameters are omitted instead
// of NULL.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.10
//       SEQUENCE (1 elem)
//           [0] (1 elem)
//               SEQUENCE (1 elem)
//                   OBJECT IDENTIFIER  2.16.840.1.101.3.4.2.2
TEST(SignatureAlgorithmTest, ParseDer_rsaPss_NonDefaultHash_AbsentParams) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x1C,  // SEQUENCE (28 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A,
      0x30, 0x0F,  // SEQUENCE (15 bytes)
      0xA0, 0x0D,  // [0] (13 bytes)
      0x30, 0x0B,  // SEQUENCE (11 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02,
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_TRUE(ParseDer(kData, &algorithm));

  ASSERT_EQ(SignatureAlgorithmId::RsaPss, algorithm->algorithm());
  EXPECT_EQ(DigestAlgorithm::Sha384, algorithm->digest());

  const RsaPssParameters* params = algorithm->ParamsForRsaPss();

  ASSERT_TRUE(params);
  EXPECT_EQ(DigestAlgorithm::Sha1, params->mgf1_hash());
  EXPECT_EQ(20u, params->salt_length());
}

// Parses a rsaPss algorithm that uses an invalid hash algorithm (twiddled the
// bytes for the SHA-384 OID a bit).
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.10
//       SEQUENCE (1 elem)
//           [0] (1 elem)
//               SEQUENCE (1 elem)
//                   OBJECT IDENTIFIER  2.16.840.2.103.19.4.2.2
TEST(SignatureAlgorithmTest, ParseDer_rsaPss_UnsupportedHashOid) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x1C,  // SEQUENCE (28 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A,
      0x30, 0x0F,  // SEQUENCE (15 bytes)
      0xA0, 0x0D,  // [0] (13 bytes)
      0x30, 0x0B,  // SEQUENCE (11 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x60, 0x86, 0x48, 0x02, 0x67, 0x13, 0x04, 0x02, 0x02,
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a rsaPss algorithm that uses SHA512 MGF1 for the mask gen, and
// defaults for the rest.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.10
//       SEQUENCE (1 elem)
//           [1] (1 elem)
//               SEQUENCE (2 elem)
//                   OBJECT IDENTIFIER  1.2.840.113549.1.1.8
//                   SEQUENCE (2 elem)
//                       OBJECT IDENTIFIER  2.16.840.1.101.3.4.2.3
//                       NULL
TEST(SignatureAlgorithmTest, ParseDer_rsaPss_NonDefaultMaskGen) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x2B,  // SEQUENCE (43 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A,
      0x30, 0x1E,  // SEQUENCE (30 bytes)
      0xA1, 0x1C,  // [1] (28 bytes)
      0x30, 0x1A,  // SEQUENCE (26 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x08,
      0x30, 0x0D,  // SEQUENCE (13 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03,
      0x05, 0x00,  // NULL (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_TRUE(ParseDer(kData, &algorithm));

  ASSERT_EQ(SignatureAlgorithmId::RsaPss, algorithm->algorithm());
  EXPECT_EQ(DigestAlgorithm::Sha1, algorithm->digest());

  const RsaPssParameters* params = algorithm->ParamsForRsaPss();

  ASSERT_TRUE(params);
  EXPECT_EQ(DigestAlgorithm::Sha512, params->mgf1_hash());
  EXPECT_EQ(20u, params->salt_length());
}

// Parses a rsaPss algorithm that uses a mask gen with an unrecognized OID
// (twiddled some of the bits).
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.10
//       SEQUENCE (1 elem)
//           [1] (1 elem)
//               SEQUENCE (2 elem)
//                   OBJECT IDENTIFIER  1.2.840.113618.1.2.8
//                   SEQUENCE (2 elem)
//                       OBJECT IDENTIFIER  2.16.840.1.101.3.4.2.3
//                       NULL
TEST(SignatureAlgorithmTest, ParseDer_rsaPss_UnsupportedMaskGen) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x2B,  // SEQUENCE (43 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A,
      0x30, 0x1E,  // SEQUENCE (30 bytes)
      0xA1, 0x1C,  // [1] (28 bytes)
      0x30, 0x1A,  // SEQUENCE (26 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x52, 0x01, 0x02, 0x08,
      0x30, 0x0D,  // SEQUENCE (13 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03,
      0x05, 0x00,  // NULL (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_FALSE(ParseDer(kData, &algorithm));
}

// Parses a rsaPss algorithm that uses SHA256 for the hash, and SHA512 for the
// MGF1.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.10
//       SEQUENCE (2 elem)
//           [0] (1 elem)
//               SEQUENCE (2 elem)
//                   OBJECT IDENTIFIER 2.16.840.1.101.3.4.2.1
//                   NULL
//           [1] (1 elem)
//               SEQUENCE (2 elem)
//                   OBJECT IDENTIFIER  1.2.840.113549.1.1.8
//                   SEQUENCE (2 elem)
//                       OBJECT IDENTIFIER  2.16.840.1.101.3.4.2.3
//                       NULL
TEST(SignatureAlgorithmTest, ParseDer_rsaPss_NonDefaultHashAndMaskGen) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x3C,  // SEQUENCE (60 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A,
      0x30, 0x2F,  // SEQUENCE (47 bytes)
      0xA0, 0x0F,  // [0] (15 bytes)
      0x30, 0x0D,  // SEQUENCE (13 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01,
      0x05, 0x00,  // NULL (0 bytes)
      0xA1, 0x1C,  // [1] (28 bytes)
      0x30, 0x1A,  // SEQUENCE (26 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x08,
      0x30, 0x0D,  // SEQUENCE (13 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03,
      0x05, 0x00,  // NULL (0 bytes)
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_TRUE(ParseDer(kData, &algorithm));

  ASSERT_EQ(SignatureAlgorithmId::RsaPss, algorithm->algorithm());
  EXPECT_EQ(DigestAlgorithm::Sha256, algorithm->digest());

  const RsaPssParameters* params = algorithm->ParamsForRsaPss();

  ASSERT_TRUE(params);
  EXPECT_EQ(DigestAlgorithm::Sha512, params->mgf1_hash());
  EXPECT_EQ(20u, params->salt_length());
}

// Parses a rsaPss algorithm that uses SHA256 for the hash, and SHA256 for the
// MGF1, and a salt length of 10.
//
//   SEQUENCE (2 elem)
//       OBJECT IDENTIFIER  1.2.840.113549.1.1.10
//       SEQUENCE (3 elem)
//           [0] (1 elem)
//               SEQUENCE (2 elem)
//                   OBJECT IDENTIFIER 2.16.840.1.101.3.4.2.1
//                   NULL
//           [1] (1 elem)
//               SEQUENCE (2 elem)
//                   OBJECT IDENTIFIER  1.2.840.113549.1.1.8
//                   SEQUENCE (2 elem)
//                       OBJECT IDENTIFIER  2.16.840.1.101.3.4.2.1
//                       NULL
//           [2] (1 elem)
//               INTEGER  10
TEST(SignatureAlgorithmTest, ParseDer_rsaPss_NonDefaultHashAndMaskGenAndSalt) {
  // clang-format off
  const uint8_t kData[] = {
      0x30, 0x41,  // SEQUENCE (65 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A,
      0x30, 0x34,  // SEQUENCE (52 bytes)
      0xA0, 0x0F,  // [0] (15 bytes)
      0x30, 0x0D,  // SEQUENCE (13 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01,
      0x05, 0x00,  // NULL (0 bytes)
      0xA1, 0x1C,  // [1] (28 bytes)
      0x30, 0x1A,  // SEQUENCE (26 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x08,
      0x30, 0x0D,  // SEQUENCE (13 bytes)
      0x06, 0x09,  // OBJECT IDENTIFIER (9 bytes)
      0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01,
      0x05, 0x00,  // NULL (0 bytes)
      0xA2, 0x03,  // [2] (3 bytes)
      0x02, 0x01,  // INTEGER (1 byte)
      0x0A,
  };
  // clang-format on
  scoped_ptr<SignatureAlgorithm> algorithm;
  ASSERT_TRUE(ParseDer(kData, &algorithm));

  ASSERT_EQ(SignatureAlgorithmId::RsaPss, algorithm->algorithm());
  EXPECT_EQ(DigestAlgorithm::Sha256, algorithm->digest());

  const RsaPssParameters* params = algorithm->ParamsForRsaPss();

  ASSERT_TRUE(params);
  EXPECT_EQ(DigestAlgorithm::Sha256, params->mgf1_hash());
  EXPECT_EQ(10u, params->salt_length());
}

}  // namespace

}  // namespace net
