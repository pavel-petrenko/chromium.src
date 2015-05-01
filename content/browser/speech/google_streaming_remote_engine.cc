// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/speech/google_streaming_remote_engine.h"

#include <algorithm>
#include <vector>

#include "base/big_endian.h"
#include "base/bind.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "content/browser/speech/audio_buffer.h"
#include "content/browser/speech/proto/google_streaming_api.pb.h"
#include "content/public/common/speech_recognition_error.h"
#include "content/public/common/speech_recognition_result.h"
#include "google_apis/google_api_keys.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/url_request/http_user_agent_settings.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"

using net::URLFetcher;

namespace content {
namespace {

const char kWebServiceBaseUrl[] =
    "https://www.google.com/speech-api/full-duplex/v1";
const char kDownstreamUrl[] = "/down?";
const char kUpstreamUrl[] = "/up?";
const AudioEncoder::Codec kDefaultAudioCodec = AudioEncoder::CODEC_FLAC;

// This matches the maximum maxAlternatives value supported by the server.
const uint32 kMaxMaxAlternatives = 30;

// TODO(hans): Remove this and other logging when we don't need it anymore.
void DumpResponse(const std::string& response) {
  DVLOG(1) << "------------";
  proto::SpeechRecognitionEvent event;
  if (!event.ParseFromString(response)) {
    DVLOG(1) << "Parse failed!";
    return;
  }
  if (event.has_status())
    DVLOG(1) << "STATUS\t" << event.status();
  for (int i = 0; i < event.result_size(); ++i) {
    DVLOG(1) << "RESULT #" << i << ":";
    const proto::SpeechRecognitionResult& res = event.result(i);
    if (res.has_final())
      DVLOG(1) << "  final:\t" << res.final();
    if (res.has_stability())
      DVLOG(1) << "  STABILITY:\t" << res.stability();
    for (int j = 0; j < res.alternative_size(); ++j) {
      const proto::SpeechRecognitionAlternative& alt =
          res.alternative(j);
      if (alt.has_confidence())
        DVLOG(1) << "    CONFIDENCE:\t" << alt.confidence();
      if (alt.has_transcript())
        DVLOG(1) << "    TRANSCRIPT:\t" << alt.transcript();
    }
  }
}

}  // namespace

const int GoogleStreamingRemoteEngine::kAudioPacketIntervalMs = 100;
const int GoogleStreamingRemoteEngine::kUpstreamUrlFetcherIdForTesting = 0;
const int GoogleStreamingRemoteEngine::kDownstreamUrlFetcherIdForTesting = 1;
const int GoogleStreamingRemoteEngine::kWebserviceStatusNoError = 0;
const int GoogleStreamingRemoteEngine::kWebserviceStatusErrorNoMatch = 5;

GoogleStreamingRemoteEngine::GoogleStreamingRemoteEngine(
    net::URLRequestContextGetter* context)
    : url_context_(context),
      previous_response_length_(0),
      got_last_definitive_result_(false),
      is_dispatching_event_(false),
      use_framed_post_data_(false),
      state_(STATE_IDLE) {}

GoogleStreamingRemoteEngine::~GoogleStreamingRemoteEngine() {}

void GoogleStreamingRemoteEngine::SetConfig(
    const SpeechRecognitionEngineConfig& config) {
  config_ = config;
}

void GoogleStreamingRemoteEngine::StartRecognition() {
  FSMEventArgs event_args(EVENT_START_RECOGNITION);
  DispatchEvent(event_args);
}

void GoogleStreamingRemoteEngine::EndRecognition() {
  FSMEventArgs event_args(EVENT_END_RECOGNITION);
  DispatchEvent(event_args);
}

void GoogleStreamingRemoteEngine::TakeAudioChunk(const AudioChunk& data) {
  FSMEventArgs event_args(EVENT_AUDIO_CHUNK);
  event_args.audio_data = &data;
  DispatchEvent(event_args);
}

void GoogleStreamingRemoteEngine::AudioChunksEnded() {
  FSMEventArgs event_args(EVENT_AUDIO_CHUNKS_ENDED);
  DispatchEvent(event_args);
}

void GoogleStreamingRemoteEngine::OnURLFetchComplete(const URLFetcher* source) {
  const bool kResponseComplete = true;
  DispatchHTTPResponse(source, kResponseComplete);
}

void GoogleStreamingRemoteEngine::OnURLFetchDownloadProgress(
    const URLFetcher* source, int64 current, int64 total) {
  const bool kPartialResponse = false;
  DispatchHTTPResponse(source, kPartialResponse);
}

void GoogleStreamingRemoteEngine::DispatchHTTPResponse(const URLFetcher* source,
                                                       bool end_of_response) {
  DCHECK(CalledOnValidThread());
  DCHECK(source);
  const bool response_is_good = source->GetStatus().is_success() &&
                                source->GetResponseCode() == 200;
  std::string response;
  if (response_is_good)
    source->GetResponseAsString(&response);
  const size_t current_response_length = response.size();

  DVLOG(1) << (source == downstream_fetcher_.get() ? "Downstream" : "Upstream")
           << "HTTP, code: " << source->GetResponseCode()
           << "      length: " << current_response_length
           << "      eor: " << end_of_response;

  // URLFetcher provides always the entire response buffer, but we are only
  // interested in the fresh data introduced by the last chunk. Therefore, we
  // drop the previous content we have already processed.
  if (current_response_length != 0) {
    DCHECK_GE(current_response_length, previous_response_length_);
    response.erase(0, previous_response_length_);
    previous_response_length_ = current_response_length;
  }

  if (!response_is_good && source == downstream_fetcher_.get()) {
    DVLOG(1) << "Downstream error " << source->GetResponseCode();
    FSMEventArgs event_args(EVENT_DOWNSTREAM_ERROR);
    DispatchEvent(event_args);
    return;
  }
  if (!response_is_good && source == upstream_fetcher_.get()) {
    DVLOG(1) << "Upstream error " << source->GetResponseCode()
             << " EOR " << end_of_response;
    FSMEventArgs event_args(EVENT_UPSTREAM_ERROR);
    DispatchEvent(event_args);
    return;
  }

  // Ignore incoming data on the upstream connection.
  if (source == upstream_fetcher_.get())
    return;

  DCHECK(response_is_good && source == downstream_fetcher_.get());

  // The downstream response is organized in chunks, whose size is determined
  // by a 4 bytes prefix, transparently handled by the ChunkedByteBuffer class.
  // Such chunks are sent by the speech recognition webservice over the HTTP
  // downstream channel using HTTP chunked transfer (unrelated to our chunks).
  // This function is called every time an HTTP chunk is received by the
  // url fetcher. However there isn't any particular matching beween our
  // protocol chunks and HTTP chunks, in the sense that a single HTTP chunk can
  // contain a portion of one chunk or even more chunks together.
  chunked_byte_buffer_.Append(response);

  // A single HTTP chunk can contain more than one data chunk, thus the while.
  while (chunked_byte_buffer_.HasChunks()) {
    FSMEventArgs event_args(EVENT_DOWNSTREAM_RESPONSE);
    event_args.response = chunked_byte_buffer_.PopChunk();
    DCHECK(event_args.response.get());
    DumpResponse(std::string(event_args.response->begin(),
                             event_args.response->end()));
    DispatchEvent(event_args);
  }
  if (end_of_response) {
    FSMEventArgs event_args(EVENT_DOWNSTREAM_CLOSED);
    DispatchEvent(event_args);
  }
}

bool GoogleStreamingRemoteEngine::IsRecognitionPending() const {
  DCHECK(CalledOnValidThread());
  return state_ != STATE_IDLE;
}

int GoogleStreamingRemoteEngine::GetDesiredAudioChunkDurationMs() const {
  return kAudioPacketIntervalMs;
}

// -----------------------  Core FSM implementation ---------------------------

void GoogleStreamingRemoteEngine::DispatchEvent(
    const FSMEventArgs& event_args) {
  DCHECK(CalledOnValidThread());
  DCHECK_LE(event_args.event, EVENT_MAX_VALUE);
  DCHECK_LE(state_, STATE_MAX_VALUE);

  // Event dispatching must be sequential, otherwise it will break all the rules
  // and the assumptions of the finite state automata model.
  DCHECK(!is_dispatching_event_);
  is_dispatching_event_ = true;

  state_ = ExecuteTransitionAndGetNextState(event_args);

  is_dispatching_event_ = false;
}

GoogleStreamingRemoteEngine::FSMState
GoogleStreamingRemoteEngine::ExecuteTransitionAndGetNextState(
    const FSMEventArgs& event_args) {
  const FSMEvent event = event_args.event;
  switch (state_) {
    case STATE_IDLE:
      switch (event) {
        case EVENT_START_RECOGNITION:
          return ConnectBothStreams(event_args);
        case EVENT_END_RECOGNITION:
        // Note AUDIO_CHUNK and AUDIO_END events can remain enqueued in case of
        // abort, so we just silently drop them here.
        case EVENT_AUDIO_CHUNK:
        case EVENT_AUDIO_CHUNKS_ENDED:
        // DOWNSTREAM_CLOSED can be received if we end up here due to an error.
        case EVENT_DOWNSTREAM_CLOSED:
          return DoNothing(event_args);
        case EVENT_UPSTREAM_ERROR:
        case EVENT_DOWNSTREAM_ERROR:
        case EVENT_DOWNSTREAM_RESPONSE:
          return NotFeasible(event_args);
      }
      break;
    case STATE_BOTH_STREAMS_CONNECTED:
      switch (event) {
        case EVENT_AUDIO_CHUNK:
          return TransmitAudioUpstream(event_args);
        case EVENT_DOWNSTREAM_RESPONSE:
          return ProcessDownstreamResponse(event_args);
        case EVENT_AUDIO_CHUNKS_ENDED:
          return CloseUpstreamAndWaitForResults(event_args);
        case EVENT_END_RECOGNITION:
          return AbortSilently(event_args);
        case EVENT_UPSTREAM_ERROR:
        case EVENT_DOWNSTREAM_ERROR:
        case EVENT_DOWNSTREAM_CLOSED:
          return AbortWithError(event_args);
        case EVENT_START_RECOGNITION:
          return NotFeasible(event_args);
      }
      break;
    case STATE_WAITING_DOWNSTREAM_RESULTS:
      switch (event) {
        case EVENT_DOWNSTREAM_RESPONSE:
          return ProcessDownstreamResponse(event_args);
        case EVENT_DOWNSTREAM_CLOSED:
          return RaiseNoMatchErrorIfGotNoResults(event_args);
        case EVENT_END_RECOGNITION:
          return AbortSilently(event_args);
        case EVENT_UPSTREAM_ERROR:
        case EVENT_DOWNSTREAM_ERROR:
          return AbortWithError(event_args);
        case EVENT_START_RECOGNITION:
        case EVENT_AUDIO_CHUNK:
        case EVENT_AUDIO_CHUNKS_ENDED:
          return NotFeasible(event_args);
      }
      break;
  }
  return NotFeasible(event_args);
}

// ----------- Contract for all the FSM evolution functions below -------------
//  - Are guaranteed to be executed in the same thread (IO, except for tests);
//  - Are guaranteed to be not reentrant (themselves and each other);
//  - event_args members are guaranteed to be stable during the call;

GoogleStreamingRemoteEngine::FSMState
GoogleStreamingRemoteEngine::ConnectBothStreams(const FSMEventArgs&) {
  DCHECK(!upstream_fetcher_.get());
  DCHECK(!downstream_fetcher_.get());

  encoder_.reset(AudioEncoder::Create(kDefaultAudioCodec,
                                      config_.audio_sample_rate,
                                      config_.audio_num_bits_per_sample));
  DCHECK(encoder_.get());
  const std::string request_key = GenerateRequestKey();

  // Only use the framed post data format when a preamble needs to be logged.
  use_framed_post_data_ = (config_.preamble &&
                           !config_.preamble->sample_data.empty() &&
                           !config_.auth_token.empty() &&
                           !config_.auth_scope.empty());
  if (use_framed_post_data_) {
    preamble_encoder_.reset(AudioEncoder::Create(
        kDefaultAudioCodec,
        config_.preamble->sample_rate,
        config_.preamble->sample_depth * 8));
  }

  // Setup downstream fetcher.
  std::vector<std::string> downstream_args;
  downstream_args.push_back(
      "key=" + net::EscapeQueryParamValue(google_apis::GetAPIKey(), true));
  downstream_args.push_back("pair=" + request_key);
  downstream_args.push_back("output=pb");
  GURL downstream_url(std::string(kWebServiceBaseUrl) +
                      std::string(kDownstreamUrl) +
                      JoinString(downstream_args, '&'));

  downstream_fetcher_ = URLFetcher::Create(
      kDownstreamUrlFetcherIdForTesting, downstream_url, URLFetcher::GET, this);
  downstream_fetcher_->SetRequestContext(url_context_.get());
  downstream_fetcher_->SetLoadFlags(net::LOAD_DO_NOT_SAVE_COOKIES |
                                    net::LOAD_DO_NOT_SEND_COOKIES |
                                    net::LOAD_DO_NOT_SEND_AUTH_DATA);
  downstream_fetcher_->Start();

  // Setup upstream fetcher.
  // TODO(hans): Support for user-selected grammars.
  std::vector<std::string> upstream_args;
  upstream_args.push_back("key=" +
      net::EscapeQueryParamValue(google_apis::GetAPIKey(), true));
  upstream_args.push_back("pair=" + request_key);
  upstream_args.push_back("output=pb");
  upstream_args.push_back(
      "lang=" + net::EscapeQueryParamValue(GetAcceptedLanguages(), true));
  upstream_args.push_back(
      config_.filter_profanities ? "pFilter=2" : "pFilter=0");
  if (config_.max_hypotheses > 0U) {
    int max_alternatives = std::min(kMaxMaxAlternatives,
                                    config_.max_hypotheses);
    upstream_args.push_back("maxAlternatives=" +
                            base::UintToString(max_alternatives));
  }
  upstream_args.push_back("client=chromium");
  if (!config_.hardware_info.empty()) {
    upstream_args.push_back(
        "xhw=" + net::EscapeQueryParamValue(config_.hardware_info, true));
  }
  if (config_.continuous)
    upstream_args.push_back("continuous");
  if (config_.interim_results)
    upstream_args.push_back("interim");
  if (!config_.auth_token.empty() && !config_.auth_scope.empty()) {
    upstream_args.push_back(
        "authScope=" + net::EscapeQueryParamValue(config_.auth_scope, true));
    upstream_args.push_back(
        "authToken=" + net::EscapeQueryParamValue(config_.auth_token, true));
  }
  if (use_framed_post_data_) {
    std::string audio_format;
    if (preamble_encoder_)
      audio_format = preamble_encoder_->mime_type() + ",";
    audio_format += encoder_->mime_type();
    upstream_args.push_back(
        "audioFormat=" + net::EscapeQueryParamValue(audio_format, true));
  }
  GURL upstream_url(std::string(kWebServiceBaseUrl) +
                    std::string(kUpstreamUrl) +
                    JoinString(upstream_args, '&'));

  upstream_fetcher_ = URLFetcher::Create(kUpstreamUrlFetcherIdForTesting,
                                         upstream_url, URLFetcher::POST, this);
  if (use_framed_post_data_)
    upstream_fetcher_->SetChunkedUpload("application/octet-stream");
  else
    upstream_fetcher_->SetChunkedUpload(encoder_->mime_type());
  upstream_fetcher_->SetRequestContext(url_context_.get());
  upstream_fetcher_->SetReferrer(config_.origin_url);
  upstream_fetcher_->SetLoadFlags(net::LOAD_DO_NOT_SAVE_COOKIES |
                                  net::LOAD_DO_NOT_SEND_COOKIES |
                                  net::LOAD_DO_NOT_SEND_AUTH_DATA);
  upstream_fetcher_->Start();
  previous_response_length_ = 0;

  if (preamble_encoder_) {
    // Encode and send preamble right away.
    scoped_refptr<AudioChunk> chunk = new AudioChunk(
        reinterpret_cast<const uint8*>(config_.preamble->sample_data.data()),
        config_.preamble->sample_data.size(),
        config_.preamble->sample_depth);
    preamble_encoder_->Encode(*chunk);
    preamble_encoder_->Flush();
    scoped_refptr<AudioChunk> encoded_data(
        preamble_encoder_->GetEncodedDataAndClear());
    UploadAudioChunk(encoded_data->AsString(), FRAME_PREAMBLE_AUDIO, false);
  }
  return STATE_BOTH_STREAMS_CONNECTED;
}

GoogleStreamingRemoteEngine::FSMState
GoogleStreamingRemoteEngine::TransmitAudioUpstream(
    const FSMEventArgs& event_args) {
  DCHECK(upstream_fetcher_.get());
  DCHECK(event_args.audio_data.get());
  const AudioChunk& audio = *(event_args.audio_data.get());

  DCHECK_EQ(audio.bytes_per_sample(), config_.audio_num_bits_per_sample / 8);
  encoder_->Encode(audio);
  scoped_refptr<AudioChunk> encoded_data(encoder_->GetEncodedDataAndClear());
  UploadAudioChunk(encoded_data->AsString(), FRAME_RECOGNITION_AUDIO, false);
  return state_;
}

GoogleStreamingRemoteEngine::FSMState
GoogleStreamingRemoteEngine::ProcessDownstreamResponse(
    const FSMEventArgs& event_args) {
  DCHECK(event_args.response.get());

  proto::SpeechRecognitionEvent ws_event;
  if (!ws_event.ParseFromString(std::string(event_args.response->begin(),
                                            event_args.response->end())))
    return AbortWithError(event_args);

  // An empty (default) event is used to notify us that the upstream has
  // been connected. Ignore.
  if (!ws_event.result_size() && (!ws_event.has_status() ||
      ws_event.status() == proto::SpeechRecognitionEvent::STATUS_SUCCESS)) {
    DVLOG(1) << "Received empty response";
    return state_;
  }

  if (ws_event.has_status()) {
    switch (ws_event.status()) {
      case proto::SpeechRecognitionEvent::STATUS_SUCCESS:
        break;
      case proto::SpeechRecognitionEvent::STATUS_NO_SPEECH:
        return Abort(SPEECH_RECOGNITION_ERROR_NO_SPEECH);
      case proto::SpeechRecognitionEvent::STATUS_ABORTED:
        return Abort(SPEECH_RECOGNITION_ERROR_ABORTED);
      case proto::SpeechRecognitionEvent::STATUS_AUDIO_CAPTURE:
        return Abort(SPEECH_RECOGNITION_ERROR_AUDIO_CAPTURE);
      case proto::SpeechRecognitionEvent::STATUS_NETWORK:
        return Abort(SPEECH_RECOGNITION_ERROR_NETWORK);
      case proto::SpeechRecognitionEvent::STATUS_NOT_ALLOWED:
        return Abort(SPEECH_RECOGNITION_ERROR_NOT_ALLOWED);
      case proto::SpeechRecognitionEvent::STATUS_SERVICE_NOT_ALLOWED:
        return Abort(SPEECH_RECOGNITION_ERROR_SERVICE_NOT_ALLOWED);
      case proto::SpeechRecognitionEvent::STATUS_BAD_GRAMMAR:
        return Abort(SPEECH_RECOGNITION_ERROR_BAD_GRAMMAR);
      case proto::SpeechRecognitionEvent::STATUS_LANGUAGE_NOT_SUPPORTED:
        return Abort(SPEECH_RECOGNITION_ERROR_LANGUAGE_NOT_SUPPORTED);
    }
  }

  SpeechRecognitionResults results;
  for (int i = 0; i < ws_event.result_size(); ++i) {
    const proto::SpeechRecognitionResult& ws_result = ws_event.result(i);
    results.push_back(SpeechRecognitionResult());
    SpeechRecognitionResult& result = results.back();
    result.is_provisional = !(ws_result.has_final() && ws_result.final());

    if (!result.is_provisional)
      got_last_definitive_result_ = true;

    for (int j = 0; j < ws_result.alternative_size(); ++j) {
      const proto::SpeechRecognitionAlternative& ws_alternative =
          ws_result.alternative(j);
      SpeechRecognitionHypothesis hypothesis;
      if (ws_alternative.has_confidence())
        hypothesis.confidence = ws_alternative.confidence();
      else if (ws_result.has_stability())
        hypothesis.confidence = ws_result.stability();
      DCHECK(ws_alternative.has_transcript());
      // TODO(hans): Perhaps the transcript should be required in the proto?
      if (ws_alternative.has_transcript())
        hypothesis.utterance = base::UTF8ToUTF16(ws_alternative.transcript());

      result.hypotheses.push_back(hypothesis);
    }
  }

  delegate()->OnSpeechRecognitionEngineResults(results);

  return state_;
}

GoogleStreamingRemoteEngine::FSMState
GoogleStreamingRemoteEngine::RaiseNoMatchErrorIfGotNoResults(
    const FSMEventArgs& event_args) {
  if (!got_last_definitive_result_) {
    // Provide an empty result to notify that recognition is ended with no
    // errors, yet neither any further results.
    delegate()->OnSpeechRecognitionEngineResults(SpeechRecognitionResults());
  }
  return AbortSilently(event_args);
}

GoogleStreamingRemoteEngine::FSMState
GoogleStreamingRemoteEngine::CloseUpstreamAndWaitForResults(
    const FSMEventArgs&) {
  DCHECK(upstream_fetcher_.get());
  DCHECK(encoder_.get());

  DVLOG(1) <<  "Closing upstream.";

  // The encoder requires a non-empty final buffer. So we encode a packet
  // of silence in case encoder had no data already.
  size_t sample_count =
      config_.audio_sample_rate * kAudioPacketIntervalMs / 1000;
  scoped_refptr<AudioChunk> dummy_chunk = new AudioChunk(
      sample_count * sizeof(int16), encoder_->bits_per_sample() / 8);
  encoder_->Encode(*dummy_chunk.get());
  encoder_->Flush();
  scoped_refptr<AudioChunk> encoded_dummy_data =
      encoder_->GetEncodedDataAndClear();
  DCHECK(!encoded_dummy_data->IsEmpty());
  encoder_.reset();

  UploadAudioChunk(encoded_dummy_data->AsString(),
                   FRAME_RECOGNITION_AUDIO,
                   true);
  got_last_definitive_result_ = false;
  return STATE_WAITING_DOWNSTREAM_RESULTS;
}

GoogleStreamingRemoteEngine::FSMState
GoogleStreamingRemoteEngine::CloseDownstream(const FSMEventArgs&) {
  DCHECK(!upstream_fetcher_.get());
  DCHECK(downstream_fetcher_.get());

  DVLOG(1) <<  "Closing downstream.";
  downstream_fetcher_.reset();
  return STATE_IDLE;
}

GoogleStreamingRemoteEngine::FSMState
GoogleStreamingRemoteEngine::AbortSilently(const FSMEventArgs&) {
  return Abort(SPEECH_RECOGNITION_ERROR_NONE);
}

GoogleStreamingRemoteEngine::FSMState
GoogleStreamingRemoteEngine::AbortWithError(const FSMEventArgs&) {
  return Abort(SPEECH_RECOGNITION_ERROR_NETWORK);
}

GoogleStreamingRemoteEngine::FSMState GoogleStreamingRemoteEngine::Abort(
    SpeechRecognitionErrorCode error_code) {
  DVLOG(1) << "Aborting with error " << error_code;

  if (error_code != SPEECH_RECOGNITION_ERROR_NONE) {
    delegate()->OnSpeechRecognitionEngineError(
        SpeechRecognitionError(error_code));
  }
  downstream_fetcher_.reset();
  upstream_fetcher_.reset();
  encoder_.reset();
  return STATE_IDLE;
}

GoogleStreamingRemoteEngine::FSMState
GoogleStreamingRemoteEngine::DoNothing(const FSMEventArgs&) {
  return state_;
}

GoogleStreamingRemoteEngine::FSMState
GoogleStreamingRemoteEngine::NotFeasible(const FSMEventArgs& event_args) {
  NOTREACHED() << "Unfeasible event " << event_args.event
               << " in state " << state_;
  return state_;
}

std::string GoogleStreamingRemoteEngine::GetAcceptedLanguages() const {
  std::string langs = config_.language;
  if (langs.empty() && url_context_.get()) {
    // If no language is provided then we use the first from the accepted
    // language list. If this list is empty then it defaults to "en-US".
    // Example of the contents of this list: "es,en-GB;q=0.8", ""
    net::URLRequestContext* request_context =
        url_context_->GetURLRequestContext();
    DCHECK(request_context);
    // TODO(pauljensen): GoogleStreamingRemoteEngine should be constructed with
    // a reference to the HttpUserAgentSettings rather than accessing the
    // accept language through the URLRequestContext.
    if (request_context->http_user_agent_settings()) {
      std::string accepted_language_list =
          request_context->http_user_agent_settings()->GetAcceptLanguage();
      size_t separator = accepted_language_list.find_first_of(",;");
      if (separator != std::string::npos)
        langs = accepted_language_list.substr(0, separator);
    }
  }
  if (langs.empty())
    langs = "en-US";
  return langs;
}

// TODO(primiano): Is there any utility in the codebase that already does this?
std::string GoogleStreamingRemoteEngine::GenerateRequestKey() const {
  const int64 kKeepLowBytes = 0x00000000FFFFFFFFLL;
  const int64 kKeepHighBytes = 0xFFFFFFFF00000000LL;

  // Just keep the least significant bits of timestamp, in order to reduce
  // probability of collisions.
  int64 key = (base::Time::Now().ToInternalValue() & kKeepLowBytes) |
              (base::RandUint64() & kKeepHighBytes);
  return base::HexEncode(reinterpret_cast<void*>(&key), sizeof(key));
}

void GoogleStreamingRemoteEngine::UploadAudioChunk(const std::string& data,
                                                   FrameType type,
                                                   bool is_final) {
  if (use_framed_post_data_) {
    std::string frame(data.size() + 8, 0);
    base::WriteBigEndian(&frame[0], static_cast<uint32_t>(data.size()));
    base::WriteBigEndian(&frame[4], static_cast<uint32_t>(type));
    frame.replace(8, data.size(), data);
    upstream_fetcher_->AppendChunkToUpload(frame, is_final);
  } else {
    upstream_fetcher_->AppendChunkToUpload(data, is_final);
  }
}

GoogleStreamingRemoteEngine::FSMEventArgs::FSMEventArgs(FSMEvent event_value)
    : event(event_value) {
}

GoogleStreamingRemoteEngine::FSMEventArgs::~FSMEventArgs() {
}

}  // namespace content
