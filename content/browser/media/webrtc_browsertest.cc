// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "content/browser/media/webrtc_internals.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "content/test/webrtc_content_browsertest_base.h"
#include "media/audio/audio_manager.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif

namespace content {

class WebRtcBrowserTest : public WebRtcContentBrowserTest,
                          public testing::WithParamInterface<bool> {
 public:
  WebRtcBrowserTest() {}
  virtual ~WebRtcBrowserTest() {}

  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    WebRtcContentBrowserTest::SetUpCommandLine(command_line);

    bool enable_audio_track_processing = GetParam();
    if (enable_audio_track_processing)
      command_line->AppendSwitch(switches::kEnableAudioTrackProcessing);
  }

  // Convenience function since most peerconnection-call.html tests just load
  // the page, kick off some javascript and wait for the title to change to OK.
  void MakeTypicalPeerConnectionCall(const std::string& javascript) {
    ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

    GURL url(embedded_test_server()->GetURL("/media/peerconnection-call.html"));
    NavigateToURL(shell(), url);

    DisableOpusIfOnAndroid();
    ExecuteJavascriptAndWaitForOk(javascript);
  }

  void DisableOpusIfOnAndroid() {
#if defined (OS_ANDROID)
    // Always force iSAC 16K on Android for now (Opus is broken).
    EXPECT_EQ("isac-forced",
              ExecuteJavascriptAndReturnResult("forceIsac16KInSdp();"));
#endif
  }
};

static const bool kRunTestsWithFlag[] = { false, true };
INSTANTIATE_TEST_CASE_P(WebRtcBrowserTests,
                        WebRtcBrowserTest,
                        testing::ValuesIn(kRunTestsWithFlag));

#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux bot: http://crbug.com/238490
#define MAYBE_CanSetupDefaultVideoCall DISABLED_CanSetupDefaultVideoCall
#else
#define MAYBE_CanSetupDefaultVideoCall CanSetupDefaultVideoCall
#endif

// These tests will make a complete PeerConnection-based call and verify that
// video is playing for the call.
IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest, MAYBE_CanSetupDefaultVideoCall) {
  MakeTypicalPeerConnectionCall(
      "callAndExpectResolution({video: true}, 640, 480);");
}

IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest, CanSetupVideoCallWith1To1AspecRatio) {
  const std::string javascript =
      "callAndExpectResolution({video: {mandatory: {minWidth: 320,"
      " maxWidth: 320, minHeight: 320, maxHeight: 320}}}, 320, 320);";
  MakeTypicalPeerConnectionCall(javascript);
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux, see http://crbug.com/240376
#define MAYBE_CanSetupAudioAndVideoCall DISABLED_CanSetupAudioAndVideoCall
#else
#define MAYBE_CanSetupAudioAndVideoCall CanSetupAudioAndVideoCall
#endif

IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest, MAYBE_CanSetupAudioAndVideoCall) {
  MakeTypicalPeerConnectionCall("call({video: true, audio: true});");
}

IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest, MANUAL_CanSetupCallAndSendDtmf) {
  MakeTypicalPeerConnectionCall("callAndSendDtmf(\'123,abc\');");
}

// TODO(phoglund): this test fails because the peer connection state will be
// stable in the second negotiation round rather than have-local-offer.
// http://crbug.com/293125.
IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest,
                       DISABLED_CanMakeEmptyCallThenAddStreamsAndRenegotiate) {
  const char* kJavascript =
      "callEmptyThenAddOneStreamAndRenegotiate({video: true, audio: true});";
  MakeTypicalPeerConnectionCall(kJavascript);
}

// Below 2 test will make a complete PeerConnection-based call between pc1 and
// pc2, and then use the remote stream to setup a call between pc3 and pc4, and
// then verify that video is received on pc3 and pc4.
// Flaky on win xp. http://crbug.com/304775
#if defined(OS_WIN)
#define MAYBE_CanForwardRemoteStream DISABLED_CanForwardRemoteStream
#define MAYBE_CanForwardRemoteStream720p DISABLED_CanForwardRemoteStream720p
#else
#define MAYBE_CanForwardRemoteStream CanForwardRemoteStream
#define MAYBE_CanForwardRemoteStream720p CanForwardRemoteStream720p
#endif
IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest, MAYBE_CanForwardRemoteStream) {
  MakeTypicalPeerConnectionCall(
      "callAndForwardRemoteStream({video: true, audio: false});");
}

IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest, MAYBE_CanForwardRemoteStream720p) {
  const std::string javascript = GenerateGetUserMediaCall(
      "callAndForwardRemoteStream", 1280, 1280, 720, 720, 10, 30);
  MakeTypicalPeerConnectionCall(javascript);
}

// This test will make a complete PeerConnection-based call but remove the
// MSID and bundle attribute from the initial offer to verify that
// video is playing for the call even if the initiating client don't support
// MSID. http://tools.ietf.org/html/draft-alvestrand-rtcweb-msid-02
#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux, see http://crbug.com/240373
#define MAYBE_CanSetupAudioAndVideoCallWithoutMsidAndBundle\
        DISABLED_CanSetupAudioAndVideoCallWithoutMsidAndBundle
#else
#define MAYBE_CanSetupAudioAndVideoCallWithoutMsidAndBundle\
        CanSetupAudioAndVideoCallWithoutMsidAndBundle
#endif
IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest,
                       MAYBE_CanSetupAudioAndVideoCallWithoutMsidAndBundle) {
  MakeTypicalPeerConnectionCall("callWithoutMsidAndBundle();");
}

// This test will modify the SDP offer to an unsupported codec, which should
// cause SetLocalDescription to fail.
#if defined(USE_OZONE)
// Disabled for Ozone, see http://crbug.com/315392#c15
#define MAYBE_NegotiateUnsupportedVideoCodec\
    DISABLED_NegotiateUnsupportedVideoCodec
#else
#define MAYBE_NegotiateUnsupportedVideoCodec NegotiateUnsupportedVideoCodec
#endif

IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest,
                       MAYBE_NegotiateUnsupportedVideoCodec) {
  MakeTypicalPeerConnectionCall("negotiateUnsupportedVideoCodec();");
}

// This test will modify the SDP offer to use no encryption, which should
// cause SetLocalDescription to fail.
#if defined(USE_OZONE)
// Disabled for Ozone, see http://crbug.com/315392#c15
#define MAYBE_NegotiateNonCryptoCall DISABLED_NegotiateNonCryptoCall
#else
#define MAYBE_NegotiateNonCryptoCall NegotiateNonCryptoCall
#endif

IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest, MAYBE_NegotiateNonCryptoCall) {
  MakeTypicalPeerConnectionCall("negotiateNonCryptoCall();");
}

// This test can negotiate an SDP offer that includes a b=AS:xx to control
// the bandwidth for audio and video
IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest, NegotiateOfferWithBLine) {
  MakeTypicalPeerConnectionCall("negotiateOfferWithBLine();");
}

// This test will make a complete PeerConnection-based call using legacy SDP
// settings: GIce, external SDES, and no BUNDLE.
#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux, see http://crbug.com/240373
#define MAYBE_CanSetupLegacyCall DISABLED_CanSetupLegacyCall
#else
#define MAYBE_CanSetupLegacyCall CanSetupLegacyCall
#endif

IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest, MAYBE_CanSetupLegacyCall) {
  MakeTypicalPeerConnectionCall("callWithLegacySdp();");
}

// This test will make a PeerConnection-based call and test an unreliable text
// dataChannel.
// TODO(mallinath) - Remove this test after rtp based data channel is disabled.
IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest, CallWithDataOnly) {
  MakeTypicalPeerConnectionCall("callWithDataOnly();");
}

IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest, CallWithSctpDataOnly) {
  MakeTypicalPeerConnectionCall("callWithSctpDataOnly();");
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux bot: http://crbug.com/238490
#define MAYBE_CallWithDataAndMedia DISABLED_CallWithDataAndMedia
#else
#define MAYBE_CallWithDataAndMedia CallWithDataAndMedia
#endif

// This test will make a PeerConnection-based call and test an unreliable text
// dataChannel and audio and video tracks.
// TODO(mallinath) - Remove this test after rtp based data channel is disabled.
IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest, MAYBE_CallWithDataAndMedia) {
  MakeTypicalPeerConnectionCall("callWithDataAndMedia();");
}


#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux bot: http://crbug.com/238490
#define MAYBE_CallWithSctpDataAndMedia DISABLED_CallWithSctpDataAndMedia
#else
#define MAYBE_CallWithSctpDataAndMedia CallWithSctpDataAndMedia
#endif

IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest,
                       MAYBE_CallWithSctpDataAndMedia) {
  MakeTypicalPeerConnectionCall("callWithSctpDataAndMedia();");
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux bot: http://crbug.com/238490
#define MAYBE_CallWithDataAndLaterAddMedia DISABLED_CallWithDataAndLaterAddMedia
#else
// Temporarily disable the test on all platforms. http://crbug.com/293252
#define MAYBE_CallWithDataAndLaterAddMedia DISABLED_CallWithDataAndLaterAddMedia
#endif

// This test will make a PeerConnection-based call and test an unreliable text
// dataChannel and later add an audio and video track.
IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest, MAYBE_CallWithDataAndLaterAddMedia) {
  MakeTypicalPeerConnectionCall("callWithDataAndLaterAddMedia();");
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux bot: http://crbug.com/238490
#define MAYBE_CallWithNewVideoMediaStream DISABLED_CallWithNewVideoMediaStream
#else
#define MAYBE_CallWithNewVideoMediaStream CallWithNewVideoMediaStream
#endif

// This test will make a PeerConnection-based call and send a new Video
// MediaStream that has been created based on a MediaStream created with
// getUserMedia.
IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest, MAYBE_CallWithNewVideoMediaStream) {
  MakeTypicalPeerConnectionCall("callWithNewVideoMediaStream();");
}

// This test will make a PeerConnection-based call and send a new Video
// MediaStream that has been created based on a MediaStream created with
// getUserMedia. When video is flowing, the VideoTrack is removed and an
// AudioTrack is added instead.
// TODO(phoglund): This test is manual since not all buildbots has an audio
// input.
IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest, MANUAL_CallAndModifyStream) {
  MakeTypicalPeerConnectionCall(
      "callWithNewVideoMediaStreamLaterSwitchToAudio();");
}

IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest, AddTwoMediaStreamsToOnePC) {
  MakeTypicalPeerConnectionCall("addTwoMediaStreamsToOneConnection();");
}

IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest,
                       EstablishAudioVideoCallAndMeasureOutputLevel) {
  if (!media::AudioManager::Get()->HasAudioOutputDevices()) {
    // Bots with no output devices will force the audio code into a different
    // path where it doesn't manage to set either the low or high latency path.
    // This test will compute useless values in that case, so skip running on
    // such bots (see crbug.com/326338).
    LOG(INFO) << "Missing output devices: skipping test...";
    return;
  }

  ASSERT_TRUE(CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kUseFakeDeviceForMediaStream))
          << "Must run with fake devices since the test will explicitly look "
          << "for the fake device signal.";

  MakeTypicalPeerConnectionCall("callAndEnsureAudioIsPlaying();");
}

#if defined(OS_ANDROID)
// Flaky on Android: http://crbug.com/362432
#define MAYBE_EstablishAudioVideoCallAndVerifyMutingWorks \
    FLAKY_EstablishAudioVideoCallAndVerifyMutingWorks
#else
#define MAYBE_EstablishAudioVideoCallAndVerifyMutingWorks \
    EstablishAudioVideoCallAndVerifyMutingWorks
#endif

IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest,
                       MAYBE_EstablishAudioVideoCallAndVerifyMutingWorks) {
  if (!media::AudioManager::Get()->HasAudioOutputDevices()) {
    // Bots with no output devices will force the audio code into a different
    // path where it doesn't manage to set either the low or high latency path.
    // This test will compute useless values in that case, so skip running on
    // such bots (see crbug.com/326338).
    LOG(INFO) << "Missing output devices: skipping test...";
    return;
  }

  ASSERT_TRUE(CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kUseFakeDeviceForMediaStream))
          << "Must run with fake devices since the test will explicitly look "
          << "for the fake device signal.";

  MakeTypicalPeerConnectionCall("callAndEnsureAudioTrackMutingWorks();");
}

IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest,
                       EstablishAudioVideoCallAndVerifyUnmutingWorks) {
  if (!media::AudioManager::Get()->HasAudioOutputDevices()) {
    // See comment on EstablishAudioVideoCallAndVerifyMutingWorks.
    LOG(INFO) << "Missing output devices: skipping test...";
    return;
  }

  ASSERT_TRUE(CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kUseFakeDeviceForMediaStream))
          << "Must run with fake devices since the test will explicitly look "
          << "for the fake device signal.";

  MakeTypicalPeerConnectionCall("callAndEnsureAudioTrackUnmutingWorks();");
}

IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest, CallAndVerifyVideoMutingWorks) {
  MakeTypicalPeerConnectionCall("callAndEnsureVideoTrackMutingWorks();");
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux bot: http://crbug.com/238490
#define MAYBE_CallWithAecDump DISABLED_CallWithAecDump
#else
#define MAYBE_CallWithAecDump CallWithAecDump
#endif

// This tests will make a complete PeerConnection-based call, verify that
// video is playing for the call, and verify that a non-empty AEC dump file
// exists. The AEC dump is enabled through webrtc-internals. The HTML and
// Javascript is bypassed since it would trigger a file picker dialog. Instead,
// the dialog callback FileSelected() is invoked directly. In fact, there's
// never a webrtc-internals page opened at all since that's not needed.
IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest, MAYBE_CallWithAecDump) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // We must navigate somewhere first so that the render process is created.
  NavigateToURL(shell(), GURL(""));

  base::FilePath dump_file;
  ASSERT_TRUE(CreateTemporaryFile(&dump_file));

  // This fakes the behavior of another open tab with webrtc-internals, and
  // enabling AEC dump in that tab.
  WebRTCInternals::GetInstance()->FileSelected(dump_file, -1, NULL);

  GURL url(embedded_test_server()->GetURL("/media/peerconnection-call.html"));
  NavigateToURL(shell(), url);
  DisableOpusIfOnAndroid();
  ExecuteJavascriptAndWaitForOk("call({video: true, audio: true});");

  EXPECT_TRUE(base::PathExists(dump_file));
  int64 file_size = 0;
  EXPECT_TRUE(base::GetFileSize(dump_file, &file_size));
  EXPECT_GT(file_size, 0);

  base::DeleteFile(dump_file, false);
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux bot: http://crbug.com/238490
#define MAYBE_CallWithAecDumpEnabledThenDisabled DISABLED_CallWithAecDumpEnabledThenDisabled
#else
#define MAYBE_CallWithAecDumpEnabledThenDisabled CallWithAecDumpEnabledThenDisabled
#endif

// As above, but enable and disable dump before starting a call. The file should
// be created, but should be empty.
IN_PROC_BROWSER_TEST_P(WebRtcBrowserTest,
                       MAYBE_CallWithAecDumpEnabledThenDisabled) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // We must navigate somewhere first so that the render process is created.
  NavigateToURL(shell(), GURL(""));

  base::FilePath dump_file;
  ASSERT_TRUE(CreateTemporaryFile(&dump_file));

  // This fakes the behavior of another open tab with webrtc-internals, and
  // enabling AEC dump in that tab, then disabling it.
  WebRTCInternals::GetInstance()->FileSelected(dump_file, -1, NULL);
  WebRTCInternals::GetInstance()->DisableAecDump();

  GURL url(embedded_test_server()->GetURL("/media/peerconnection-call.html"));
  NavigateToURL(shell(), url);
  DisableOpusIfOnAndroid();
  ExecuteJavascriptAndWaitForOk("call({video: true, audio: true});");

  EXPECT_TRUE(base::PathExists(dump_file));
  int64 file_size = 0;
  EXPECT_TRUE(base::GetFileSize(dump_file, &file_size));
  EXPECT_EQ(0, file_size);

  base::DeleteFile(dump_file, false);
}

}  // namespace content
