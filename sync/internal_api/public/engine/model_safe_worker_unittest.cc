// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sync/internal_api/public/engine/model_safe_worker.h"

#include "base/memory/scoped_ptr.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {
namespace {

class ModelSafeWorkerTest : public ::testing::Test {
};

TEST_F(ModelSafeWorkerTest, ModelSafeRoutingInfoToValue) {
  ModelSafeRoutingInfo routing_info;
  routing_info[BOOKMARKS] = GROUP_PASSIVE;
  routing_info[NIGORI] = GROUP_UI;
  routing_info[PREFERENCES] = GROUP_DB;
  DictionaryValue expected_value;
  expected_value.SetString("Bookmarks", "GROUP_PASSIVE");
  expected_value.SetString("Encryption keys", "GROUP_UI");
  expected_value.SetString("Preferences", "GROUP_DB");
  scoped_ptr<DictionaryValue> value(
      ModelSafeRoutingInfoToValue(routing_info));
  EXPECT_TRUE(value->Equals(&expected_value));
}

TEST_F(ModelSafeWorkerTest, ModelSafeRoutingInfoToString) {
  ModelSafeRoutingInfo routing_info;
  routing_info[BOOKMARKS] = GROUP_PASSIVE;
  routing_info[NIGORI] = GROUP_UI;
  routing_info[PREFERENCES] = GROUP_DB;
  EXPECT_EQ(
      "{\"Bookmarks\":\"GROUP_PASSIVE\",\"Encryption keys\":\"GROUP_UI\","
      "\"Preferences\":\"GROUP_DB\"}",
      ModelSafeRoutingInfoToString(routing_info));
}

TEST_F(ModelSafeWorkerTest, GetRoutingInfoTypes) {
  ModelSafeRoutingInfo routing_info;
  routing_info[BOOKMARKS] = GROUP_PASSIVE;
  routing_info[NIGORI] = GROUP_UI;
  routing_info[PREFERENCES] = GROUP_DB;
  const ModelTypeSet expected_types(BOOKMARKS, NIGORI, PREFERENCES);
  EXPECT_TRUE(GetRoutingInfoTypes(routing_info).Equals(expected_types));
}

TEST_F(ModelSafeWorkerTest, ModelSafeRoutingInfoToStateMap) {
  std::string payload = "test";
  ModelSafeRoutingInfo routing_info;
  routing_info[BOOKMARKS] = GROUP_PASSIVE;
  routing_info[NIGORI] = GROUP_UI;
  routing_info[PREFERENCES] = GROUP_DB;
  ModelTypeStateMap type_state_map =
      ModelSafeRoutingInfoToStateMap(routing_info, payload);
  ASSERT_EQ(routing_info.size(), type_state_map.size());
  for (ModelSafeRoutingInfo::iterator iter = routing_info.begin();
       iter != routing_info.end();
       ++iter) {
    EXPECT_EQ(payload, type_state_map[iter->first].payload);
  }
}

}  // namespace
}  // namespace syncer
