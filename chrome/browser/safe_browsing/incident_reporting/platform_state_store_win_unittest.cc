// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/incident_reporting/platform_state_store.h"

#include "base/prefs/pref_notifier_impl.h"
#include "base/prefs/testing_pref_store.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/test_reg_util_win.h"
#include "base/test/test_simple_task_runner.h"
#include "base/thread_task_runner_handle.h"
#include "base/win/registry.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/syncable_prefs/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace safe_browsing {
namespace platform_state_store {

namespace {

const char kTestData_[] = "comme un poisson";
const DWORD kTestDataSize_ = sizeof(kTestData_) - 1;

}  // namespace

class PlatformStateStoreWinTest : public ::testing::Test {
 protected:
  PlatformStateStoreWinTest()
      : profile_(nullptr),
        task_runner_(new base::TestSimpleTaskRunner()),
        thread_task_runner_handle_(task_runner_),
        profile_manager_(TestingBrowserProcess::GetGlobal()) {}

  void SetUp() override {
    ::testing::Test::SetUp();
    registry_override_manager_.OverrideRegistry(HKEY_CURRENT_USER);
    ASSERT_TRUE(profile_manager_.SetUp());
  }

  // Creates/resets |profile_|. If |new_profile| is true, the profile will
  // believe that it is new (Profile::IsNewProfile() will return true).
  void ResetProfile(bool new_profile) {
    if (profile_) {
      profile_manager_.DeleteTestingProfile(kProfileName_);
      profile_ = nullptr;
    }
    // Create a profile with a user PrefStore that can be manipulated.
    TestingPrefStore* user_pref_store = new TestingPrefStore();
    // Profile::IsNewProfile() returns true/false on the basis of the pref
    // store's read_error property. A profile is considered "New" if it didn't
    // have a user prefs file.
    user_pref_store->set_read_error(
        new_profile ? PersistentPrefStore::PREF_READ_ERROR_NO_FILE
                    : PersistentPrefStore::PREF_READ_ERROR_NONE);
    // Ownership of |user_pref_store| is passed to the service.
    scoped_ptr<TestingPrefServiceSyncable> prefs(new TestingPrefServiceSyncable(
        new TestingPrefStore(), user_pref_store, new TestingPrefStore(),
        new user_prefs::PrefRegistrySyncable(), new PrefNotifierImpl()));
    chrome::RegisterUserProfilePrefs(prefs->registry());
    profile_ = profile_manager_.CreateTestingProfile(
        kProfileName_, prefs.Pass(), base::UTF8ToUTF16(kProfileName_), 0,
        std::string(), TestingProfile::TestingFactories());
    if (new_profile)
      ASSERT_TRUE(profile_->IsNewProfile());
    else
      ASSERT_FALSE(profile_->IsNewProfile());
  }

  void WriteTestData() {
    base::win::RegKey key;
    ASSERT_EQ(ERROR_SUCCESS, key.Create(HKEY_CURRENT_USER, kStoreKeyName_,
                                        KEY_SET_VALUE | KEY_WOW64_32KEY));
    ASSERT_EQ(ERROR_SUCCESS,
              key.WriteValue(base::UTF8ToUTF16(kProfileName_).c_str(),
                             &kTestData_[0], kTestDataSize_, REG_BINARY));
  }

  void AssertTestDataIsAbsent() {
    base::win::RegKey key;
    ASSERT_EQ(ERROR_SUCCESS, key.Open(HKEY_CURRENT_USER, kStoreKeyName_,
                                      KEY_QUERY_VALUE | KEY_WOW64_32KEY));
    ASSERT_FALSE(key.HasValue(base::UTF8ToUTF16(kProfileName_).c_str()));
  }

  void AssertTestDataIsPresent() {
    char buffer[kTestDataSize_] = {};
    base::win::RegKey key;
    ASSERT_EQ(ERROR_SUCCESS, key.Open(HKEY_CURRENT_USER, kStoreKeyName_,
                                      KEY_QUERY_VALUE | KEY_WOW64_32KEY));
    DWORD data_size = kTestDataSize_;
    DWORD data_type = REG_NONE;
    ASSERT_EQ(ERROR_SUCCESS,
              key.ReadValue(base::UTF8ToUTF16(kProfileName_).c_str(),
                            &buffer[0], &data_size, &data_type));
    EXPECT_EQ(REG_BINARY, data_type);
    ASSERT_EQ(kTestDataSize_, data_size);
    EXPECT_EQ(std::string(&buffer[0], data_size),
              std::string(&kTestData_[0], kTestDataSize_));
  }

  static const char kProfileName_[];
  static const base::char16 kStoreKeyName_[];
  TestingProfile* profile_;

 private:
  registry_util::RegistryOverrideManager registry_override_manager_;
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle thread_task_runner_handle_;
  TestingProfileManager profile_manager_;

  DISALLOW_COPY_AND_ASSIGN(PlatformStateStoreWinTest);
};

// static
const char PlatformStateStoreWinTest::kProfileName_[] = "test_profile";
#if defined(GOOGLE_CHROME_BUILD)
const base::char16 PlatformStateStoreWinTest::kStoreKeyName_[] =
    L"Software\\Google\\Chrome\\IncidentsSent";
#else
const base::char16 PlatformStateStoreWinTest::kStoreKeyName_[] =
    L"Software\\Chromium\\IncidentsSent";
#endif

// Tests that store data is written correctly to the proper location.
TEST_F(PlatformStateStoreWinTest, WriteStoreData) {
  ResetProfile(false /* !new_profile */);

  ASSERT_FALSE(base::win::RegKey(HKEY_CURRENT_USER, kStoreKeyName_,
                                 KEY_QUERY_VALUE | KEY_WOW64_32KEY)
                   .HasValue(base::UTF8ToUTF16(kProfileName_).c_str()));
  WriteStoreData(profile_, kTestData_);
  AssertTestDataIsPresent();
}

// Tests that store data is read from the proper location.
TEST_F(PlatformStateStoreWinTest, ReadStoreData) {
  // Put some data in the registry.
  WriteTestData();

  ResetProfile(false /* !new_profile */);
  std::string data;
  PlatformStateStoreLoadResult result = ReadStoreData(profile_, &data);
  EXPECT_EQ(PlatformStateStoreLoadResult::SUCCESS, result);
  EXPECT_EQ(std::string(&kTestData_[0], kTestDataSize_), data);
}

// Tests that an empty write clears the stored data.
TEST_F(PlatformStateStoreWinTest, WriteEmptyStoreData) {
  // Put some data in the registry.
  WriteTestData();

  ResetProfile(false /* !new_profile */);

  WriteStoreData(profile_, std::string());
  AssertTestDataIsAbsent();
}

// Tests that data in the registry is ignored if the profile is new.
TEST_F(PlatformStateStoreWinTest, ReadNewProfileClearData) {
  ResetProfile(true /* new_profile */);

  // Put some data in the registry.
  WriteTestData();

  std::string data;
  PlatformStateStoreLoadResult result = ReadStoreData(profile_, &data);
  EXPECT_EQ(PlatformStateStoreLoadResult::CLEARED_DATA, result);
  EXPECT_EQ(std::string(), data);
  AssertTestDataIsAbsent();
}

}  // namespace platform_state_store
}  // namespace safe_browsing
