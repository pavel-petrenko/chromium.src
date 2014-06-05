// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/identity/gaia_web_auth_flow.h"

#include <vector>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "content/public/test/test_browser_thread.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

class FakeWebAuthFlow : public WebAuthFlow {
 public:
  explicit FakeWebAuthFlow(WebAuthFlow::Delegate* delegate)
      : WebAuthFlow(delegate,
                    NULL,
                    GURL(),
                    WebAuthFlow::INTERACTIVE) {}

  virtual void Start() OVERRIDE {}
};

class TestGaiaWebAuthFlow : public GaiaWebAuthFlow {
 public:
  TestGaiaWebAuthFlow(GaiaWebAuthFlow::Delegate* delegate,
                      const std::string& extension_id,
                      const OAuth2Info& oauth2_info,
                      GoogleServiceAuthError::State ubertoken_error_state)
      : GaiaWebAuthFlow(delegate,
                        NULL,
                        "account_id",
                        "extension_id",
                        oauth2_info,
                        "en-us"),
        ubertoken_error_(ubertoken_error_state) {}

  virtual void Start() OVERRIDE {
    if (ubertoken_error_.state() == GoogleServiceAuthError::NONE)
      OnUbertokenSuccess("fake_ubertoken");
    else
      OnUbertokenFailure(ubertoken_error_);
  }

 private:
  virtual scoped_ptr<WebAuthFlow> CreateWebAuthFlow(GURL url) OVERRIDE {
    return scoped_ptr<WebAuthFlow>(new FakeWebAuthFlow(this));
  }

  GoogleServiceAuthError ubertoken_error_;
};

class MockGaiaWebAuthFlowDelegate : public GaiaWebAuthFlow::Delegate {
 public:
  MOCK_METHOD3(OnGaiaFlowFailure,
               void(GaiaWebAuthFlow::Failure failure,
                    GoogleServiceAuthError service_error,
                    const std::string& oauth_error));
  MOCK_METHOD2(OnGaiaFlowCompleted,
               void(const std::string& access_token,
                    const std::string& expiration));
};

class IdentityGaiaWebAuthFlowTest : public testing::Test {
 public:
  IdentityGaiaWebAuthFlowTest()
      : ubertoken_error_state_(GoogleServiceAuthError::NONE),
        fake_ui_thread_(content::BrowserThread::UI, &message_loop_) {}

  virtual void TearDown() {
    testing::Test::TearDown();
    base::RunLoop loop;
    loop.RunUntilIdle();  // Run tasks so FakeWebAuthFlows get deleted.
  }

  scoped_ptr<TestGaiaWebAuthFlow> CreateTestFlow() {
    OAuth2Info oauth2_info;
    oauth2_info.client_id = "fake.client.id";
    return scoped_ptr<TestGaiaWebAuthFlow>(new TestGaiaWebAuthFlow(
        &delegate_, "extension_id", oauth2_info, ubertoken_error_state_));
  }

  std::string GetFinalTitle(const std::string& fragment) {
    return std::string("Loading id.client.fake:/extension_id#") + fragment;
  }

  GoogleServiceAuthError GetNoneServiceError() {
    return GoogleServiceAuthError(GoogleServiceAuthError::NONE);
  }

  void set_ubertoken_error(
      GoogleServiceAuthError::State ubertoken_error_state) {
    ubertoken_error_state_ = ubertoken_error_state;
  }

 protected:
  testing::StrictMock<MockGaiaWebAuthFlowDelegate> delegate_;
  GoogleServiceAuthError::State ubertoken_error_state_;
  base::MessageLoop message_loop_;
  content::TestBrowserThread fake_ui_thread_;
};

TEST_F(IdentityGaiaWebAuthFlowTest, OAuthError) {
  scoped_ptr<TestGaiaWebAuthFlow> flow = CreateTestFlow();
  flow->Start();
  EXPECT_CALL(delegate_, OnGaiaFlowFailure(
          GaiaWebAuthFlow::OAUTH_ERROR,
          GoogleServiceAuthError(GoogleServiceAuthError::NONE),
          "access_denied"));
  flow->OnAuthFlowTitleChange(GetFinalTitle("error=access_denied"));
}

TEST_F(IdentityGaiaWebAuthFlowTest, Token) {
  scoped_ptr<TestGaiaWebAuthFlow> flow = CreateTestFlow();
  flow->Start();
  EXPECT_CALL(delegate_, OnGaiaFlowCompleted("fake_access_token", ""));
  flow->OnAuthFlowTitleChange(GetFinalTitle("access_token=fake_access_token"));
}

TEST_F(IdentityGaiaWebAuthFlowTest, TokenAndExpiration) {
  scoped_ptr<TestGaiaWebAuthFlow> flow = CreateTestFlow();
  flow->Start();
  EXPECT_CALL(delegate_, OnGaiaFlowCompleted("fake_access_token", "3600"));
  flow->OnAuthFlowTitleChange(
      GetFinalTitle("access_token=fake_access_token&expires_in=3600"));
}

TEST_F(IdentityGaiaWebAuthFlowTest, ExtraFragmentParametersSuccess) {
  scoped_ptr<TestGaiaWebAuthFlow> flow = CreateTestFlow();
  flow->Start();
  EXPECT_CALL(delegate_,
              OnGaiaFlowCompleted("fake_access_token", "3600"));
  flow->OnAuthFlowTitleChange(GetFinalTitle("chaff1=stuff&"
                                            "expires_in=3600&"
                                            "chaff2=and&"
                                            "nonerror=fake_error&"
                                            "chaff3=nonsense&"
                                            "access_token=fake_access_token&"
                                            "chaff4="));
}

TEST_F(IdentityGaiaWebAuthFlowTest, ExtraFragmentParametersError) {
  scoped_ptr<TestGaiaWebAuthFlow> flow = CreateTestFlow();
  flow->Start();
  EXPECT_CALL(delegate_, OnGaiaFlowFailure(
          GaiaWebAuthFlow::OAUTH_ERROR,
          GoogleServiceAuthError(GoogleServiceAuthError::NONE),
          "fake_error"));
  flow->OnAuthFlowTitleChange(GetFinalTitle("chaff1=stuff&"
                                            "expires_in=3600&"
                                            "chaff2=and&"
                                            "error=fake_error&"
                                            "chaff3=nonsense&"
                                            "access_token=fake_access_token&"
                                            "chaff4="));
}

TEST_F(IdentityGaiaWebAuthFlowTest, TitleSpam) {
  scoped_ptr<TestGaiaWebAuthFlow> flow = CreateTestFlow();
  flow->Start();
  flow->OnAuthFlowTitleChange(
      "Loading https://extension_id.chromiumapp.org/#error=non_final_title");
  flow->OnAuthFlowTitleChange("I'm feeling entitled.");
  flow->OnAuthFlowTitleChange("");
  flow->OnAuthFlowTitleChange(
      "Loading id.client.fake:/bad_extension_id#error=non_final_title");
  flow->OnAuthFlowTitleChange(
      "Loading bad.id.client.fake:/extension_id#error=non_final_title");
  EXPECT_CALL(delegate_, OnGaiaFlowCompleted("fake_access_token", ""));
  flow->OnAuthFlowTitleChange(GetFinalTitle("access_token=fake_access_token"));
}

TEST_F(IdentityGaiaWebAuthFlowTest, EmptyFragment) {
  scoped_ptr<TestGaiaWebAuthFlow> flow = CreateTestFlow();
  flow->Start();
  EXPECT_CALL(
      delegate_,
      OnGaiaFlowFailure(
          GaiaWebAuthFlow::INVALID_REDIRECT,
          GoogleServiceAuthError(GoogleServiceAuthError::NONE),
          ""));
  flow->OnAuthFlowTitleChange(GetFinalTitle(""));
}

TEST_F(IdentityGaiaWebAuthFlowTest, JunkFragment) {
  scoped_ptr<TestGaiaWebAuthFlow> flow = CreateTestFlow();
  flow->Start();
  EXPECT_CALL(
      delegate_,
      OnGaiaFlowFailure(
          GaiaWebAuthFlow::INVALID_REDIRECT,
          GoogleServiceAuthError(GoogleServiceAuthError::NONE),
          ""));
  flow->OnAuthFlowTitleChange(GetFinalTitle("thisisjustabunchofjunk"));
}

TEST_F(IdentityGaiaWebAuthFlowTest, NoFragment) {
  scoped_ptr<TestGaiaWebAuthFlow> flow = CreateTestFlow();
  flow->Start();
  // This won't be recognized as an interesting title.
  flow->OnAuthFlowTitleChange("Loading id.client.fake:/extension_id");
}

TEST_F(IdentityGaiaWebAuthFlowTest, Host) {
  scoped_ptr<TestGaiaWebAuthFlow> flow = CreateTestFlow();
  flow->Start();
  // These won't be recognized as interesting titles.
  flow->OnAuthFlowTitleChange(
      "Loading id.client.fake://extension_id#access_token=fake_access_token");
  flow->OnAuthFlowTitleChange(
      "Loading id.client.fake://extension_id/#access_token=fake_access_token");
  flow->OnAuthFlowTitleChange(
      "Loading "
      "id.client.fake://host/extension_id/#access_token=fake_access_token");
}

TEST_F(IdentityGaiaWebAuthFlowTest, UbertokenFailure) {
  set_ubertoken_error(GoogleServiceAuthError::CONNECTION_FAILED);
  scoped_ptr<TestGaiaWebAuthFlow> flow = CreateTestFlow();
  EXPECT_CALL(
      delegate_,
      OnGaiaFlowFailure(
          GaiaWebAuthFlow::SERVICE_AUTH_ERROR,
          GoogleServiceAuthError(GoogleServiceAuthError::CONNECTION_FAILED),
          ""));
  flow->Start();
}

TEST_F(IdentityGaiaWebAuthFlowTest, AuthFlowFailure) {
  scoped_ptr<TestGaiaWebAuthFlow> flow = CreateTestFlow();
  flow->Start();
  EXPECT_CALL(
      delegate_,
      OnGaiaFlowFailure(
          GaiaWebAuthFlow::WINDOW_CLOSED,
          GoogleServiceAuthError(GoogleServiceAuthError::NONE),
          ""));
  flow->OnAuthFlowFailure(WebAuthFlow::WINDOW_CLOSED);
}

}  // namespace extensions
