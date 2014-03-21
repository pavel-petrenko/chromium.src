// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/browser/risk/fingerprint.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/port.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/autofill/content/browser/risk/proto/fingerprint.pb.h"
#include "content/public/browser/geolocation_provider.h"
#include "content/public/browser/gpu_data_manager.h"
#include "content/public/common/geoposition.h"
#include "content/public/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/WebRect.h"
#include "third_party/WebKit/public/platform/WebScreenInfo.h"
#include "ui/gfx/rect.h"

using testing::ElementsAre;

namespace autofill {
namespace risk {

namespace internal {

// Defined in the implementation file corresponding to this test.
void GetFingerprintInternal(
    uint64 obfuscated_gaia_id,
    const gfx::Rect& window_bounds,
    const gfx::Rect& content_bounds,
    const blink::WebScreenInfo& screen_info,
    const std::string& version,
    const std::string& charset,
    const std::string& accept_languages,
    const base::Time& install_time,
    const std::string& app_locale,
    const std::string& user_agent,
    const base::TimeDelta& timeout,
    const base::Callback<void(scoped_ptr<Fingerprint>)>& callback);

}  // namespace internal

// Constants that are passed verbatim to the fingerprinter code and should be
// serialized into the resulting protocol buffer.
const uint64 kObfuscatedGaiaId = GG_UINT64_C(16571487432910023183);
const char kCharset[] = "UTF-8";
const char kAcceptLanguages[] = "en-US,en";
const int kScreenColorDepth = 53;

// Geolocation constants that are passed verbatim to the fingerprinter code and
// should be serialized into the resulting protocol buffer.
const double kLatitude = -42.0;
const double kLongitude = 17.3;
const double kAltitude = 123.4;
const double kAccuracy = 73.7;
const int kGeolocationTime = 87;

class AutofillRiskFingerprintTest : public InProcessBrowserTest {
 public:
  AutofillRiskFingerprintTest()
      : window_bounds_(2, 3, 5, 7),
        content_bounds_(11, 13, 17, 37),
        screen_bounds_(0, 0, 101, 71),
        available_screen_bounds_(0, 11, 101, 60),
        unavailable_screen_bounds_(0, 0, 101, 11) {}

  void GetFingerprintTestCallback(scoped_ptr<Fingerprint> fingerprint) {
    // Verify that all fields Chrome can fill have been filled.
    ASSERT_TRUE(fingerprint->has_machine_characteristics());
    const Fingerprint::MachineCharacteristics& machine =
        fingerprint->machine_characteristics();
    EXPECT_TRUE(machine.has_operating_system_build());
    EXPECT_TRUE(machine.has_browser_install_time_hours());
    EXPECT_GT(machine.font_size(), 0);
    EXPECT_GT(machine.plugin_size(), 0);
    EXPECT_TRUE(machine.has_utc_offset_ms());
    EXPECT_TRUE(machine.has_browser_language());
    EXPECT_GT(machine.requested_language_size(), 0);
    EXPECT_TRUE(machine.has_charset());
    EXPECT_TRUE(machine.has_screen_count());
    ASSERT_TRUE(machine.has_screen_size());
    EXPECT_TRUE(machine.screen_size().has_width());
    EXPECT_TRUE(machine.screen_size().has_height());
    EXPECT_TRUE(machine.has_screen_color_depth());
    ASSERT_TRUE(machine.has_unavailable_screen_size());
    EXPECT_TRUE(machine.unavailable_screen_size().has_width());
    EXPECT_TRUE(machine.unavailable_screen_size().has_height());
    EXPECT_TRUE(machine.has_user_agent());
    ASSERT_TRUE(machine.has_cpu());
    EXPECT_TRUE(machine.cpu().has_vendor_name());
    EXPECT_TRUE(machine.cpu().has_brand());
    EXPECT_TRUE(machine.has_ram());
    ASSERT_TRUE(machine.has_graphics_card());
    EXPECT_TRUE(machine.graphics_card().has_vendor_id());
    EXPECT_TRUE(machine.graphics_card().has_device_id());
    EXPECT_TRUE(machine.has_browser_build());
    EXPECT_TRUE(machine.has_browser_feature());

    ASSERT_TRUE(fingerprint->has_transient_state());
    const Fingerprint::TransientState& transient_state =
        fingerprint->transient_state();
    ASSERT_TRUE(transient_state.has_inner_window_size());
    ASSERT_TRUE(transient_state.has_outer_window_size());
    EXPECT_TRUE(transient_state.inner_window_size().has_width());
    EXPECT_TRUE(transient_state.inner_window_size().has_height());
    EXPECT_TRUE(transient_state.outer_window_size().has_width());
    EXPECT_TRUE(transient_state.outer_window_size().has_height());

    ASSERT_TRUE(fingerprint->has_user_characteristics());
    const Fingerprint::UserCharacteristics& user_characteristics =
        fingerprint->user_characteristics();
    ASSERT_TRUE(user_characteristics.has_location());
    const Fingerprint::UserCharacteristics::Location& location =
        user_characteristics.location();
    EXPECT_TRUE(location.has_altitude());
    EXPECT_TRUE(location.has_latitude());
    EXPECT_TRUE(location.has_longitude());
    EXPECT_TRUE(location.has_accuracy());
    EXPECT_TRUE(location.has_time_in_ms());

    ASSERT_TRUE(fingerprint->has_metadata());
    EXPECT_TRUE(fingerprint->metadata().has_timestamp_ms());
    EXPECT_TRUE(fingerprint->metadata().has_obfuscated_gaia_id());
    EXPECT_TRUE(fingerprint->metadata().has_fingerprinter_version());

    // Some values have exact known (mocked out) values:
    EXPECT_THAT(machine.requested_language(), ElementsAre("en-US", "en"));
    EXPECT_EQ(kCharset, machine.charset());
    EXPECT_EQ(kScreenColorDepth, machine.screen_color_depth());
    EXPECT_EQ(unavailable_screen_bounds_.width(),
              machine.unavailable_screen_size().width());
    EXPECT_EQ(unavailable_screen_bounds_.height(),
              machine.unavailable_screen_size().height());
    EXPECT_EQ(Fingerprint::MachineCharacteristics::FEATURE_REQUEST_AUTOCOMPLETE,
              machine.browser_feature());
    EXPECT_EQ(content_bounds_.width(),
              transient_state.inner_window_size().width());
    EXPECT_EQ(content_bounds_.height(),
              transient_state.inner_window_size().height());
    EXPECT_EQ(window_bounds_.width(),
              transient_state.outer_window_size().width());
    EXPECT_EQ(window_bounds_.height(),
              transient_state.outer_window_size().height());
    EXPECT_EQ(kObfuscatedGaiaId, fingerprint->metadata().obfuscated_gaia_id());
    EXPECT_EQ(kAltitude, location.altitude());
    EXPECT_EQ(kLatitude, location.latitude());
    EXPECT_EQ(kLongitude, location.longitude());
    EXPECT_EQ(kAccuracy, location.accuracy());
    EXPECT_EQ(kGeolocationTime, location.time_in_ms());

    message_loop_.Quit();
  }

 protected:
  // Constants defining bounds in the screen coordinate system that are passed
  // verbatim to the fingerprinter code and should be serialized into the
  // resulting protocol buffer.  Declared as class members because gfx::Rect is
  // not a POD type, so it cannot be statically initialized.
  const gfx::Rect window_bounds_;
  const gfx::Rect content_bounds_;
  const gfx::Rect screen_bounds_;
  const gfx::Rect available_screen_bounds_;
  const gfx::Rect unavailable_screen_bounds_;

  // A message loop to block on the asynchronous loading of the fingerprint.
  base::MessageLoopForUI message_loop_;
};

// Test that getting a fingerprint works on some basic level.
IN_PROC_BROWSER_TEST_F(AutofillRiskFingerprintTest, GetFingerprint) {
  // This test hangs when there is no GPU process.
  // http://crbug.com/327272
  if (!content::GpuDataManager::GetInstance()->GpuAccessAllowed(NULL))
    return;

  content::Geoposition position;
  position.latitude = kLatitude;
  position.longitude = kLongitude;
  position.altitude = kAltitude;
  position.accuracy = kAccuracy;
  position.timestamp =
      base::Time::UnixEpoch() +
      base::TimeDelta::FromMilliseconds(kGeolocationTime);
  scoped_refptr<content::MessageLoopRunner> runner =
      new content::MessageLoopRunner;
  content::GeolocationProvider::OverrideLocationForTesting(
      position, runner->QuitClosure());
  runner->Run();

  blink::WebScreenInfo screen_info;
  screen_info.depth = kScreenColorDepth;
  screen_info.rect = blink::WebRect(screen_bounds_);
  screen_info.availableRect = blink::WebRect(available_screen_bounds_);

  internal::GetFingerprintInternal(
      kObfuscatedGaiaId, window_bounds_, content_bounds_, screen_info,
      "25.0.0.123", kCharset, kAcceptLanguages, base::Time::Now(),
      g_browser_process->GetApplicationLocale(), GetUserAgent(),
      base::TimeDelta::FromDays(1),  // Ought to be longer than any test run.
      base::Bind(&AutofillRiskFingerprintTest::GetFingerprintTestCallback,
                 base::Unretained(this)));

  // Wait for the callback to be called.
  message_loop_.Run();
}

}  // namespace risk
}  // namespace autofill
