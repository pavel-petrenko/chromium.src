// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/setup/me2me_native_messaging_host.h"

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/stl_util.h"
#include "base/strings/stringize_macros.h"
#include "base/values.h"
#include "google_apis/gaia/gaia_oauth_client.h"
#include "net/base/file_stream.h"
#include "net/base/net_util.h"
#include "remoting/base/auto_thread_task_runner.h"
#include "remoting/host/native_messaging/pipe_messaging_channel.h"
#include "remoting/host/pin_hash.h"
#include "remoting/host/setup/mock_oauth_client.h"
#include "remoting/host/setup/test_util.h"
#include "remoting/protocol/pairing_registry.h"
#include "remoting/protocol/protocol_mock_objects.h"
#include "testing/gtest/include/gtest/gtest.h"

using remoting::protocol::MockPairingRegistryDelegate;
using remoting::protocol::PairingRegistry;
using remoting::protocol::SynchronousPairingRegistry;

namespace {

void VerifyHelloResponse(scoped_ptr<base::DictionaryValue> response) {
  ASSERT_TRUE(response);
  std::string value;
  EXPECT_TRUE(response->GetString("type", &value));
  EXPECT_EQ("helloResponse", value);
  EXPECT_TRUE(response->GetString("version", &value));

  // The check below will compile but fail if VERSION isn't defined (STRINGIZE
  // silently converts undefined values).
  #ifndef VERSION
  #error VERSION must be defined
  #endif
  EXPECT_EQ(STRINGIZE(VERSION), value);
}

void VerifyGetHostNameResponse(scoped_ptr<base::DictionaryValue> response) {
  ASSERT_TRUE(response);
  std::string value;
  EXPECT_TRUE(response->GetString("type", &value));
  EXPECT_EQ("getHostNameResponse", value);
  EXPECT_TRUE(response->GetString("hostname", &value));
  EXPECT_EQ(net::GetHostName(), value);
}

void VerifyGetPinHashResponse(scoped_ptr<base::DictionaryValue> response) {
  ASSERT_TRUE(response);
  std::string value;
  EXPECT_TRUE(response->GetString("type", &value));
  EXPECT_EQ("getPinHashResponse", value);
  EXPECT_TRUE(response->GetString("hash", &value));
  EXPECT_EQ(remoting::MakeHostPinHash("my_host", "1234"), value);
}

void VerifyGenerateKeyPairResponse(scoped_ptr<base::DictionaryValue> response) {
  ASSERT_TRUE(response);
  std::string value;
  EXPECT_TRUE(response->GetString("type", &value));
  EXPECT_EQ("generateKeyPairResponse", value);
  EXPECT_TRUE(response->GetString("privateKey", &value));
  EXPECT_TRUE(response->GetString("publicKey", &value));
}

void VerifyGetDaemonConfigResponse(scoped_ptr<base::DictionaryValue> response) {
  ASSERT_TRUE(response);
  std::string value;
  EXPECT_TRUE(response->GetString("type", &value));
  EXPECT_EQ("getDaemonConfigResponse", value);
  const base::DictionaryValue* config = nullptr;
  EXPECT_TRUE(response->GetDictionary("config", &config));
  EXPECT_TRUE(base::DictionaryValue().Equals(config));
}

void VerifyGetUsageStatsConsentResponse(
    scoped_ptr<base::DictionaryValue> response) {
  ASSERT_TRUE(response);
  std::string value;
  EXPECT_TRUE(response->GetString("type", &value));
  EXPECT_EQ("getUsageStatsConsentResponse", value);
  bool supported, allowed, set_by_policy;
  EXPECT_TRUE(response->GetBoolean("supported", &supported));
  EXPECT_TRUE(response->GetBoolean("allowed", &allowed));
  EXPECT_TRUE(response->GetBoolean("setByPolicy", &set_by_policy));
  EXPECT_TRUE(supported);
  EXPECT_TRUE(allowed);
  EXPECT_TRUE(set_by_policy);
}

void VerifyStopDaemonResponse(scoped_ptr<base::DictionaryValue> response) {
  ASSERT_TRUE(response);
  std::string value;
  EXPECT_TRUE(response->GetString("type", &value));
  EXPECT_EQ("stopDaemonResponse", value);
  EXPECT_TRUE(response->GetString("result", &value));
  EXPECT_EQ("OK", value);
}

void VerifyGetDaemonStateResponse(scoped_ptr<base::DictionaryValue> response) {
  ASSERT_TRUE(response);
  std::string value;
  EXPECT_TRUE(response->GetString("type", &value));
  EXPECT_EQ("getDaemonStateResponse", value);
  EXPECT_TRUE(response->GetString("state", &value));
  EXPECT_EQ("STARTED", value);
}

void VerifyUpdateDaemonConfigResponse(
    scoped_ptr<base::DictionaryValue> response) {
  ASSERT_TRUE(response);
  std::string value;
  EXPECT_TRUE(response->GetString("type", &value));
  EXPECT_EQ("updateDaemonConfigResponse", value);
  EXPECT_TRUE(response->GetString("result", &value));
  EXPECT_EQ("OK", value);
}

void VerifyStartDaemonResponse(scoped_ptr<base::DictionaryValue> response) {
  ASSERT_TRUE(response);
  std::string value;
  EXPECT_TRUE(response->GetString("type", &value));
  EXPECT_EQ("startDaemonResponse", value);
  EXPECT_TRUE(response->GetString("result", &value));
  EXPECT_EQ("OK", value);
}

void VerifyGetCredentialsFromAuthCodeResponse(
    scoped_ptr<base::DictionaryValue> response) {
  ASSERT_TRUE(response);
  std::string value;
  EXPECT_TRUE(response->GetString("type", &value));
  EXPECT_EQ("getCredentialsFromAuthCodeResponse", value);
  EXPECT_TRUE(response->GetString("userEmail", &value));
  EXPECT_EQ("fake_user_email", value);
  EXPECT_TRUE(response->GetString("refreshToken", &value));
  EXPECT_EQ("fake_refresh_token", value);
}

}  // namespace

namespace remoting {

class MockDaemonControllerDelegate : public DaemonController::Delegate {
 public:
  MockDaemonControllerDelegate();
  ~MockDaemonControllerDelegate() override;

  // DaemonController::Delegate interface.
  DaemonController::State GetState() override;
  scoped_ptr<base::DictionaryValue> GetConfig() override;
  void SetConfigAndStart(
      scoped_ptr<base::DictionaryValue> config,
      bool consent,
      const DaemonController::CompletionCallback& done) override;
  void UpdateConfig(scoped_ptr<base::DictionaryValue> config,
                    const DaemonController::CompletionCallback& done) override;
  void Stop(const DaemonController::CompletionCallback& done) override;
  DaemonController::UsageStatsConsent GetUsageStatsConsent() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockDaemonControllerDelegate);
};

MockDaemonControllerDelegate::MockDaemonControllerDelegate() {}

MockDaemonControllerDelegate::~MockDaemonControllerDelegate() {}

DaemonController::State MockDaemonControllerDelegate::GetState() {
  return DaemonController::STATE_STARTED;
}

scoped_ptr<base::DictionaryValue> MockDaemonControllerDelegate::GetConfig() {
  return make_scoped_ptr(new base::DictionaryValue());
}

void MockDaemonControllerDelegate::SetConfigAndStart(
    scoped_ptr<base::DictionaryValue> config,
    bool consent,
    const DaemonController::CompletionCallback& done) {

  // Verify parameters passed in.
  if (consent && config && config->HasKey("start")) {
    done.Run(DaemonController::RESULT_OK);
  } else {
    done.Run(DaemonController::RESULT_FAILED);
  }
}

void MockDaemonControllerDelegate::UpdateConfig(
    scoped_ptr<base::DictionaryValue> config,
    const DaemonController::CompletionCallback& done) {
  if (config && config->HasKey("update")) {
    done.Run(DaemonController::RESULT_OK);
  } else {
    done.Run(DaemonController::RESULT_FAILED);
  }
}

void MockDaemonControllerDelegate::Stop(
    const DaemonController::CompletionCallback& done) {
  done.Run(DaemonController::RESULT_OK);
}

DaemonController::UsageStatsConsent
MockDaemonControllerDelegate::GetUsageStatsConsent() {
  DaemonController::UsageStatsConsent consent;
  consent.supported = true;
  consent.allowed = true;
  consent.set_by_policy = true;
  return consent;
}

class Me2MeNativeMessagingHostTest : public testing::Test {
 public:
  Me2MeNativeMessagingHostTest();
  ~Me2MeNativeMessagingHostTest() override;

  void SetUp() override;
  void TearDown() override;

  scoped_ptr<base::DictionaryValue> ReadMessageFromOutputPipe();

  void WriteMessageToInputPipe(const base::Value& message);

  // The Host process should shut down when it receives a malformed request.
  // This is tested by sending a known-good request, followed by |message|,
  // followed by the known-good request again. The response file should only
  // contain a single response from the first good request.
  void TestBadRequest(const base::Value& message);

 protected:
  // Reference to the MockDaemonControllerDelegate, which is owned by
  // |channel_|.
  MockDaemonControllerDelegate* daemon_controller_delegate_;

 private:
  void StartHost();
  void StopHost();
  void ExitTest();

  // Each test creates two unidirectional pipes: "input" and "output".
  // Me2MeNativeMessagingHost reads from input_read_handle and writes to
  // output_write_file. The unittest supplies data to input_write_handle, and
  // verifies output from output_read_handle.
  //
  // unittest -> [input] -> Me2MeNativeMessagingHost -> [output] -> unittest
  base::File input_write_file_;
  base::File output_read_file_;

  // Message loop of the test thread.
  scoped_ptr<base::MessageLoop> test_message_loop_;
  scoped_ptr<base::RunLoop> test_run_loop_;

  scoped_ptr<base::Thread> host_thread_;
  scoped_ptr<base::RunLoop> host_run_loop_;

  // Task runner of the host thread.
  scoped_refptr<AutoThreadTaskRunner> host_task_runner_;
  scoped_ptr<remoting::Me2MeNativeMessagingHost> host_;

  DISALLOW_COPY_AND_ASSIGN(Me2MeNativeMessagingHostTest);
};

Me2MeNativeMessagingHostTest::Me2MeNativeMessagingHostTest() {}

Me2MeNativeMessagingHostTest::~Me2MeNativeMessagingHostTest() {}

void Me2MeNativeMessagingHostTest::SetUp() {
  base::File input_read_file;
  base::File output_write_file;

  ASSERT_TRUE(MakePipe(&input_read_file, &input_write_file_));
  ASSERT_TRUE(MakePipe(&output_read_file_, &output_write_file));

  test_message_loop_.reset(new base::MessageLoop());
  test_run_loop_.reset(new base::RunLoop());

  // Run the host on a dedicated thread.
  host_thread_.reset(new base::Thread("host_thread"));
  host_thread_->Start();

  // Arrange to run |test_message_loop_| until no components depend on it.
  host_task_runner_ = new AutoThreadTaskRunner(
      host_thread_->message_loop_proxy(),
      base::Bind(&Me2MeNativeMessagingHostTest::ExitTest,
                 base::Unretained(this)));

  host_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&Me2MeNativeMessagingHostTest::StartHost,
                 base::Unretained(this)));

  // Wait until the host finishes starting.
  test_run_loop_->Run();
}

void Me2MeNativeMessagingHostTest::StartHost() {
  DCHECK(host_task_runner_->RunsTasksOnCurrentThread());

  base::File input_read_file;
  base::File output_write_file;

  ASSERT_TRUE(MakePipe(&input_read_file, &input_write_file_));
  ASSERT_TRUE(MakePipe(&output_read_file_, &output_write_file));

  daemon_controller_delegate_ = new MockDaemonControllerDelegate();
  scoped_refptr<DaemonController> daemon_controller(
      new DaemonController(make_scoped_ptr(daemon_controller_delegate_)));

  scoped_refptr<PairingRegistry> pairing_registry =
      new SynchronousPairingRegistry(
          make_scoped_ptr(new MockPairingRegistryDelegate()));

  scoped_ptr<extensions::NativeMessagingChannel> channel(
      new PipeMessagingChannel(input_read_file.Pass(),
                               output_write_file.Pass()));

  scoped_ptr<OAuthClient> oauth_client(
      new MockOAuthClient("fake_user_email", "fake_refresh_token"));

  host_.reset(new Me2MeNativeMessagingHost(false, 0, channel.Pass(),
                                           daemon_controller, pairing_registry,
                                           oauth_client.Pass()));
  host_->Start(base::Bind(&Me2MeNativeMessagingHostTest::StopHost,
                          base::Unretained(this)));

  // Notify the test that the host has finished starting up.
  test_message_loop_->message_loop_proxy()->PostTask(
      FROM_HERE, test_run_loop_->QuitClosure());
}

void Me2MeNativeMessagingHostTest::StopHost() {
  DCHECK(host_task_runner_->RunsTasksOnCurrentThread());

  host_.reset();

  // Wait till all shutdown tasks have completed.
  base::RunLoop().RunUntilIdle();

  // Trigger a test shutdown via ExitTest().
  host_task_runner_ = nullptr;
}

void Me2MeNativeMessagingHostTest::ExitTest() {
  if (!test_message_loop_->message_loop_proxy()->RunsTasksOnCurrentThread()) {
    test_message_loop_->message_loop_proxy()->PostTask(
        FROM_HERE,
        base::Bind(&Me2MeNativeMessagingHostTest::ExitTest,
                   base::Unretained(this)));
    return;
  }
  test_run_loop_->Quit();
}

void Me2MeNativeMessagingHostTest::TearDown() {
  // Closing the write-end of the input will send an EOF to the native
  // messaging reader. This will trigger a host shutdown.
  input_write_file_.Close();

  // Start a new RunLoop and Wait until the host finishes shutting down.
  test_run_loop_.reset(new base::RunLoop());
  test_run_loop_->Run();

  // Verify there are no more message in the output pipe.
  scoped_ptr<base::DictionaryValue> response = ReadMessageFromOutputPipe();
  EXPECT_FALSE(response);

  // The It2MeMe2MeNativeMessagingHost dtor closes the handles that are passed
  // to it. So the only handle left to close is |output_read_file_|.
  output_read_file_.Close();
}

scoped_ptr<base::DictionaryValue>
Me2MeNativeMessagingHostTest::ReadMessageFromOutputPipe() {
  uint32 length;
  int read_result = output_read_file_.ReadAtCurrentPos(
      reinterpret_cast<char*>(&length), sizeof(length));
  if (read_result != sizeof(length)) {
    return nullptr;
  }

  std::string message_json(length, '\0');
  read_result = output_read_file_.ReadAtCurrentPos(
      string_as_array(&message_json), length);
  if (read_result != static_cast<int>(length)) {
    return nullptr;
  }

  scoped_ptr<base::Value> message = base::JSONReader::Read(message_json);
  if (!message || !message->IsType(base::Value::TYPE_DICTIONARY)) {
    return nullptr;
  }

  return make_scoped_ptr(
      static_cast<base::DictionaryValue*>(message.release()));
}

void Me2MeNativeMessagingHostTest::WriteMessageToInputPipe(
    const base::Value& message) {
  std::string message_json;
  base::JSONWriter::Write(message, &message_json);

  uint32 length = message_json.length();
  input_write_file_.WriteAtCurrentPos(reinterpret_cast<char*>(&length),
                                      sizeof(length));
  input_write_file_.WriteAtCurrentPos(message_json.data(), length);
}

void Me2MeNativeMessagingHostTest::TestBadRequest(const base::Value& message) {
  base::DictionaryValue good_message;
  good_message.SetString("type", "hello");

  // This test currently relies on synchronous processing of hello messages and
  // message parameters verification.
  WriteMessageToInputPipe(good_message);
  WriteMessageToInputPipe(message);
  WriteMessageToInputPipe(good_message);

  // Read from output pipe, and verify responses.
  scoped_ptr<base::DictionaryValue> response = ReadMessageFromOutputPipe();
  VerifyHelloResponse(response.Pass());

  response = ReadMessageFromOutputPipe();
  EXPECT_FALSE(response);
}

// TODO (weitaosu): crbug.com/323306. Re-enable these tests.
// Test all valid request-types.
TEST_F(Me2MeNativeMessagingHostTest, All) {
  int next_id = 0;
  base::DictionaryValue message;
  message.SetInteger("id", next_id++);
  message.SetString("type", "hello");
  WriteMessageToInputPipe(message);

  message.SetInteger("id", next_id++);
  message.SetString("type", "getHostName");
  WriteMessageToInputPipe(message);

  message.SetInteger("id", next_id++);
  message.SetString("type", "getPinHash");
  message.SetString("hostId", "my_host");
  message.SetString("pin", "1234");
  WriteMessageToInputPipe(message);

  message.Clear();
  message.SetInteger("id", next_id++);
  message.SetString("type", "generateKeyPair");
  WriteMessageToInputPipe(message);

  message.SetInteger("id", next_id++);
  message.SetString("type", "getDaemonConfig");
  WriteMessageToInputPipe(message);

  message.SetInteger("id", next_id++);
  message.SetString("type", "getUsageStatsConsent");
  WriteMessageToInputPipe(message);

  message.SetInteger("id", next_id++);
  message.SetString("type", "stopDaemon");
  WriteMessageToInputPipe(message);

  message.SetInteger("id", next_id++);
  message.SetString("type", "getDaemonState");
  WriteMessageToInputPipe(message);

  // Following messages require a "config" dictionary.
  base::DictionaryValue config;
  config.SetBoolean("update", true);
  message.Set("config", config.DeepCopy());
  message.SetInteger("id", next_id++);
  message.SetString("type", "updateDaemonConfig");
  WriteMessageToInputPipe(message);

  config.Clear();
  config.SetBoolean("start", true);
  message.Set("config", config.DeepCopy());
  message.SetBoolean("consent", true);
  message.SetInteger("id", next_id++);
  message.SetString("type", "startDaemon");
  WriteMessageToInputPipe(message);

  message.SetInteger("id", next_id++);
  message.SetString("type", "getCredentialsFromAuthCode");
  message.SetString("authorizationCode", "fake_auth_code");
  WriteMessageToInputPipe(message);

  void (*verify_routines[])(scoped_ptr<base::DictionaryValue>) = {
      &VerifyHelloResponse,
      &VerifyGetHostNameResponse,
      &VerifyGetPinHashResponse,
      &VerifyGenerateKeyPairResponse,
      &VerifyGetDaemonConfigResponse,
      &VerifyGetUsageStatsConsentResponse,
      &VerifyStopDaemonResponse,
      &VerifyGetDaemonStateResponse,
      &VerifyUpdateDaemonConfigResponse,
      &VerifyStartDaemonResponse,
      &VerifyGetCredentialsFromAuthCodeResponse,
  };
  ASSERT_EQ(arraysize(verify_routines), static_cast<size_t>(next_id));

  // Read all responses from output pipe, and verify them.
  for (int i = 0; i < next_id; ++i) {
    scoped_ptr<base::DictionaryValue> response = ReadMessageFromOutputPipe();

    // Make sure that id is available and is in the range.
    int id;
    ASSERT_TRUE(response->GetInteger("id", &id));
    ASSERT_TRUE(0 <= id && id < next_id);

    // Call the verification routine corresponding to the message id.
    ASSERT_TRUE(verify_routines[id]);
    verify_routines[id](response.Pass());

    // Clear the pointer so that the routine cannot be called the second time.
    verify_routines[id] = nullptr;
  }
}

// Verify that response ID matches request ID.
TEST_F(Me2MeNativeMessagingHostTest, Id) {
  base::DictionaryValue message;
  message.SetString("type", "hello");
  WriteMessageToInputPipe(message);
  message.SetString("id", "42");
  WriteMessageToInputPipe(message);

  scoped_ptr<base::DictionaryValue> response = ReadMessageFromOutputPipe();
  EXPECT_TRUE(response);
  std::string value;
  EXPECT_FALSE(response->GetString("id", &value));

  response = ReadMessageFromOutputPipe();
  EXPECT_TRUE(response);
  EXPECT_TRUE(response->GetString("id", &value));
  EXPECT_EQ("42", value);
}

// Verify non-Dictionary requests are rejected.
TEST_F(Me2MeNativeMessagingHostTest, WrongFormat) {
  base::ListValue message;
  TestBadRequest(message);
}

// Verify requests with no type are rejected.
TEST_F(Me2MeNativeMessagingHostTest, MissingType) {
  base::DictionaryValue message;
  TestBadRequest(message);
}

// Verify rejection if type is unrecognized.
TEST_F(Me2MeNativeMessagingHostTest, InvalidType) {
  base::DictionaryValue message;
  message.SetString("type", "xxx");
  TestBadRequest(message);
}

// Verify rejection if getPinHash request has no hostId.
TEST_F(Me2MeNativeMessagingHostTest, GetPinHashNoHostId) {
  base::DictionaryValue message;
  message.SetString("type", "getPinHash");
  message.SetString("pin", "1234");
  TestBadRequest(message);
}

// Verify rejection if getPinHash request has no pin.
TEST_F(Me2MeNativeMessagingHostTest, GetPinHashNoPin) {
  base::DictionaryValue message;
  message.SetString("type", "getPinHash");
  message.SetString("hostId", "my_host");
  TestBadRequest(message);
}

// Verify rejection if updateDaemonConfig request has invalid config.
TEST_F(Me2MeNativeMessagingHostTest, UpdateDaemonConfigInvalidConfig) {
  base::DictionaryValue message;
  message.SetString("type", "updateDaemonConfig");
  message.SetString("config", "xxx");
  TestBadRequest(message);
}

// Verify rejection if startDaemon request has invalid config.
TEST_F(Me2MeNativeMessagingHostTest, StartDaemonInvalidConfig) {
  base::DictionaryValue message;
  message.SetString("type", "startDaemon");
  message.SetString("config", "xxx");
  message.SetBoolean("consent", true);
  TestBadRequest(message);
}

// Verify rejection if startDaemon request has no "consent" parameter.
TEST_F(Me2MeNativeMessagingHostTest, StartDaemonNoConsent) {
  base::DictionaryValue message;
  message.SetString("type", "startDaemon");
  message.Set("config", base::DictionaryValue().DeepCopy());
  TestBadRequest(message);
}

// Verify rejection if getCredentialsFromAuthCode has no auth code.
TEST_F(Me2MeNativeMessagingHostTest, GetCredentialsFromAuthCodeNoAuthCode) {
  base::DictionaryValue message;
  message.SetString("type", "getCredentialsFromAuthCode");
  TestBadRequest(message);
}

}  // namespace remoting
