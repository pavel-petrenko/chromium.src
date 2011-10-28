// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop.h"
#include "base/string_number_conversions.h"
#include "chrome/browser/chromeos/dbus/dbus_thread_manager.h"
#include "chrome/browser/chromeos/dbus/speech_synthesizer_client.h"
#include "chrome/browser/extensions/extension_tts_api_controller.h"
#include "chrome/browser/extensions/extension_tts_api_platform.h"

using base::DoubleToString;

namespace {
const int kSpeechCheckDelayIntervalMs = 100;
};

class ExtensionTtsPlatformImplChromeOs : public ExtensionTtsPlatformImpl {
 public:
  virtual bool PlatformImplAvailable() {
    return true;
  }

  virtual bool Speak(
      int utterance_id,
      const std::string& utterance,
      const std::string& lang,
      const UtteranceContinuousParameters& params);

  virtual bool StopSpeaking();

  virtual bool SendsEvent(TtsEventType event_type);

  // Get the single instance of this class.
  static ExtensionTtsPlatformImplChromeOs* GetInstance();

 private:
  ExtensionTtsPlatformImplChromeOs()
      : ALLOW_THIS_IN_INITIALIZER_LIST(weak_ptr_factory_(this)) {}
  virtual ~ExtensionTtsPlatformImplChromeOs() {}

  void PollUntilSpeechFinishes(int utterance_id);
  void ContinuePollingIfSpeechIsNotFinished(int utterance_id, bool result);

  void AppendSpeakOption(std::string key,
                         std::string value,
                         std::string* options);

  int utterance_id_;
  int utterance_length_;
  base::WeakPtrFactory<ExtensionTtsPlatformImplChromeOs> weak_ptr_factory_;

  friend struct DefaultSingletonTraits<ExtensionTtsPlatformImplChromeOs>;

  DISALLOW_COPY_AND_ASSIGN(ExtensionTtsPlatformImplChromeOs);
};

// static
ExtensionTtsPlatformImpl* ExtensionTtsPlatformImpl::GetInstance() {
  return ExtensionTtsPlatformImplChromeOs::GetInstance();
}

bool ExtensionTtsPlatformImplChromeOs::Speak(
    int utterance_id,
    const std::string& utterance,
    const std::string& lang,
    const UtteranceContinuousParameters& params) {
  utterance_id_ = utterance_id;
  utterance_length_ = utterance.size();

  std::string options;

  if (!lang.empty()) {
    AppendSpeakOption(
        chromeos::SpeechSynthesizerClient::kSpeechPropertyLocale,
        lang,
        &options);
  }

  if (params.rate >= 0.0) {
    AppendSpeakOption(
        chromeos::SpeechSynthesizerClient::kSpeechPropertyRate,
        DoubleToString(params.rate),
        &options);
  }

  if (params.pitch >= 0.0) {
    // The TTS service allows a range of 0 to 2 for speech pitch.
    AppendSpeakOption(
        chromeos::SpeechSynthesizerClient::kSpeechPropertyPitch,
        DoubleToString(params.pitch),
        &options);
  }

  if (params.volume >= 0.0) {
    // The Chrome OS TTS service allows a range of 0 to 5 for speech volume,
    // but 5 clips, so map to a range of 0...4.
    AppendSpeakOption(
        chromeos::SpeechSynthesizerClient::kSpeechPropertyVolume,
        DoubleToString(params.volume * 4),
        &options);
  }

  chromeos::SpeechSynthesizerClient* speech_synthesizer_client =
      chromeos::DBusThreadManager::Get()->speech_synthesizer_client();

  if (!options.empty())
    speech_synthesizer_client->SetSpeakProperties(options);

  speech_synthesizer_client->Speak(utterance);
  ExtensionTtsController* controller = ExtensionTtsController::GetInstance();
  controller->OnTtsEvent(utterance_id_, TTS_EVENT_START, 0, std::string());
  PollUntilSpeechFinishes(utterance_id_);

  return true;
}

bool ExtensionTtsPlatformImplChromeOs::StopSpeaking() {
  chromeos::DBusThreadManager::Get()->speech_synthesizer_client()->
      StopSpeaking();
  return true;
}

bool ExtensionTtsPlatformImplChromeOs::SendsEvent(TtsEventType event_type) {
  return (event_type == TTS_EVENT_START ||
          event_type == TTS_EVENT_END ||
          event_type == TTS_EVENT_ERROR);
}

void ExtensionTtsPlatformImplChromeOs::PollUntilSpeechFinishes(
    int utterance_id) {
  if (utterance_id != utterance_id_) {
    // This utterance must have been interrupted or cancelled.
    return;
  }
  chromeos::SpeechSynthesizerClient* speech_synthesizer_client =
      chromeos::DBusThreadManager::Get()->speech_synthesizer_client();
  speech_synthesizer_client->IsSpeaking(base::Bind(
      &ExtensionTtsPlatformImplChromeOs::ContinuePollingIfSpeechIsNotFinished,
      weak_ptr_factory_.GetWeakPtr(), utterance_id));
}

void ExtensionTtsPlatformImplChromeOs::ContinuePollingIfSpeechIsNotFinished(
    int utterance_id, bool is_speaking) {
  if (utterance_id != utterance_id_) {
    // This utterance must have been interrupted or cancelled.
    return;
  }
  if (!is_speaking) {
    ExtensionTtsController* controller = ExtensionTtsController::GetInstance();
    controller->OnTtsEvent(
        utterance_id_, TTS_EVENT_END, utterance_length_, std::string());
    return;
  }
  // Continue polling.
  MessageLoop::current()->PostDelayedTask(
      FROM_HERE, base::Bind(
          &ExtensionTtsPlatformImplChromeOs::PollUntilSpeechFinishes,
          weak_ptr_factory_.GetWeakPtr(),
          utterance_id),
      kSpeechCheckDelayIntervalMs);
}

void ExtensionTtsPlatformImplChromeOs::AppendSpeakOption(
    std::string key,
    std::string value,
    std::string* options) {
  *options +=
      key +
      chromeos::SpeechSynthesizerClient::kSpeechPropertyEquals +
      value +
      chromeos::SpeechSynthesizerClient::kSpeechPropertyDelimiter;
}

// static
ExtensionTtsPlatformImplChromeOs*
ExtensionTtsPlatformImplChromeOs::GetInstance() {
  return Singleton<ExtensionTtsPlatformImplChromeOs>::get();
}
