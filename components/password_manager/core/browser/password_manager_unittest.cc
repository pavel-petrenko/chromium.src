// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/password_manager.h"

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/password_manager/core/browser/mock_password_store.h"
#include "components/password_manager/core/browser/password_autofill_manager.h"
#include "components/password_manager/core/browser/password_manager_driver.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/browser/stub_password_manager_client.h"
#include "components/password_manager/core/browser/stub_password_manager_driver.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/password_manager/core/common/password_manager_switches.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using autofill::PasswordForm;
using base::ASCIIToUTF16;
using testing::_;
using testing::AnyNumber;
using testing::Return;
using testing::SaveArg;
using testing::WithArg;

namespace password_manager {

namespace {

class MockPasswordManagerClient : public StubPasswordManagerClient {
 public:
  MOCK_CONST_METHOD0(IsSavingEnabledForCurrentPage, bool());
  MOCK_CONST_METHOD0(DidLastPageLoadEncounterSSLErrors, bool());
  MOCK_CONST_METHOD2(IsSyncAccountCredential,
                     bool(const std::string&, const std::string&));
  MOCK_CONST_METHOD0(GetPasswordStore, PasswordStore*());
  // The code inside EXPECT_CALL for PromptUserToSaveOrUpdatePasswordPtr owns
  // the PasswordFormManager* argument.
  MOCK_METHOD2(PromptUserToSaveOrUpdatePasswordPtr,
               void(PasswordFormManager*, CredentialSourceType type));
  MOCK_METHOD0(AutomaticPasswordSaveIndicator, void());
  MOCK_METHOD0(GetPrefs, PrefService*());
  MOCK_METHOD0(GetDriver, PasswordManagerDriver*());

  // Workaround for scoped_ptr<> lacking a copy constructor.
  bool PromptUserToSaveOrUpdatePassword(
      scoped_ptr<PasswordFormManager> manager,
      password_manager::CredentialSourceType type,
      bool update_password) override {
    PromptUserToSaveOrUpdatePasswordPtr(manager.release(), type);
    return false;
  }
  void AutomaticPasswordSave(scoped_ptr<PasswordFormManager> manager) override {
    AutomaticPasswordSaveIndicator();
  }
};

class MockPasswordManagerDriver : public StubPasswordManagerDriver {
 public:
  MOCK_METHOD1(FillPasswordForm, void(const autofill::PasswordFormFillData&));
  MOCK_METHOD0(GetPasswordManager, PasswordManager*());
  MOCK_METHOD0(GetPasswordAutofillManager, PasswordAutofillManager*());
};

// Invokes the password store consumer with a single copy of |form|.
ACTION_P(InvokeConsumer, form) {
  ScopedVector<PasswordForm> result;
  result.push_back(make_scoped_ptr(new PasswordForm(form)));
  arg0->OnGetPasswordStoreResults(result.Pass());
}

ACTION(InvokeEmptyConsumer) {
  arg0->OnGetPasswordStoreResults(ScopedVector<PasswordForm>());
}

ACTION_P(SaveToScopedPtr, scoped) { scoped->reset(arg0); }

}  // namespace

class PasswordManagerTest : public testing::Test {
 protected:
  void SetUp() override {
    store_ = new testing::StrictMock<MockPasswordStore>;
    EXPECT_CALL(*store_, ReportMetrics(_, _)).Times(AnyNumber());
    CHECK(store_->Init(syncer::SyncableService::StartSyncFlare()));

    EXPECT_CALL(client_, IsSyncAccountCredential(_, _))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(client_, GetPasswordStore())
        .WillRepeatedly(Return(store_.get()));
    EXPECT_CALL(client_, GetDriver()).WillRepeatedly(Return(&driver_));

    manager_.reset(new PasswordManager(&client_));
    password_autofill_manager_.reset(
        new PasswordAutofillManager(client_.GetDriver(), nullptr));

    EXPECT_CALL(driver_, GetPasswordManager())
        .WillRepeatedly(Return(manager_.get()));
    EXPECT_CALL(driver_, GetPasswordAutofillManager())
        .WillRepeatedly(Return(password_autofill_manager_.get()));
    EXPECT_CALL(client_, DidLastPageLoadEncounterSSLErrors())
        .WillRepeatedly(Return(false));
  }

  void TearDown() override {
    store_->Shutdown();
    store_ = nullptr;
  }

  PasswordForm MakeSimpleForm() {
    PasswordForm form;
    form.origin = GURL("http://www.google.com/a/LoginAuth");
    form.action = GURL("http://www.google.com/a/Login");
    form.username_element = ASCIIToUTF16("Email");
    form.password_element = ASCIIToUTF16("Passwd");
    form.username_value = ASCIIToUTF16("google");
    form.password_value = ASCIIToUTF16("password");
    form.submit_element = ASCIIToUTF16("signIn");
    form.signon_realm = "http://www.google.com";
    return form;
  }

  // Create a sign-up form that only has a new password field.
  PasswordForm MakeFormWithOnlyNewPasswordField() {
    PasswordForm form = MakeSimpleForm();
    form.new_password_element.swap(form.password_element);
    form.new_password_value.swap(form.password_value);
    return form;
  }

  // Reproduction of the form present on twitter's login page.
  PasswordForm MakeTwitterLoginForm() {
    PasswordForm form;
    form.origin = GURL("https://twitter.com/");
    form.action = GURL("https://twitter.com/sessions");
    form.username_element = ASCIIToUTF16("Email");
    form.password_element = ASCIIToUTF16("Passwd");
    form.username_value = ASCIIToUTF16("twitter");
    form.password_value = ASCIIToUTF16("password");
    form.submit_element = ASCIIToUTF16("signIn");
    form.signon_realm = "https://twitter.com";
    return form;
  }

  // Reproduction of the form present on twitter's failed login page.
  PasswordForm MakeTwitterFailedLoginForm() {
    PasswordForm form;
    form.origin = GURL("https://twitter.com/login/error?redirect_after_login");
    form.action = GURL("https://twitter.com/sessions");
    form.username_element = ASCIIToUTF16("EmailField");
    form.password_element = ASCIIToUTF16("PasswdField");
    form.username_value = ASCIIToUTF16("twitter");
    form.password_value = ASCIIToUTF16("password");
    form.submit_element = ASCIIToUTF16("signIn");
    form.signon_realm = "https://twitter.com";
    return form;
  }

  PasswordForm MakeSimpleFormWithOnlyPasswordField() {
    PasswordForm form(MakeSimpleForm());
    form.username_element.clear();
    form.username_value.clear();
    return form;
  }

  PasswordManager* manager() { return manager_.get(); }

  void OnPasswordFormSubmitted(const PasswordForm& form) {
    manager()->OnPasswordFormSubmitted(&driver_, form);
  }

  PasswordManager::PasswordSubmittedCallback SubmissionCallback() {
    return base::Bind(&PasswordManagerTest::FormSubmitted,
                      base::Unretained(this));
  }

  void FormSubmitted(const PasswordForm& form) { submitted_form_ = form; }

  base::MessageLoop message_loop_;
  scoped_refptr<MockPasswordStore> store_;
  MockPasswordManagerClient client_;
  MockPasswordManagerDriver driver_;
  scoped_ptr<PasswordAutofillManager> password_autofill_manager_;
  scoped_ptr<PasswordManager> manager_;
  PasswordForm submitted_form_;
};

MATCHER_P(FormMatches, form, "") {
  return form.signon_realm == arg.signon_realm && form.origin == arg.origin &&
         form.action == arg.action &&
         form.username_element == arg.username_element &&
         form.username_value == arg.username_value &&
         form.password_element == arg.password_element &&
         form.password_value == arg.password_value &&
         form.new_password_element == arg.new_password_element &&
         form.submit_element == arg.submit_element;
}

TEST_F(PasswordManagerTest, FormSubmitWithOnlyNewPasswordField) {
  // Test that when a form only contains a "new password" field, the form gets
  // saved and in password store, the new password value is saved as a current
  // password value, without specifying the password field name (so that
  // credentials from sign-up forms can be filled in login forms).
  std::vector<PasswordForm> observed;
  PasswordForm form(MakeFormWithOnlyNewPasswordField());
  observed.push_back(form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  OnPasswordFormSubmitted(form);

  scoped_ptr<PasswordFormManager> form_manager_to_save;
  EXPECT_CALL(client_,
              PromptUserToSaveOrUpdatePasswordPtr(
                  _, CredentialSourceType::CREDENTIAL_SOURCE_PASSWORD_MANAGER))
      .WillOnce(WithArg<0>(SaveToScopedPtr(&form_manager_to_save)));

  // Now the password manager waits for the navigation to complete.
  observed.clear();
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // Simulate saving the form, as if the info bar was accepted.
  PasswordForm saved_form;
  EXPECT_CALL(*store_, AddLogin(_)).WillOnce(SaveArg<0>(&saved_form));
  ASSERT_TRUE(form_manager_to_save);
  form_manager_to_save->Save();

  // The value of the new password field should have been promoted to, and saved
  // to the password store as the current password.
  PasswordForm expected_form(form);
  expected_form.password_value.swap(expected_form.new_password_value);
  EXPECT_THAT(saved_form, FormMatches(expected_form));
}

TEST_F(PasswordManagerTest, GeneratedPasswordFormSubmitEmptyStore) {
  // Test that generated passwords are stored without asking the user.
  std::vector<PasswordForm> observed;
  PasswordForm form(MakeFormWithOnlyNewPasswordField());
  observed.push_back(form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // Simulate the user generating the password and submitting the form.
  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  manager()->SetHasGeneratedPasswordForForm(&driver_, form, true);
  OnPasswordFormSubmitted(form);

  // The user should not need to confirm saving as they have already given
  // consent by using the generated password. The form should be saved once
  // navigation occurs. The client will be informed that automatic saving has
  // occured.
  EXPECT_CALL(client_, PromptUserToSaveOrUpdatePasswordPtr(_, _)).Times(0);
  PasswordForm form_to_save;
  EXPECT_CALL(*store_, AddLogin(_)).WillOnce(SaveArg<0>(&form_to_save));
  EXPECT_CALL(client_, AutomaticPasswordSaveIndicator());

  // Now the password manager waits for the navigation to complete.
  observed.clear();
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);
  EXPECT_EQ(form.username_value, form_to_save.username_value);
  // What was "new password" field in the submitted form, becomes the current
  // password field in the form to save.
  EXPECT_EQ(form.new_password_value, form_to_save.password_value);
}

TEST_F(PasswordManagerTest, FormSubmitNoGoodMatch) {
  // When the password store already contains credentials for a given form, new
  // credentials get still added, as long as they differ in username from the
  // stored ones.
  ScopedVector<PasswordForm> result;
  PasswordForm existing_different(MakeSimpleForm());
  existing_different.username_value = ASCIIToUTF16("google2");

  std::vector<PasswordForm> observed;
  PasswordForm form(MakeSimpleForm());
  observed.push_back(form);
  EXPECT_CALL(driver_, FillPasswordForm(_)).Times(2);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillOnce(WithArg<2>(InvokeConsumer(existing_different)));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);
  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  OnPasswordFormSubmitted(form);

  // We still expect an add, since we didn't have a good match.
  scoped_ptr<PasswordFormManager> form_manager_to_save;
  EXPECT_CALL(client_,
              PromptUserToSaveOrUpdatePasswordPtr(
                  _, CredentialSourceType::CREDENTIAL_SOURCE_PASSWORD_MANAGER))
      .WillOnce(WithArg<0>(SaveToScopedPtr(&form_manager_to_save)));

  // Now the password manager waits for the navigation to complete.
  observed.clear();
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // Simulate saving the form.
  EXPECT_CALL(*store_, AddLogin(FormMatches(form)));
  ASSERT_TRUE(form_manager_to_save);
  form_manager_to_save->Save();
}

TEST_F(PasswordManagerTest, FormSeenThenLeftPage) {
  std::vector<PasswordForm> observed;
  PasswordForm form(MakeSimpleForm());
  observed.push_back(form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // No message from the renderer that a password was submitted. No
  // expected calls.
  EXPECT_CALL(client_, PromptUserToSaveOrUpdatePasswordPtr(_, _)).Times(0);
  observed.clear();
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);
}

TEST_F(PasswordManagerTest, FormSubmit) {
  // Test that a plain form submit results in offering to save passwords.
  std::vector<PasswordForm> observed;
  PasswordForm form(MakeSimpleForm());
  observed.push_back(form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  OnPasswordFormSubmitted(form);

  scoped_ptr<PasswordFormManager> form_manager_to_save;
  EXPECT_CALL(client_,
              PromptUserToSaveOrUpdatePasswordPtr(
                  _, CredentialSourceType::CREDENTIAL_SOURCE_PASSWORD_MANAGER))
      .WillOnce(WithArg<0>(SaveToScopedPtr(&form_manager_to_save)));

  observed.clear();
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // Simulate saving the form, as if the info bar was accepted.
  EXPECT_CALL(*store_, AddLogin(FormMatches(form)));
  ASSERT_TRUE(form_manager_to_save);
  form_manager_to_save->Save();
}

// This test verifies a fix for http://crbug.com/236673
TEST_F(PasswordManagerTest, FormSubmitWithFormOnPreviousPage) {
  PasswordForm first_form(MakeSimpleForm());
  first_form.origin = GURL("http://www.nytimes.com/");
  first_form.action = GURL("https://myaccount.nytimes.com/auth/login");
  first_form.signon_realm = "http://www.nytimes.com/";
  PasswordForm second_form(MakeSimpleForm());
  second_form.origin = GURL("https://myaccount.nytimes.com/auth/login");
  second_form.action = GURL("https://myaccount.nytimes.com/auth/login");
  second_form.signon_realm = "https://myaccount.nytimes.com/";

  // Pretend that the form is hidden on the first page.
  std::vector<PasswordForm> observed;
  observed.push_back(first_form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  observed.clear();
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // Now navigate to a second page.
  manager()->DidNavigateMainFrame();

  // This page contains a form with the same markup, but on a different
  // URL.
  observed.push_back(second_form);
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // Now submit this form
  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  OnPasswordFormSubmitted(second_form);

  scoped_ptr<PasswordFormManager> form_manager_to_save;
  EXPECT_CALL(client_,
              PromptUserToSaveOrUpdatePasswordPtr(
                  _, CredentialSourceType::CREDENTIAL_SOURCE_PASSWORD_MANAGER))
      .WillOnce(WithArg<0>(SaveToScopedPtr(&form_manager_to_save)));
  // Navigation after form submit, no forms appear.
  observed.clear();
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // Simulate saving the form, as if the info bar was accepted and make sure
  // that the saved form matches the second form, not the first.
  EXPECT_CALL(*store_, AddLogin(FormMatches(second_form)));
  ASSERT_TRUE(form_manager_to_save);
  form_manager_to_save->Save();
}

TEST_F(PasswordManagerTest, FormSubmitInvisibleLogin) {
  // Tests fix of http://crbug.com/28911: if the login form reappears on the
  // subsequent page, but is invisible, it shouldn't count as a failed login.
  std::vector<PasswordForm> observed;
  PasswordForm form(MakeSimpleForm());
  observed.push_back(form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  OnPasswordFormSubmitted(form);

  // Expect info bar to appear:
  scoped_ptr<PasswordFormManager> form_manager_to_save;
  EXPECT_CALL(client_,
              PromptUserToSaveOrUpdatePasswordPtr(
                  _, CredentialSourceType::CREDENTIAL_SOURCE_PASSWORD_MANAGER))
      .WillOnce(WithArg<0>(SaveToScopedPtr(&form_manager_to_save)));

  // The form reappears, but is not visible in the layout:
  manager()->OnPasswordFormsParsed(&driver_, observed);
  observed.clear();
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // Simulate saving the form.
  EXPECT_CALL(*store_, AddLogin(FormMatches(form)));
  ASSERT_TRUE(form_manager_to_save);
  form_manager_to_save->Save();
}

TEST_F(PasswordManagerTest, InitiallyInvisibleForm) {
  // Make sure an invisible login form still gets autofilled.
  PasswordForm form(MakeSimpleForm());
  std::vector<PasswordForm> observed;
  observed.push_back(form);
  EXPECT_CALL(driver_, FillPasswordForm(_));
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillOnce(WithArg<2>(InvokeConsumer(form)));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  observed.clear();
  manager()->OnPasswordFormsRendered(&driver_, observed, true);
}


TEST_F(PasswordManagerTest, FillPasswordsOnDisabledManager) {
  // Test fix for http://crbug.com/158296: Passwords must be filled even if the
  // password manager is disabled.
  PasswordForm form(MakeSimpleForm());
  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(false));
  std::vector<PasswordForm> observed;
  observed.push_back(form);
  EXPECT_CALL(driver_, FillPasswordForm(_));
  EXPECT_CALL(*store_, GetLogins(_, PasswordStore::DISALLOW_PROMPT, _))
      .WillOnce(WithArg<2>(InvokeConsumer(form)));
  manager()->OnPasswordFormsParsed(&driver_, observed);
}

TEST_F(PasswordManagerTest, SubmissionCallbackTest) {
  manager()->AddSubmissionCallback(SubmissionCallback());
  PasswordForm form = MakeSimpleForm();
  // Prefs are needed for failure logging about having no matching observed
  // form.
  EXPECT_CALL(client_, GetPrefs()).WillRepeatedly(Return(nullptr));
  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  OnPasswordFormSubmitted(form);
  EXPECT_EQ(form, submitted_form_);
}

TEST_F(PasswordManagerTest, PasswordFormReappearance) {
  // If the password form reappears after submit, PasswordManager should deduce
  // that the login failed and not offer saving.
  std::vector<PasswordForm> observed;
  PasswordForm login_form(MakeTwitterLoginForm());
  observed.push_back(login_form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  OnPasswordFormSubmitted(login_form);

  observed.clear();
  observed.push_back(MakeTwitterFailedLoginForm());
  // A PasswordForm appears, and is visible in the layout:
  // No expected calls to the PasswordStore...
  EXPECT_CALL(client_, PromptUserToSaveOrUpdatePasswordPtr(_, _)).Times(0);
  EXPECT_CALL(client_, AutomaticPasswordSaveIndicator()).Times(0);
  EXPECT_CALL(*store_, AddLogin(_)).Times(0);
  EXPECT_CALL(*store_, UpdateLogin(_)).Times(0);
  EXPECT_CALL(*store_, UpdateLoginWithPrimaryKey(_, _)).Times(0);
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);
}

TEST_F(PasswordManagerTest, SyncCredentialsNotSaved) {
  // Simulate loading a simple form with no existing stored password.
  std::vector<PasswordForm> observed;
  PasswordForm form(MakeSimpleForm());
  observed.push_back(form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // User should not be prompted and password should not be saved.
  EXPECT_CALL(client_, PromptUserToSaveOrUpdatePasswordPtr(_, _)).Times(0);
  EXPECT_CALL(*store_, AddLogin(_)).Times(0);
  // Prefs are needed for failure logging about sync credentials.
  EXPECT_CALL(client_, GetPrefs()).WillRepeatedly(Return(nullptr));
  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));

  // Recognise the stored credential as a sync one. Make sure that the
  // credentials are identified by the realm, not origin with path.
  EXPECT_CALL(client_, IsSyncAccountCredential(_, form.signon_realm))
      .WillRepeatedly(Return(true));

  OnPasswordFormSubmitted(form);
  observed.clear();
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);
}

// When there is a sync password saved, and the user successfully uses the
// stored version of it, PasswordManager should not drop that password.
TEST_F(PasswordManagerTest, SyncCredentialsNotDroppedIfUpToDate) {
  PasswordForm form(MakeSimpleForm());
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeConsumer(form)));

  // Start recognising the stored credential as a sync one.
  EXPECT_CALL(client_, IsSyncAccountCredential(_, _))
      .WillRepeatedly(Return(true));

  std::vector<PasswordForm> observed;
  observed.push_back(form);
  EXPECT_CALL(driver_, FillPasswordForm(_)).Times(2);
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // Submit form and finish navigation.
  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(client_, GetPrefs()).WillRepeatedly(Return(nullptr));
  manager()->ProvisionallySavePassword(form);

  // Chrome should not remove the sync credential, because it was successfully
  // used as stored, and therefore is up to date.
  EXPECT_CALL(*store_, RemoveLogin(_)).Times(0);
  observed.clear();
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);
}

// When there is a sync password saved, and the user successfully uses an
// updated version of it, the obsolete one should be dropped, to avoid filling
// it later.
TEST_F(PasswordManagerTest, SyncCredentialsDroppedWhenObsolete) {
  PasswordForm form(MakeSimpleForm());
  form.password_value = ASCIIToUTF16("old pa55word");
  // Pretend that the password store contains "old pa55word" stored for |form|.
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeConsumer(form)));

  // Load the page, pretend the password was updated.
  PasswordForm updated_form(form);
  updated_form.password_value = ASCIIToUTF16("n3w passw0rd");
  std::vector<PasswordForm> observed;
  observed.push_back(updated_form);
  EXPECT_CALL(driver_, FillPasswordForm(_)).Times(2);
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // Submit form and finish navigation.
  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(client_, GetPrefs()).WillRepeatedly(Return(nullptr));
  manager()->ProvisionallySavePassword(updated_form);

  // Recognise the stored credential as a sync one. Make sure that the
  // credentials are identified by the realm, not origin with path.
  EXPECT_CALL(client_, IsSyncAccountCredential(_, form.signon_realm))
      .WillRepeatedly(Return(true));

  // Because the user successfully uses an updated sync password, Chrome should
  // remove the obsolete copy of it.
  EXPECT_CALL(*store_, RemoveLogin(form));
  EXPECT_CALL(client_, PromptUserToSaveOrUpdatePasswordPtr(_, _)).Times(0);
  observed.clear();
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);
}

// While sync credentials are not saved, they are still filled to avoid users
// thinking they lost access to their accounts.
TEST_F(PasswordManagerTest, SyncCredentialsStillFilled) {
  PasswordForm form(MakeSimpleForm());
  // Pretend that the password store contains credentials stored for |form|.
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeConsumer(form)));

  // Start recognising the stored credential as a sync one.
  EXPECT_CALL(client_, IsSyncAccountCredential(_, _))
      .WillRepeatedly(Return(true));

  // Load the page.
  autofill::PasswordFormFillData form_data;
  EXPECT_CALL(driver_, FillPasswordForm(_)).WillOnce(SaveArg<0>(&form_data));
  std::vector<PasswordForm> observed;
  observed.push_back(form);
  manager()->OnPasswordFormsParsed(&driver_, observed);
  EXPECT_EQ(form.password_value, form_data.password_field.value);
}

// On failed login attempts, the retry-form can have action scheme changed from
// HTTP to HTTPS (see http://crbug.com/400769). Check that such retry-form is
// considered equal to the original login form, and the attempt recognised as a
// failure.
TEST_F(PasswordManagerTest,
       SeeingFormActionWithOnlyHttpHttpsChangeIsLoginFailure) {
  EXPECT_CALL(driver_, FillPasswordForm(_)).Times(0);

  PasswordForm first_form(MakeSimpleForm());
  first_form.origin = GURL("http://www.xda-developers.com/");
  first_form.action = GURL("http://forum.xda-developers.com/login.php");

  // |second_form|'s action differs only with it's scheme i.e. *https://*.
  PasswordForm second_form(first_form);
  second_form.action = GURL("https://forum.xda-developers.com/login.php");

  std::vector<PasswordForm> observed;
  observed.push_back(first_form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  OnPasswordFormSubmitted(first_form);

  // Simulate loading a page, which contains |second_form| instead of
  // |first_form|.
  observed.clear();
  observed.push_back(second_form);

  // Verify that no prompt to save the password is shown.
  EXPECT_CALL(client_, PromptUserToSaveOrUpdatePasswordPtr(_, _)).Times(0);
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);
}

// Create a form with both a new and current password element. Let the current
// password value be non-empty and the new password value be empty and submit
// the form. While normally saving the new password is preferred (on change
// password forms, that would be the reasonable choice), if the new password is
// empty, this is likely just a slightly misunderstood form, and Chrome should
// save the non-empty current password field.
TEST_F(PasswordManagerTest, DoNotSaveWithEmptyNewPasswordAndNonemptyPassword) {
  std::vector<PasswordForm> observed;
  PasswordForm form(MakeSimpleForm());
  ASSERT_FALSE(form.password_value.empty());
  form.new_password_element = ASCIIToUTF16("new_password_element");
  form.new_password_value.clear();
  observed.push_back(form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillOnce(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  OnPasswordFormSubmitted(form);

  scoped_ptr<PasswordFormManager> form_manager_to_save;
  EXPECT_CALL(client_,
              PromptUserToSaveOrUpdatePasswordPtr(
                  _, CredentialSourceType::CREDENTIAL_SOURCE_PASSWORD_MANAGER))
      .WillOnce(WithArg<0>(SaveToScopedPtr(&form_manager_to_save)));

  // Now the password manager waits for the login to complete successfully.
  observed.clear();
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);
  ASSERT_TRUE(form_manager_to_save);
  EXPECT_EQ(form.password_value,
            PasswordFormManager::PasswordToSave(
                form_manager_to_save->pending_credentials()));
}

TEST_F(PasswordManagerTest, FormSubmitWithOnlyPasswordField) {
  // Test to verify that on submitting the HTML password form without having
  // username input filed shows password save promt and saves the password to
  // store.
  EXPECT_CALL(driver_, FillPasswordForm(_)).Times(0);
  std::vector<PasswordForm> observed;

  // Loads passsword form without username input field.
  PasswordForm form(MakeSimpleFormWithOnlyPasswordField());
  observed.push_back(form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillOnce(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  OnPasswordFormSubmitted(form);

  scoped_ptr<PasswordFormManager> form_manager_to_save;
  EXPECT_CALL(client_,
              PromptUserToSaveOrUpdatePasswordPtr(
                  _, CredentialSourceType::CREDENTIAL_SOURCE_PASSWORD_MANAGER))
      .WillOnce(WithArg<0>(SaveToScopedPtr(&form_manager_to_save)));

  // Now the password manager waits for the navigation to complete.
  observed.clear();
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // Simulate saving the form, as if the info bar was accepted.
  EXPECT_CALL(*store_, AddLogin(FormMatches(form)));
  ASSERT_TRUE(form_manager_to_save);
  form_manager_to_save->Save();
}

TEST_F(PasswordManagerTest, FillPasswordOnManyFrames) {
  // A password form should be filled in all frames it appears in.
  PasswordForm form(MakeSimpleForm());  // The observed and saved form.

  // Observe the form in the first frame.
  std::vector<PasswordForm> observed;
  observed.push_back(form);
  EXPECT_CALL(driver_, FillPasswordForm(_));
  EXPECT_CALL(*store_, GetLogins(_, PasswordStore::DISALLOW_PROMPT, _))
      .WillOnce(WithArg<2>(InvokeConsumer(form)));
  manager()->OnPasswordFormsParsed(&driver_, observed);

  // Now the form will be seen the second time, in a different frame. The driver
  // for that frame should be told to fill it, but the store should not be asked
  // for it again.
  MockPasswordManagerDriver driver_b;
  EXPECT_CALL(driver_b, FillPasswordForm(_));
  EXPECT_CALL(*store_, GetLogins(_, _, _)).Times(0);
  manager()->OnPasswordFormsParsed(&driver_b, observed);
}

TEST_F(PasswordManagerTest, InPageNavigation) {
  // Test that observing a newly submitted form shows the save password bar on
  // call in page navigation.
  std::vector<PasswordForm> observed;
  PasswordForm form(MakeSimpleForm());
  observed.push_back(form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillOnce(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));

  scoped_ptr<PasswordFormManager> form_manager_to_save;
  EXPECT_CALL(client_,
              PromptUserToSaveOrUpdatePasswordPtr(
                  _, CredentialSourceType::CREDENTIAL_SOURCE_PASSWORD_MANAGER))
      .WillOnce(WithArg<0>(SaveToScopedPtr(&form_manager_to_save)));

  manager()->OnInPageNavigation(&driver_, form);

  // Simulate saving the form, as if the info bar was accepted.
  EXPECT_CALL(*store_, AddLogin(FormMatches(form)));
  ASSERT_TRUE(form_manager_to_save);
  form_manager_to_save->Save();
}

TEST_F(PasswordManagerTest, InPageNavigationBlacklistedSite) {
  // Test that observing a newly submitted form on blacklisted site does notify
  // the embedder on call in page navigation.
  std::vector<PasswordForm> observed;
  PasswordForm form(MakeSimpleForm());
  observed.push_back(form);
  // Simulate that blacklisted form stored in store.
  PasswordForm blacklisted_form(form);
  blacklisted_form.username_value = ASCIIToUTF16("");
  blacklisted_form.blacklisted_by_user = true;
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillOnce(WithArg<2>(InvokeConsumer(blacklisted_form)));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  // Prefs are needed for failure logging about blacklisting.
  EXPECT_CALL(client_, GetPrefs()).WillRepeatedly(Return(nullptr));

  scoped_ptr<PasswordFormManager> form_manager_to_save;
  EXPECT_CALL(client_,
              PromptUserToSaveOrUpdatePasswordPtr(
                  _, CredentialSourceType::CREDENTIAL_SOURCE_PASSWORD_MANAGER))
      .WillOnce(WithArg<0>(SaveToScopedPtr(&form_manager_to_save)));

  manager()->OnInPageNavigation(&driver_, form);
  EXPECT_TRUE(form_manager_to_save->IsBlacklisted());
}

TEST_F(PasswordManagerTest, SavingSignupForms_NoHTMLMatch) {
  // Signup forms don't require HTML attributes match in order to save.
  // Verify that we prefer a better match (action + origin vs. origin).
  std::vector<PasswordForm> observed;
  PasswordForm form(MakeSimpleForm());
  observed.push_back(form);
  PasswordForm wrong_action_form(form);
  wrong_action_form.action = GURL("http://www.google.com/other/action");
  observed.push_back(wrong_action_form);

  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // Simulate either form changing or heuristics choosing other fields
  // after the user has entered their information.
  PasswordForm submitted_form(form);
  submitted_form.new_password_element = ASCIIToUTF16("new_password");
  submitted_form.new_password_value = form.password_value;
  submitted_form.password_element.clear();
  submitted_form.password_value.clear();

  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  OnPasswordFormSubmitted(submitted_form);

  scoped_ptr<PasswordFormManager> form_manager_to_save;
  EXPECT_CALL(client_,
              PromptUserToSaveOrUpdatePasswordPtr(
                  _, CredentialSourceType::CREDENTIAL_SOURCE_PASSWORD_MANAGER))
      .WillOnce(WithArg<0>(SaveToScopedPtr(&form_manager_to_save)));

  // Now the password manager waits for the navigation to complete.
  observed.clear();
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // Simulate saving the form, as if the info bar was accepted.
  PasswordForm form_to_save;
  EXPECT_CALL(*store_, AddLogin(_)).WillOnce(SaveArg<0>(&form_to_save));
  ASSERT_TRUE(form_manager_to_save);
  form_manager_to_save->Save();

  // PasswordManager observed two forms, and should have associate the saved one
  // with the observed form with a matching action.
  EXPECT_EQ(form.action, form_to_save.action);
  // Password values are always saved as the current password value, but the
  // current password field name should be empty if the value comes from the new
  // password field.
  EXPECT_EQ(submitted_form.new_password_value, form_to_save.password_value);
  EXPECT_TRUE(form_to_save.password_element.empty());
}

TEST_F(PasswordManagerTest, SavingSignupForms_NoActionMatch) {
  // Signup forms don't require HTML attributes match in order to save.
  // Verify that we prefer a better match (HTML attributes + origin vs. origin).
  std::vector<PasswordForm> observed;
  PasswordForm form(MakeSimpleForm());
  observed.push_back(form);
  // Change the submit element so we can track which of the two forms is
  // chosen as a better match.
  PasswordForm wrong_submit_form(form);
  wrong_submit_form.submit_element = ASCIIToUTF16("different_signin");
  wrong_submit_form.new_password_element = ASCIIToUTF16("new_password");
  wrong_submit_form.new_password_value = form.password_value;
  wrong_submit_form.password_element.clear();
  wrong_submit_form.password_value.clear();
  observed.push_back(wrong_submit_form);

  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  PasswordForm submitted_form(form);
  submitted_form.action = GURL("http://www.google.com/other/action");

  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  OnPasswordFormSubmitted(submitted_form);

  scoped_ptr<PasswordFormManager> form_manager_to_save;
  EXPECT_CALL(client_,
              PromptUserToSaveOrUpdatePasswordPtr(
                  _, CredentialSourceType::CREDENTIAL_SOURCE_PASSWORD_MANAGER))
      .WillOnce(WithArg<0>(SaveToScopedPtr(&form_manager_to_save)));

  // Now the password manager waits for the navigation to complete.
  observed.clear();
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // Simulate saving the form, as if the info bar was accepted.
  PasswordForm form_to_save;
  EXPECT_CALL(*store_, AddLogin(_)).WillOnce(SaveArg<0>(&form_to_save));
  ASSERT_TRUE(form_manager_to_save);
  form_manager_to_save->Save();

  // PasswordManager observed two forms, and should have associate the saved one
  // with the observed form with a matching action.
  EXPECT_EQ(form.submit_element, form_to_save.submit_element);

  EXPECT_EQ(submitted_form.password_value, form_to_save.password_value);
  EXPECT_EQ(submitted_form.password_element, form_to_save.password_element);
  EXPECT_EQ(submitted_form.username_value, form_to_save.username_value);
  EXPECT_EQ(submitted_form.username_element, form_to_save.username_element);
  EXPECT_TRUE(form_to_save.new_password_element.empty());
  EXPECT_TRUE(form_to_save.new_password_value.empty());
}

TEST_F(PasswordManagerTest, FormSubmittedChangedWithAutofillResponse) {
  // This tests verifies that if the observed forms and provisionally saved
  // differ in the choice of the username, the saving still succeeds, as long as
  // the changed form is marked "parsed using autofill predictions".
  std::vector<PasswordForm> observed;
  PasswordForm form(MakeSimpleForm());
  observed.push_back(form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // Simulate that based on autofill server username prediction, the username
  // of the form changed from the default candidate("Email") to something else.
  // Set the parsed_using_autofill_predictions bit to true to make sure that
  // choice of username is accepted by PasswordManager, otherwise the the form
  // will be rejected as not equal to the observed one. Note that during
  // initial parsing we don't have autofill server predictions yet, that's why
  // observed form and submitted form may be different.
  form.username_element = ASCIIToUTF16("Username");
  form.was_parsed_using_autofill_predictions = true;
  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  OnPasswordFormSubmitted(form);

  scoped_ptr<PasswordFormManager> form_manager_to_save;
  EXPECT_CALL(client_,
              PromptUserToSaveOrUpdatePasswordPtr(
                  _, CredentialSourceType::CREDENTIAL_SOURCE_PASSWORD_MANAGER))
      .WillOnce(WithArg<0>(SaveToScopedPtr(&form_manager_to_save)));

  // Now the password manager waits for the navigation to complete.
  observed.clear();
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // Simulate saving the form, as if the info bar was accepted.
  EXPECT_CALL(*store_, AddLogin(FormMatches(form)));
  ASSERT_TRUE(form_manager_to_save);
  form_manager_to_save->Save();
}

TEST_F(PasswordManagerTest, SaveFormFetchedAfterSubmit) {
  // Test that a password is offered for saving even if the response from the
  // PasswordStore comes after submit.
  std::vector<PasswordForm> observed;
  PasswordForm form(MakeSimpleForm());
  observed.push_back(form);

  PasswordStoreConsumer* form_manager = nullptr;
  // No call-back from store after GetLogins is called emulates that
  // PasswordStore did not fetch a form in time before submission.
  EXPECT_CALL(*store_, GetLogins(_, _, _)).WillOnce(SaveArg<2>(&form_manager));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  OnPasswordFormSubmitted(form);

  // Emulate fetching password form from PasswordStore after submission but
  // before post-navigation load.
  ASSERT_TRUE(form_manager);
  form_manager->OnGetPasswordStoreResults(ScopedVector<PasswordForm>());

  scoped_ptr<PasswordFormManager> form_manager_to_save;
  EXPECT_CALL(client_,
              PromptUserToSaveOrUpdatePasswordPtr(
                  _, CredentialSourceType::CREDENTIAL_SOURCE_PASSWORD_MANAGER))
      .WillOnce(WithArg<0>(SaveToScopedPtr(&form_manager_to_save)));

  // Now the password manager waits for the navigation to complete.
  observed.clear();
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  // Simulate saving the form, as if the info bar was accepted.
  EXPECT_CALL(*store_, AddLogin(FormMatches(form)));
  ASSERT_TRUE(form_manager_to_save);
  form_manager_to_save->Save();
}

TEST_F(PasswordManagerTest, PasswordGenerationForceSaving) {
  // Test the safety net against password loss during generation
  // (http://crbug.com/506307) -- out of the two rules (1) "always save
  // generated passwords" and (2) "do not save passwords when the password form
  // reappears", rule (1) takes precedence.
  std::vector<PasswordForm> observed;
  PasswordForm form(MakeFormWithOnlyNewPasswordField());
  observed.push_back(form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  manager()->SetHasGeneratedPasswordForForm(&driver_, form, true);

  // The user should not need to confirm saving as they have already given
  // consent by using the generated password. The form should be saved once
  // navigation occurs. The client will be informed that automatic saving has
  // occured.
  EXPECT_CALL(client_, PromptUserToSaveOrUpdatePasswordPtr(_, _)).Times(0);
  PasswordForm form_to_save;
  EXPECT_CALL(*store_, AddLogin(_)).WillOnce(SaveArg<0>(&form_to_save));
  EXPECT_CALL(client_, AutomaticPasswordSaveIndicator());

  // Simulate submission failing, with the same form being visible after
  // navigation.
  OnPasswordFormSubmitted(form);
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);
  EXPECT_EQ(form.username_value, form_to_save.username_value);
  // What was "new password" field in the submitted form, becomes the current
  // password field in the form to save.
  EXPECT_EQ(form.new_password_value, form_to_save.password_value);
}

// If the user edits the generated password, but does not remove it completely,
// it should stay treated as a generated password.
TEST_F(PasswordManagerTest, PasswordGenerationPasswordEdited) {
  std::vector<PasswordForm> observed;
  PasswordForm form(MakeFormWithOnlyNewPasswordField());
  observed.push_back(form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  manager()->SetHasGeneratedPasswordForForm(&driver_, form, true);

  // Simulate user editing and submitting a different password. Verify that
  // the edited password is the one that is saved.
  form.new_password_value = ASCIIToUTF16("different_password");
  OnPasswordFormSubmitted(form);

  // The user should not be presented with an infobar as they have already given
  // consent by using the generated password. The form should be saved once
  // navigation occurs. The client will be informed that automatic saving has
  // occurred.
  EXPECT_CALL(client_, PromptUserToSaveOrUpdatePasswordPtr(_, _)).Times(0);
  PasswordForm form_to_save;
  EXPECT_CALL(*store_, AddLogin(_)).WillOnce(SaveArg<0>(&form_to_save));
  EXPECT_CALL(client_, AutomaticPasswordSaveIndicator());

  // Simulate submission failing, with the same form being visible after
  // navigation.
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);
  EXPECT_EQ(form.username_value, form_to_save.username_value);
  // What was "new password" field in the submitted form, becomes the current
  // password field in the form to save.
  EXPECT_EQ(form.new_password_value, form_to_save.password_value);
}

// Generated password are saved even if it looks like the submit failed (the
// form reappeared). Verify that passwords which are no longer marked as
// generated will not be automatically saved.
TEST_F(PasswordManagerTest,
       PasswordGenerationNoLongerGeneratedPasswordNotForceSaved_FailedSubmit) {
  std::vector<PasswordForm> observed;
  PasswordForm form(MakeFormWithOnlyNewPasswordField());
  observed.push_back(form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  manager()->SetHasGeneratedPasswordForForm(&driver_, form, true);

  // Simulate user removing generated password and adding a new one.
  form.new_password_value = ASCIIToUTF16("different_password");
  manager()->SetHasGeneratedPasswordForForm(&driver_, form, false);

  OnPasswordFormSubmitted(form);

  // No infobar or prompt is shown if submission fails.
  EXPECT_CALL(client_, PromptUserToSaveOrUpdatePasswordPtr(_, _)).Times(0);
  EXPECT_CALL(client_, AutomaticPasswordSaveIndicator()).Times(0);

  // Simulate submission failing, with the same form being visible after
  // navigation.
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);
}

// Verify that passwords which are no longer generated trigger the confirmation
// dialog when submitted.
TEST_F(PasswordManagerTest,
       PasswordGenerationNoLongerGeneratedPasswordNotForceSaved) {
  std::vector<PasswordForm> observed;
  PasswordForm form(MakeFormWithOnlyNewPasswordField());
  observed.push_back(form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  manager()->SetHasGeneratedPasswordForForm(&driver_, form, true);

  // Simulate user removing generated password and adding a new one.
  form.new_password_value = ASCIIToUTF16("different_password");
  manager()->SetHasGeneratedPasswordForForm(&driver_, form, false);

  OnPasswordFormSubmitted(form);

  // Verify that a normal prompt is shown instead of the force saving UI.
  scoped_ptr<PasswordFormManager> form_to_save;
  EXPECT_CALL(client_,
              PromptUserToSaveOrUpdatePasswordPtr(
                  _, CredentialSourceType::CREDENTIAL_SOURCE_PASSWORD_MANAGER))
      .WillOnce(WithArg<0>(SaveToScopedPtr(&form_to_save)));
  EXPECT_CALL(client_, AutomaticPasswordSaveIndicator()).Times(0);

  // Simulate a successful submission.
  observed.clear();
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);
}

TEST_F(PasswordManagerTest, PasswordGenerationUsernameChanged) {
  std::vector<PasswordForm> observed;
  PasswordForm form(MakeFormWithOnlyNewPasswordField());
  observed.push_back(form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  manager()->SetHasGeneratedPasswordForForm(&driver_, form, true);

  // Simulate user changing the password and username, without ever completely
  // deleting the password.
  form.new_password_value = ASCIIToUTF16("different_password");
  form.username_value = ASCIIToUTF16("new_username");
  OnPasswordFormSubmitted(form);

  EXPECT_CALL(client_, PromptUserToSaveOrUpdatePasswordPtr(_, _)).Times(0);
  PasswordForm form_to_save;
  EXPECT_CALL(*store_, AddLogin(_)).WillOnce(SaveArg<0>(&form_to_save));
  EXPECT_CALL(client_, AutomaticPasswordSaveIndicator());

  observed.clear();
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);
  EXPECT_EQ(form.username_value, form_to_save.username_value);
  // What was "new password" field in the submitted form, becomes the current
  // password field in the form to save.
  EXPECT_EQ(form.new_password_value, form_to_save.password_value);
}

TEST_F(PasswordManagerTest, ForceSavingPasswords) {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kEnablePasswordForceSaving);
  PasswordForm form(MakeSimpleForm());

  std::vector<PasswordForm> observed;
  observed.push_back(form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  scoped_ptr<PasswordFormManager> form_manager_to_save;
  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(client_,
              PromptUserToSaveOrUpdatePasswordPtr(
                  _, CredentialSourceType::CREDENTIAL_SOURCE_PASSWORD_MANAGER))
      .WillOnce(WithArg<0>(SaveToScopedPtr(&form_manager_to_save)));
  manager()->OnPasswordFormForceSaveRequested(&driver_, form);
  ASSERT_TRUE(form_manager_to_save);
  EXPECT_EQ(form.password_value,
            PasswordFormManager::PasswordToSave(
                form_manager_to_save->pending_credentials()));
}

// Forcing Chrome to save an empty passwords should fail without a crash.
TEST_F(PasswordManagerTest, ForceSavingPasswords_Empty) {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kEnablePasswordForceSaving);
  PasswordForm empty_password_form;

  std::vector<PasswordForm> observed;
  observed.push_back(empty_password_form);
  EXPECT_CALL(*store_, GetLogins(_, _, _))
      .WillRepeatedly(WithArg<2>(InvokeEmptyConsumer()));
  manager()->OnPasswordFormsParsed(&driver_, observed);
  manager()->OnPasswordFormsRendered(&driver_, observed, true);

  EXPECT_CALL(client_, IsSavingEnabledForCurrentPage())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(client_, PromptUserToSaveOrUpdatePasswordPtr(_, _)).Times(0);
  manager()->OnPasswordFormForceSaveRequested(&driver_, empty_password_form);
}

}  // namespace password_manager
