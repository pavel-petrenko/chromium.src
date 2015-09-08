// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/proximity_auth/wire_message.h"

#include <limits>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/macros.h"
#include "base/values.h"
#include "components/proximity_auth/cryptauth/base64url.h"
#include "components/proximity_auth/logging/logging.h"

// The wire messages have a simple format:
// [ message version ] [ body length ] [ JSON body ]
//       1 byte            2 bytes      body length
// The JSON body contains two fields: an optional permit_id field and a required
// data field.

namespace proximity_auth {
namespace {

// The length of the message header, in bytes.
const size_t kHeaderLength = 3;

// The protocol version of the message format.
const int kMessageFormatVersionThree = 3;

const char kPayloadKey[] = "payload";
const char kPermitIdKey[] = "permit_id";

// Parses the |serialized_message|'s header. Returns |true| iff the message has
// a valid header, is complete, and is well-formed according to the header. Sets
// |is_incomplete_message| to true iff the message does not have enough data to
// parse the header, or if the message length encoded in the message header
// exceeds the size of the |serialized_message|.
bool ParseHeader(const std::string& serialized_message,
                 bool* is_incomplete_message) {
  *is_incomplete_message = false;
  if (serialized_message.size() < kHeaderLength) {
    *is_incomplete_message = true;
    return false;
  }

  static_assert(kHeaderLength > 2, "kHeaderLength too small");
  size_t version = serialized_message[0];
  if (version != kMessageFormatVersionThree) {
    PA_LOG(WARNING) << "Error: Invalid message version. Got " << version
                    << ", expected " << kMessageFormatVersionThree;
    return false;
  }

  uint16_t expected_body_length =
      (static_cast<uint8_t>(serialized_message[1]) << 8) |
      (static_cast<uint8_t>(serialized_message[2]) << 0);
  size_t expected_message_length = kHeaderLength + expected_body_length;
  if (serialized_message.size() < expected_message_length) {
    *is_incomplete_message = true;
    return false;
  }
  if (serialized_message.size() != expected_message_length) {
    PA_LOG(WARNING) << "Error: Invalid message length. Got "
                    << serialized_message.size() << ", expected "
                    << expected_message_length;
    return false;
  }

  return true;
}

}  // namespace

WireMessage::~WireMessage() {
}

// static
scoped_ptr<WireMessage> WireMessage::Deserialize(
    const std::string& serialized_message,
    bool* is_incomplete_message) {
  if (!ParseHeader(serialized_message, is_incomplete_message))
    return scoped_ptr<WireMessage>();

  scoped_ptr<base::Value> body_value =
      base::JSONReader::Read(serialized_message.substr(kHeaderLength));
  if (!body_value || !body_value->IsType(base::Value::TYPE_DICTIONARY)) {
    PA_LOG(WARNING) << "Error: Unable to parse message as JSON.";
    return scoped_ptr<WireMessage>();
  }

  base::DictionaryValue* body;
  bool success = body_value->GetAsDictionary(&body);
  DCHECK(success);

  // The permit ID is optional. In the Easy Unlock protocol, only the first
  // message includes this field.
  std::string permit_id;
  body->GetString(kPermitIdKey, &permit_id);

  std::string payload_base64;
  if (!body->GetString(kPayloadKey, &payload_base64) ||
      payload_base64.empty()) {
    PA_LOG(WARNING) << "Error: Missing payload.";
    return scoped_ptr<WireMessage>();
  }

  std::string payload;
  if (!Base64UrlDecode(payload_base64, &payload)) {
    PA_LOG(WARNING) << "Error: Invalid base64 encoding for payload.";
    return scoped_ptr<WireMessage>();
  }

  return make_scoped_ptr(new WireMessage(payload, permit_id));
}

std::string WireMessage::Serialize() const {
  if (payload_.empty()) {
    PA_LOG(ERROR) << "Failed to serialize empty wire message.";
    return std::string();
  }

  // Create JSON body containing permit id and payload.
  base::DictionaryValue body;
  if (!permit_id_.empty())
    body.SetString(kPermitIdKey, permit_id_);

  std::string base64_payload;
  Base64UrlEncode(payload_, &base64_payload);
  body.SetString(kPayloadKey, base64_payload);

  std::string json_body;
  if (!base::JSONWriter::Write(body, &json_body)) {
    PA_LOG(ERROR) << "Failed to convert WireMessage body to JSON: " << body;
    return std::string();
  }

  // Create header containing version and payload size.
  size_t body_size = json_body.size();
  if (body_size > std::numeric_limits<uint16_t>::max()) {
    PA_LOG(ERROR) << "Can not create WireMessage because body size exceeds "
                  << "16-bit unsigned integer: " << body_size;
    return std::string();
  }

  uint8_t header[] = {
      static_cast<uint8_t>(kMessageFormatVersionThree),
      static_cast<uint8_t>((body_size >> 8) & 0xFF),
      static_cast<uint8_t>(body_size & 0xFF),
  };
  static_assert(sizeof(header) == kHeaderLength, "Malformed header.");

  std::string header_string(kHeaderLength, 0);
  std::memcpy(&header_string[0], header, kHeaderLength);
  return header_string + json_body;
}

WireMessage::WireMessage(const std::string& payload)
    : WireMessage(payload, std::string()) {}

WireMessage::WireMessage(const std::string& payload,
                         const std::string& permit_id)
    : payload_(payload), permit_id_(permit_id) {}

}  // namespace proximity_auth
