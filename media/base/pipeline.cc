// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/pipeline.h"

#include <algorithm>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/compiler_specific.h"
#include "base/metrics/histogram.h"
#include "base/message_loop.h"
#include "base/stl_util.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "base/synchronization/condition_variable.h"
#include "media/base/audio_decoder.h"
#include "media/base/audio_renderer.h"
#include "media/base/buffers.h"
#include "media/base/clock.h"
#include "media/base/filter_collection.h"
#include "media/base/media_log.h"
#include "media/base/video_decoder.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_renderer.h"

using base::TimeDelta;

namespace media {

PipelineStatusNotification::PipelineStatusNotification()
    : cv_(&lock_), status_(PIPELINE_OK), notified_(false) {
}

PipelineStatusNotification::~PipelineStatusNotification() {
  DCHECK(notified_);
}

PipelineStatusCB PipelineStatusNotification::Callback() {
  return base::Bind(&PipelineStatusNotification::Notify,
                    base::Unretained(this));
}

void PipelineStatusNotification::Notify(media::PipelineStatus status) {
  base::AutoLock auto_lock(lock_);
  DCHECK(!notified_);
  notified_ = true;
  status_ = status;
  cv_.Signal();
}

void PipelineStatusNotification::Wait() {
  base::AutoLock auto_lock(lock_);
  while (!notified_)
    cv_.Wait();
}

media::PipelineStatus PipelineStatusNotification::status() {
  base::AutoLock auto_lock(lock_);
  DCHECK(notified_);
  return status_;
}

struct Pipeline::PipelineInitState {
  scoped_refptr<AudioDecoder> audio_decoder;
};

Pipeline::Pipeline(const scoped_refptr<base::MessageLoopProxy>& message_loop,
                   MediaLog* media_log)
    : message_loop_(message_loop),
      media_log_(media_log),
      running_(false),
      seek_pending_(false),
      tearing_down_(false),
      playback_rate_change_pending_(false),
      did_loading_progress_(false),
      total_bytes_(0),
      natural_size_(0, 0),
      volume_(1.0f),
      playback_rate_(0.0f),
      pending_playback_rate_(0.0f),
      clock_(new Clock(&base::Time::Now)),
      waiting_for_clock_update_(false),
      status_(PIPELINE_OK),
      has_audio_(false),
      has_video_(false),
      state_(kCreated),
      seek_timestamp_(kNoTimestamp()),
      audio_ended_(false),
      video_ended_(false),
      audio_disabled_(false),
      creation_time_(base::Time::Now()) {
  media_log_->AddEvent(media_log_->CreatePipelineStateChangedEvent(kCreated));
  media_log_->AddEvent(
      media_log_->CreateEvent(MediaLogEvent::PIPELINE_CREATED));
}

Pipeline::~Pipeline() {
  base::AutoLock auto_lock(lock_);
  DCHECK(!running_) << "Stop() must complete before destroying object";
  DCHECK(stop_cb_.is_null());
  DCHECK(!seek_pending_);

  media_log_->AddEvent(
      media_log_->CreateEvent(MediaLogEvent::PIPELINE_DESTROYED));
}

void Pipeline::Start(scoped_ptr<FilterCollection> collection,
                     const PipelineStatusCB& ended_cb,
                     const PipelineStatusCB& error_cb,
                     const PipelineStatusCB& seek_cb,
                     const BufferingStateCB& buffering_state_cb) {
  base::AutoLock auto_lock(lock_);
  CHECK(!running_) << "Media pipeline is already running";
  DCHECK(!buffering_state_cb.is_null());

  running_ = true;
  message_loop_->PostTask(FROM_HERE, base::Bind(
      &Pipeline::StartTask, this, base::Passed(&collection),
      ended_cb, error_cb, seek_cb, buffering_state_cb));
}

void Pipeline::Stop(const base::Closure& stop_cb) {
  base::AutoLock auto_lock(lock_);
  message_loop_->PostTask(FROM_HERE, base::Bind(
      &Pipeline::StopTask, this, stop_cb));
}

void Pipeline::Seek(TimeDelta time, const PipelineStatusCB& seek_cb) {
  base::AutoLock auto_lock(lock_);
  if (!running_) {
    NOTREACHED() << "Media pipeline isn't running";
    return;
  }

  message_loop_->PostTask(FROM_HERE, base::Bind(
      &Pipeline::SeekTask, this, time, seek_cb));
}

bool Pipeline::IsRunning() const {
  base::AutoLock auto_lock(lock_);
  return running_;
}

bool Pipeline::HasAudio() const {
  base::AutoLock auto_lock(lock_);
  return has_audio_;
}

bool Pipeline::HasVideo() const {
  base::AutoLock auto_lock(lock_);
  return has_video_;
}

float Pipeline::GetPlaybackRate() const {
  base::AutoLock auto_lock(lock_);
  return playback_rate_;
}

void Pipeline::SetPlaybackRate(float playback_rate) {
  if (playback_rate < 0.0f)
    return;

  base::AutoLock auto_lock(lock_);
  playback_rate_ = playback_rate;
  if (running_) {
    message_loop_->PostTask(FROM_HERE, base::Bind(
        &Pipeline::PlaybackRateChangedTask, this, playback_rate));
  }
}

float Pipeline::GetVolume() const {
  base::AutoLock auto_lock(lock_);
  return volume_;
}

void Pipeline::SetVolume(float volume) {
  if (volume < 0.0f || volume > 1.0f)
    return;

  base::AutoLock auto_lock(lock_);
  volume_ = volume;
  if (running_) {
    message_loop_->PostTask(FROM_HERE, base::Bind(
        &Pipeline::VolumeChangedTask, this, volume));
  }
}

TimeDelta Pipeline::GetMediaTime() const {
  base::AutoLock auto_lock(lock_);
  return clock_->Elapsed();
}

Ranges<TimeDelta> Pipeline::GetBufferedTimeRanges() {
  base::AutoLock auto_lock(lock_);
  Ranges<TimeDelta> time_ranges;
  for (size_t i = 0; i < buffered_time_ranges_.size(); ++i) {
    time_ranges.Add(buffered_time_ranges_.start(i),
                    buffered_time_ranges_.end(i));
  }
  if (clock_->Duration() == TimeDelta() || total_bytes_ == 0)
    return time_ranges;
  for (size_t i = 0; i < buffered_byte_ranges_.size(); ++i) {
    TimeDelta start = TimeForByteOffset_Locked(buffered_byte_ranges_.start(i));
    TimeDelta end = TimeForByteOffset_Locked(buffered_byte_ranges_.end(i));
    // Cap approximated buffered time at the length of the video.
    end = std::min(end, clock_->Duration());
    time_ranges.Add(start, end);
  }

  return time_ranges;
}

TimeDelta Pipeline::GetMediaDuration() const {
  base::AutoLock auto_lock(lock_);
  return clock_->Duration();
}

int64 Pipeline::GetTotalBytes() const {
  base::AutoLock auto_lock(lock_);
  return total_bytes_;
}

void Pipeline::GetNaturalVideoSize(gfx::Size* out_size) const {
  CHECK(out_size);
  base::AutoLock auto_lock(lock_);
  *out_size = natural_size_;
}

bool Pipeline::DidLoadingProgress() const {
  base::AutoLock auto_lock(lock_);
  bool ret = did_loading_progress_;
  did_loading_progress_ = false;
  return ret;
}

PipelineStatistics Pipeline::GetStatistics() const {
  base::AutoLock auto_lock(lock_);
  return statistics_;
}

void Pipeline::SetClockForTesting(Clock* clock) {
  clock_.reset(clock);
}

void Pipeline::SetErrorForTesting(PipelineStatus status) {
  SetError(status);
}

void Pipeline::SetState(State next_state) {
  if (state_ != kStarted && next_state == kStarted &&
      !creation_time_.is_null()) {
    UMA_HISTOGRAM_TIMES(
        "Media.TimeToPipelineStarted", base::Time::Now() - creation_time_);
    creation_time_ = base::Time();
  }
  state_ = next_state;
  media_log_->AddEvent(media_log_->CreatePipelineStateChangedEvent(next_state));
}

bool Pipeline::IsPipelineOk() {
  base::AutoLock auto_lock(lock_);
  return status_ == PIPELINE_OK;
}

bool Pipeline::IsPipelineSeeking() {
  DCHECK(message_loop_->BelongsToCurrentThread());
  if (!seek_pending_)
    return false;
  DCHECK(kSeeking == state_ || kPausing == state_ ||
         kFlushing == state_ || kStarting == state_)
      << "Current state : " << state_;
  return true;
}

void Pipeline::ReportStatus(const PipelineStatusCB& cb, PipelineStatus status) {
  DCHECK(message_loop_->BelongsToCurrentThread());
  if (cb.is_null())
    return;
  cb.Run(status);
  // Prevent double-reporting of errors to clients.
  if (status != PIPELINE_OK)
    error_cb_.Reset();
}

void Pipeline::FinishSeek() {
  DCHECK(message_loop_->BelongsToCurrentThread());
  seek_timestamp_ = kNoTimestamp();
  seek_pending_ = false;

  // Execute the seek callback, if present.  Note that this might be the
  // initial callback passed into Start().
  ReportStatus(seek_cb_, status_);
  seek_cb_.Reset();
}

// static
bool Pipeline::TransientState(State state) {
  return state == kPausing ||
         state == kFlushing ||
         state == kSeeking ||
         state == kStarting ||
         state == kStopping;
}

// static
Pipeline::State Pipeline::FindNextState(State current) {
  // TODO(scherkus): refactor InitializeTask() to make use of this function.
  if (current == kPausing) {
    return kFlushing;
  } else if (current == kFlushing) {
    // We will always honor Seek() before Stop(). This is based on the
    // assumption that we never accept Seek() after Stop().
    DCHECK(IsPipelineSeeking() ||
           !stop_cb_.is_null() ||
           tearing_down_);
    return IsPipelineSeeking() ? kSeeking : kStopping;
  } else if (current == kSeeking) {
    return kStarting;
  } else if (current == kStarting) {
    return kStarted;
  } else if (current == kStopping) {
    return kStopped;
  } else {
    return current;
  }
}

void Pipeline::OnDemuxerError(PipelineStatus error) {
  SetError(error);
}

void Pipeline::SetError(PipelineStatus error) {
  DCHECK(IsRunning());
  DCHECK_NE(PIPELINE_OK, error);
  VLOG(1) << "Media pipeline error: " << error;

  message_loop_->PostTask(FROM_HERE, base::Bind(
      &Pipeline::ErrorChangedTask, this, error));

  media_log_->AddEvent(media_log_->CreatePipelineErrorEvent(error));
}

void Pipeline::OnAudioDisabled() {
  DCHECK(IsRunning());
  message_loop_->PostTask(FROM_HERE, base::Bind(
      &Pipeline::AudioDisabledTask, this));
  media_log_->AddEvent(
      media_log_->CreateEvent(MediaLogEvent::AUDIO_RENDERER_DISABLED));
}

void Pipeline::OnAudioTimeUpdate(TimeDelta time, TimeDelta max_time) {
  DCHECK_LE(time.InMicroseconds(), max_time.InMicroseconds());
  DCHECK(IsRunning());
  base::AutoLock auto_lock(lock_);

  if (!has_audio_)
    return;
  if (waiting_for_clock_update_ && time < clock_->Elapsed())
    return;

  if (state_ == kSeeking)
    return;

  clock_->SetTime(time, max_time);
  StartClockIfWaitingForTimeUpdate_Locked();
}

void Pipeline::OnVideoTimeUpdate(TimeDelta max_time) {
  DCHECK(IsRunning());
  base::AutoLock auto_lock(lock_);

  if (has_audio_)
    return;

  if (state_ == kSeeking)
    return;

  DCHECK(!waiting_for_clock_update_);
  clock_->SetMaxTime(max_time);
}

void Pipeline::SetDuration(TimeDelta duration) {
  DCHECK(IsRunning());
  media_log_->AddEvent(
      media_log_->CreateTimeEvent(
          MediaLogEvent::DURATION_SET, "duration", duration));
  UMA_HISTOGRAM_LONG_TIMES("Media.Duration", duration);

  base::AutoLock auto_lock(lock_);
  clock_->SetDuration(duration);
}

void Pipeline::SetTotalBytes(int64 total_bytes) {
  DCHECK(IsRunning());
  media_log_->AddEvent(
      media_log_->CreateStringEvent(
          MediaLogEvent::TOTAL_BYTES_SET, "total_bytes",
          base::Int64ToString(total_bytes)));
  int64 total_mbytes = total_bytes >> 20;
  if (total_mbytes > kint32max)
    total_mbytes = kint32max;
  UMA_HISTOGRAM_CUSTOM_COUNTS(
      "Media.TotalMBytes", static_cast<int32>(total_mbytes), 1, kint32max, 50);

  base::AutoLock auto_lock(lock_);
  total_bytes_ = total_bytes;
}

TimeDelta Pipeline::TimeForByteOffset_Locked(int64 byte_offset) const {
  lock_.AssertAcquired();
  TimeDelta time_offset = byte_offset * clock_->Duration() / total_bytes_;
  // Since the byte->time calculation is approximate, fudge the beginning &
  // ending areas to look better.
  TimeDelta epsilon = clock_->Duration() / 100;
  if (time_offset < epsilon)
    return TimeDelta();
  if (time_offset + epsilon > clock_->Duration())
    return clock_->Duration();
  return time_offset;
}

void Pipeline::DoPause(const PipelineStatusCB& done_cb) {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK(!pending_callbacks_.get());
  SerialRunner::Queue bound_fns;

  if (audio_renderer_)
    bound_fns.Push(base::Bind(&AudioRenderer::Pause, audio_renderer_));

  if (video_renderer_)
    bound_fns.Push(base::Bind(&VideoRenderer::Pause, video_renderer_));

  pending_callbacks_ = SerialRunner::Run(bound_fns, done_cb);
}

void Pipeline::DoFlush(const PipelineStatusCB& done_cb) {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK(!pending_callbacks_.get());
  SerialRunner::Queue bound_fns;

  if (audio_renderer_)
    bound_fns.Push(base::Bind(&AudioRenderer::Flush, audio_renderer_));

  if (video_renderer_)
    bound_fns.Push(base::Bind(&VideoRenderer::Flush, video_renderer_));

  pending_callbacks_ = SerialRunner::Run(bound_fns, done_cb);
}

void Pipeline::DoPlay(const PipelineStatusCB& done_cb) {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK(!pending_callbacks_.get());
  SerialRunner::Queue bound_fns;

  if (audio_renderer_)
    bound_fns.Push(base::Bind(&AudioRenderer::Play, audio_renderer_));

  if (video_renderer_)
    bound_fns.Push(base::Bind(&VideoRenderer::Play, video_renderer_));

  pending_callbacks_ = SerialRunner::Run(bound_fns, done_cb);
}

void Pipeline::DoStop(const PipelineStatusCB& done_cb) {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK(!pending_callbacks_.get());
  SerialRunner::Queue bound_fns;

  if (demuxer_)
    bound_fns.Push(base::Bind(&Demuxer::Stop, demuxer_));

  if (audio_renderer_)
    bound_fns.Push(base::Bind(&AudioRenderer::Stop, audio_renderer_));

  if (video_renderer_)
    bound_fns.Push(base::Bind(&VideoRenderer::Stop, video_renderer_));

  pending_callbacks_ = SerialRunner::Run(bound_fns, done_cb);
}

void Pipeline::AddBufferedByteRange(int64 start, int64 end) {
  DCHECK(IsRunning());
  base::AutoLock auto_lock(lock_);
  buffered_byte_ranges_.Add(start, end);
  did_loading_progress_ = true;
}

void Pipeline::AddBufferedTimeRange(base::TimeDelta start,
                                    base::TimeDelta end) {
  DCHECK(IsRunning());
  base::AutoLock auto_lock(lock_);
  buffered_time_ranges_.Add(start, end);
  did_loading_progress_ = true;
}

void Pipeline::OnNaturalVideoSizeChanged(const gfx::Size& size) {
  DCHECK(IsRunning());
  media_log_->AddEvent(media_log_->CreateVideoSizeSetEvent(
      size.width(), size.height()));

  base::AutoLock auto_lock(lock_);
  natural_size_ = size;
}

void Pipeline::OnAudioRendererEnded() {
  // Force post to process ended messages after current execution frame.
  message_loop_->PostTask(FROM_HERE, base::Bind(
      &Pipeline::DoAudioRendererEnded, this));
  media_log_->AddEvent(media_log_->CreateEvent(MediaLogEvent::AUDIO_ENDED));
}

void Pipeline::OnVideoRendererEnded() {
  // Force post to process ended messages after current execution frame.
  message_loop_->PostTask(FROM_HERE, base::Bind(
      &Pipeline::DoVideoRendererEnded, this));
  media_log_->AddEvent(media_log_->CreateEvent(MediaLogEvent::VIDEO_ENDED));
}

// Called from any thread.
void Pipeline::OnFilterInitialize(PipelineStatus status) {
  // Continue the initialize task by proceeding to the next stage.
  message_loop_->PostTask(FROM_HERE, base::Bind(
      &Pipeline::InitializeTask, this, status));
}

// Called from any thread.
// This method makes the PipelineStatusCB behave like a Closure. It
// makes it look like a host()->SetError() call followed by a call to
// OnFilterStateTransition() when errors occur.
//
// TODO(scherkus): Revisit this code when SetError() is removed from FilterHost
// and all the Closures are converted to PipelineStatusCB.
void Pipeline::OnFilterStateTransition(PipelineStatus status) {
  if (status != PIPELINE_OK)
    SetError(status);
  message_loop_->PostTask(FROM_HERE, base::Bind(
      &Pipeline::FilterStateTransitionTask, this));
}

void Pipeline::OnTeardownStateTransition(PipelineStatus status) {
  // Ignore any errors during teardown.
  message_loop_->PostTask(FROM_HERE, base::Bind(
      &Pipeline::TeardownStateTransitionTask, this));
}

// Called from any thread.
void Pipeline::OnUpdateStatistics(const PipelineStatistics& stats) {
  base::AutoLock auto_lock(lock_);
  statistics_.audio_bytes_decoded += stats.audio_bytes_decoded;
  statistics_.video_bytes_decoded += stats.video_bytes_decoded;
  statistics_.video_frames_decoded += stats.video_frames_decoded;
  statistics_.video_frames_dropped += stats.video_frames_dropped;
}

void Pipeline::StartTask(scoped_ptr<FilterCollection> filter_collection,
                         const PipelineStatusCB& ended_cb,
                         const PipelineStatusCB& error_cb,
                         const PipelineStatusCB& seek_cb,
                         const BufferingStateCB& buffering_state_cb) {
  DCHECK(message_loop_->BelongsToCurrentThread());
  CHECK_EQ(kCreated, state_)
      << "Media pipeline cannot be started more than once";

  filter_collection_ = filter_collection.Pass();
  ended_cb_ = ended_cb;
  error_cb_ = error_cb;
  seek_cb_ = seek_cb;
  buffering_state_cb_ = buffering_state_cb;

  // Kick off initialization.
  pipeline_init_state_.reset(new PipelineInitState());

  SetState(kInitDemuxer);
  InitializeDemuxer();
}

// Main initialization method called on the pipeline thread.  This code attempts
// to use the specified filter factory to build a pipeline.
// Initialization step performed in this method depends on current state of this
// object, indicated by |state_|.  After each step of initialization, this
// object transits to the next stage.  It starts by creating a Demuxer, and then
// connects the Demuxer's audio stream to an AudioDecoder which is then
// connected to an AudioRenderer.  If the media has video, then it connects a
// VideoDecoder to the Demuxer's video stream, and then connects the
// VideoDecoder to a VideoRenderer.
//
// When all required filters have been created and have called their
// FilterHost's InitializationComplete() method, the pipeline will update its
// state to kStarted and |init_cb_|, will be executed.
//
// TODO(hclam): InitializeTask() is now starting the pipeline asynchronously. It
// works like a big state change table. If we no longer need to start filters
// in order, we need to get rid of all the state change.
void Pipeline::InitializeTask(PipelineStatus last_stage_status) {
  DCHECK(message_loop_->BelongsToCurrentThread());

  if (last_stage_status != PIPELINE_OK) {
    SetError(last_stage_status);
    return;
  }

  // If we have received the stop or error signal, return immediately.
  if (!stop_cb_.is_null() || state_ == kStopped || !IsPipelineOk())
    return;

  DCHECK(state_ == kInitDemuxer ||
         state_ == kInitAudioDecoder ||
         state_ == kInitAudioRenderer ||
         state_ == kInitVideoRenderer);

  // Demuxer created, create audio decoder.
  if (state_ == kInitDemuxer) {
    SetState(kInitAudioDecoder);
    // If this method returns false, then there's no audio stream.
    if (InitializeAudioDecoder(demuxer_))
      return;
  }

  // Assuming audio decoder was created, create audio renderer.
  if (state_ == kInitAudioDecoder) {
    SetState(kInitAudioRenderer);

    // Returns false if there's no audio stream.
    if (InitializeAudioRenderer(pipeline_init_state_->audio_decoder)) {
      base::AutoLock auto_lock(lock_);
      has_audio_ = true;
      return;
    }
  }

  // Assuming audio renderer was created, create video renderer.
  if (state_ == kInitAudioRenderer) {
    SetState(kInitVideoRenderer);
    scoped_refptr<DemuxerStream> video_stream =
        demuxer_->GetStream(DemuxerStream::VIDEO);
    if (InitializeVideoRenderer(video_stream)) {
      base::AutoLock auto_lock(lock_);
      has_video_ = true;

      // Get an initial natural size so we have something when we signal
      // the kHaveMetadata buffering state.
      natural_size_ = video_stream->video_decoder_config().natural_size();
      return;
    }
  }

  if (state_ == kInitVideoRenderer) {
    if (!IsPipelineOk() || !(HasAudio() || HasVideo())) {
      SetError(PIPELINE_ERROR_COULD_NOT_RENDER);
      return;
    }

    // Clear initialization state now that we're done.
    filter_collection_.reset();
    pipeline_init_state_.reset();

    // Initialization was successful, we are now considered paused, so it's safe
    // to set the initial playback rate and volume.
    PlaybackRateChangedTask(GetPlaybackRate());
    VolumeChangedTask(GetVolume());

    buffering_state_cb_.Run(kHaveMetadata);

    // Fire a seek request to get the renderers to preroll. We can skip a seek
    // here as the demuxer should be at the start of the stream.
    seek_pending_ = true;
    SetState(kSeeking);
    seek_timestamp_ = demuxer_->GetStartTime();
    DoSeek(seek_timestamp_, true,
           base::Bind(&Pipeline::OnFilterStateTransition, this));
  }
}

// This method is called as a result of the client calling Pipeline::Stop() or
// as the result of an error condition.
// We stop the filters in the reverse order.
//
// TODO(scherkus): beware!  this can get posted multiple times since we post
// Stop() tasks even if we've already stopped.  Perhaps this should no-op for
// additional calls, however most of this logic will be changing.
void Pipeline::StopTask(const base::Closure& stop_cb) {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK(stop_cb_.is_null());

  if (state_ == kStopped) {
    stop_cb.Run();
    return;
  }

  if (video_renderer_)
    video_renderer_->PrepareForShutdownHack();

  if (tearing_down_ && status_ != PIPELINE_OK) {
    // If we are stopping due to SetError(), stop normally instead of
    // going to error state and calling |error_cb_|. This converts
    // the teardown in progress from an error teardown into one that acts
    // like the error never occurred.
    base::AutoLock auto_lock(lock_);
    status_ = PIPELINE_OK;
  }

  stop_cb_ = stop_cb;

  if (!IsPipelineSeeking() && !tearing_down_) {
    // We will tear down pipeline immediately when there is no seek operation
    // pending and no teardown in progress. This should include the case where
    // we are partially initialized.
    TearDownPipeline();
  }
}

void Pipeline::ErrorChangedTask(PipelineStatus error) {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK_NE(PIPELINE_OK, error) << "PIPELINE_OK isn't an error!";

  // Suppress executing additional error logic. Note that if we are currently
  // performing a normal stop, then we return immediately and continue the
  // normal stop.
  if (state_ == kStopped || tearing_down_) {
    return;
  }

  base::AutoLock auto_lock(lock_);
  status_ = error;

  // Posting TearDownPipeline() to message loop so that we can make sure
  // it runs after any pending callbacks that are already queued.
  // |tearing_down_| is set early here to make sure that pending callbacks
  // don't modify the state before TearDownPipeline() can run.
  tearing_down_ = true;
  message_loop_->PostTask(FROM_HERE, base::Bind(
      &Pipeline::TearDownPipeline, this));
}

void Pipeline::PlaybackRateChangedTask(float playback_rate) {
  DCHECK(message_loop_->BelongsToCurrentThread());

  if (state_ == kStopped || tearing_down_)
    return;

  // Suppress rate change until after seeking.
  if (IsPipelineSeeking()) {
    pending_playback_rate_ = playback_rate;
    playback_rate_change_pending_ = true;
    return;
  }

  {
    base::AutoLock auto_lock(lock_);
    clock_->SetPlaybackRate(playback_rate);
  }

  // These will get set after initialization completes in case playback rate is
  // set prior to initialization.
  if (demuxer_)
    demuxer_->SetPlaybackRate(playback_rate);
  if (audio_renderer_)
    audio_renderer_->SetPlaybackRate(playback_rate_);
  if (video_renderer_)
    video_renderer_->SetPlaybackRate(playback_rate_);
}

void Pipeline::VolumeChangedTask(float volume) {
  DCHECK(message_loop_->BelongsToCurrentThread());

  if (state_ == kStopped || tearing_down_)
    return;

  if (audio_renderer_)
    audio_renderer_->SetVolume(volume);
}

void Pipeline::SeekTask(TimeDelta time, const PipelineStatusCB& seek_cb) {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK(stop_cb_.is_null());

  // Suppress seeking if we're not fully started.
  if (state_ != kStarted) {
    // TODO(scherkus): should we run the callback?  I'm tempted to say the API
    // will only execute the first Seek() request.
    DVLOG(1) << "Media pipeline has not started, ignoring seek to "
             << time.InMicroseconds() << " (current state: " << state_ << ")";
    return;
  }

  DCHECK(!seek_pending_);
  seek_pending_ = true;

  // We'll need to pause every filter before seeking.  The state transition
  // is as follows:
  //   kStarted
  //   kPausing (for each filter)
  //   kSeeking (for each filter)
  //   kStarting (for each filter)
  //   kStarted
  SetState(kPausing);
  audio_ended_ = false;
  video_ended_ = false;
  seek_timestamp_ = std::max(time, demuxer_->GetStartTime());
  seek_cb_ = seek_cb;

  // Kick off seeking!
  {
    base::AutoLock auto_lock(lock_);
    if (clock_->IsPlaying())
      clock_->Pause();
  }
  DoPause(base::Bind(&Pipeline::OnFilterStateTransition, this));
}

void Pipeline::DoAudioRendererEnded() {
  DCHECK(message_loop_->BelongsToCurrentThread());

  if (state_ != kStarted)
    return;

  DCHECK(!audio_ended_);
  audio_ended_ = true;

  // Start clock since there is no more audio to trigger clock updates.
  if (!audio_disabled_) {
    base::AutoLock auto_lock(lock_);
    clock_->SetMaxTime(clock_->Duration());
    StartClockIfWaitingForTimeUpdate_Locked();
  }

  RunEndedCallbackIfNeeded();
}

void Pipeline::DoVideoRendererEnded() {
  DCHECK(message_loop_->BelongsToCurrentThread());

  if (state_ != kStarted)
    return;

  DCHECK(!video_ended_);
  video_ended_ = true;

  RunEndedCallbackIfNeeded();
}

void Pipeline::RunEndedCallbackIfNeeded() {
  DCHECK(message_loop_->BelongsToCurrentThread());

  if (audio_renderer_ && !audio_ended_ && !audio_disabled_)
    return;

  if (video_renderer_ && !video_ended_)
    return;

  {
    base::AutoLock auto_lock(lock_);
    clock_->EndOfStream();
  }

  ReportStatus(ended_cb_, status_);
}

void Pipeline::AudioDisabledTask() {
  DCHECK(message_loop_->BelongsToCurrentThread());

  base::AutoLock auto_lock(lock_);
  has_audio_ = false;
  audio_disabled_ = true;

  // Notify our demuxer that we're no longer rendering audio.
  demuxer_->OnAudioRendererDisabled();

  // Start clock since there is no more audio to
  // trigger clock updates.
  clock_->SetMaxTime(clock_->Duration());
  StartClockIfWaitingForTimeUpdate_Locked();
}

void Pipeline::FilterStateTransitionTask() {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK(pending_callbacks_.get())
      << "Filter state transitions must be completed via pending_callbacks_";
  pending_callbacks_.reset();

  // State transitions while tearing down are handled via
  // TeardownStateTransitionTask().
  //
  // TODO(scherkus): Merge all state machinery!
  if (state_ == kStopped || tearing_down_) {
    return;
  }

  if (!TransientState(state_)) {
    NOTREACHED() << "Invalid current state: " << state_;
    SetError(PIPELINE_ERROR_ABORT);
    return;
  }

  // Decrement the number of remaining transitions, making sure to transition
  // to the next state if needed.
  SetState(FindNextState(state_));
  if (state_ == kSeeking) {
    base::AutoLock auto_lock(lock_);
    DCHECK(seek_timestamp_ != kNoTimestamp());
    clock_->SetTime(seek_timestamp_, seek_timestamp_);
  }

  // Carry out the action for the current state.
  if (TransientState(state_)) {
    if (state_ == kPausing) {
      DoPause(base::Bind(&Pipeline::OnFilterStateTransition, this));
    } else if (state_ == kFlushing) {
      DoFlush(base::Bind(&Pipeline::OnFilterStateTransition, this));
    } else if (state_ == kSeeking) {
      DoSeek(seek_timestamp_, false,
             base::Bind(&Pipeline::OnFilterStateTransition, this));
    } else if (state_ == kStarting) {
      DoPlay(base::Bind(&Pipeline::OnFilterStateTransition, this));
    } else if (state_ == kStopping) {
      DoStop(base::Bind(&Pipeline::OnFilterStateTransition, this));
    } else {
      NOTREACHED() << "Unexpected state: " << state_;
    }
  } else if (state_ == kStarted) {

    // Fire canplaythrough immediately after playback begins because of
    // crbug.com/106480.
    // TODO(vrk): set ready state to HaveFutureData when bug above is fixed.
    if (status_ == PIPELINE_OK)
      buffering_state_cb_.Run(kPrerollCompleted);

    FinishSeek();

    // If a playback rate change was requested during a seek, do it now that
    // the seek has compelted.
    if (playback_rate_change_pending_) {
      playback_rate_change_pending_ = false;
      PlaybackRateChangedTask(pending_playback_rate_);
    }

    base::AutoLock auto_lock(lock_);
    // We use audio stream to update the clock. So if there is such a stream,
    // we pause the clock until we receive a valid timestamp.
    waiting_for_clock_update_ = true;
    if (!has_audio_) {
      clock_->SetMaxTime(clock_->Duration());
      StartClockIfWaitingForTimeUpdate_Locked();
    }

    // Check if we have a pending stop request that needs to be honored.
    if (!stop_cb_.is_null()) {
      TearDownPipeline();
    }
  } else {
    NOTREACHED() << "Unexpected state: " << state_;
  }
}

void Pipeline::TeardownStateTransitionTask() {
  DCHECK(tearing_down_);
  DCHECK(pending_callbacks_.get())
      << "Teardown state transitions must be completed via pending_callbacks_";
  pending_callbacks_.reset();

  switch (state_) {
    case kStopping:
      SetState(kStopped);
      FinishDestroyingFiltersTask();
      break;
    case kPausing:
      SetState(kFlushing);
      DoFlush(base::Bind(&Pipeline::OnTeardownStateTransition, this));
      break;
    case kFlushing:
      SetState(kStopping);
      DoStop(base::Bind(&Pipeline::OnTeardownStateTransition, this));
      break;

    case kCreated:
    case kInitDemuxer:
    case kInitAudioDecoder:
    case kInitAudioRenderer:
    case kInitVideoRenderer:
    case kSeeking:
    case kStarting:
    case kStopped:
    case kStarted:
      NOTREACHED() << "Unexpected state for teardown: " << state_;
      break;
    // default: intentionally left out to force new states to cause compiler
    // errors.
  };
}

void Pipeline::FinishDestroyingFiltersTask() {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK_EQ(state_, kStopped);

  audio_renderer_ = NULL;
  video_renderer_ = NULL;
  demuxer_ = NULL;
  tearing_down_ = false;
  {
    base::AutoLock l(lock_);
    running_ = false;
  }

  if (!IsPipelineOk() && !error_cb_.is_null())
    error_cb_.Run(status_);

  if (!stop_cb_.is_null())
    base::ResetAndReturn(&stop_cb_).Run();
}

void Pipeline::InitializeDemuxer() {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK(IsPipelineOk());

  demuxer_ = filter_collection_->GetDemuxer();
  if (!demuxer_) {
    SetError(PIPELINE_ERROR_REQUIRED_FILTER_MISSING);
    return;
  }

  demuxer_->Initialize(this, base::Bind(&Pipeline::OnDemuxerInitialized, this));
}

void Pipeline::OnDemuxerInitialized(PipelineStatus status) {
  if (!message_loop_->BelongsToCurrentThread()) {
    message_loop_->PostTask(FROM_HERE, base::Bind(
        &Pipeline::OnDemuxerInitialized, this, status));
    return;
  }

  if (status != PIPELINE_OK) {
    SetError(status);
    return;
  }

  {
    base::AutoLock auto_lock(lock_);
    // We do not want to start the clock running. We only want to set the base
    // media time so our timestamp calculations will be correct.
    clock_->SetTime(demuxer_->GetStartTime(), demuxer_->GetStartTime());
  }

  OnFilterInitialize(PIPELINE_OK);
}

bool Pipeline::InitializeAudioDecoder(
    const scoped_refptr<Demuxer>& demuxer) {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK(IsPipelineOk());
  DCHECK(demuxer);

  scoped_refptr<DemuxerStream> stream =
      demuxer->GetStream(DemuxerStream::AUDIO);

  if (!stream)
    return false;

  filter_collection_->SelectAudioDecoder(&pipeline_init_state_->audio_decoder);

  if (!pipeline_init_state_->audio_decoder) {
    SetError(PIPELINE_ERROR_REQUIRED_FILTER_MISSING);
    return false;
  }

  pipeline_init_state_->audio_decoder->Initialize(
      stream,
      base::Bind(&Pipeline::OnFilterInitialize, this),
      base::Bind(&Pipeline::OnUpdateStatistics, this));
  return true;
}

bool Pipeline::InitializeAudioRenderer(
    const scoped_refptr<AudioDecoder>& decoder) {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK(IsPipelineOk());

  if (!decoder)
    return false;

  filter_collection_->SelectAudioRenderer(&audio_renderer_);
  if (!audio_renderer_) {
    SetError(PIPELINE_ERROR_REQUIRED_FILTER_MISSING);
    return false;
  }

  audio_renderer_->Initialize(
      decoder,
      base::Bind(&Pipeline::OnFilterInitialize, this),
      base::Bind(&Pipeline::OnAudioUnderflow, this),
      base::Bind(&Pipeline::OnAudioTimeUpdate, this),
      base::Bind(&Pipeline::OnAudioRendererEnded, this),
      base::Bind(&Pipeline::OnAudioDisabled, this),
      base::Bind(&Pipeline::SetError, this));
  return true;
}

bool Pipeline::InitializeVideoRenderer(
    const scoped_refptr<DemuxerStream>& stream) {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK(IsPipelineOk());

  if (!stream)
    return false;

  filter_collection_->SelectVideoRenderer(&video_renderer_);
  if (!video_renderer_) {
    SetError(PIPELINE_ERROR_REQUIRED_FILTER_MISSING);
    return false;
  }

  video_renderer_->Initialize(
      stream,
      *filter_collection_->GetVideoDecoders(),
      base::Bind(&Pipeline::OnFilterInitialize, this),
      base::Bind(&Pipeline::OnUpdateStatistics, this),
      base::Bind(&Pipeline::OnVideoTimeUpdate, this),
      base::Bind(&Pipeline::OnNaturalVideoSizeChanged, this),
      base::Bind(&Pipeline::OnVideoRendererEnded, this),
      base::Bind(&Pipeline::SetError, this),
      base::Bind(&Pipeline::GetMediaTime, this),
      base::Bind(&Pipeline::GetMediaDuration, this));
  filter_collection_->GetVideoDecoders()->clear();
  return true;
}

void Pipeline::TearDownPipeline() {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK_NE(kStopped, state_);

  // We're either...
  //   1) ...tearing down due to Stop() (it doesn't set tearing_down_)
  //   2) ...tearing down due to an error (it does set tearing_down_)
  //   3) ...tearing down due to an error and Stop() was called during that time
  DCHECK(!tearing_down_ ||
         (tearing_down_ && status_ != PIPELINE_OK) ||
         (tearing_down_ && !stop_cb_.is_null()));

  // Mark that we already start tearing down operation.
  tearing_down_ = true;

  // Cancel any pending operation so we can proceed with teardown.
  pending_callbacks_.reset();

  switch (state_) {
    case kCreated:
      SetState(kStopped);
      // Need to put this in the message loop to make sure that it comes
      // after any pending callback tasks that are already queued.
      message_loop_->PostTask(FROM_HERE, base::Bind(
          &Pipeline::FinishDestroyingFiltersTask, this));
      break;

    case kInitDemuxer:
    case kInitAudioDecoder:
    case kInitAudioRenderer:
    case kInitVideoRenderer:
      // Make it look like initialization was successful.
      filter_collection_.reset();
      pipeline_init_state_.reset();

      SetState(kStopping);
      DoStop(base::Bind(&Pipeline::OnTeardownStateTransition, this));

      FinishSeek();
      break;

    case kPausing:
    case kSeeking:
    case kFlushing:
    case kStarting:
      SetState(kStopping);
      DoStop(base::Bind(&Pipeline::OnTeardownStateTransition, this));

      if (seek_pending_)
        FinishSeek();

      break;

    case kStarted:
      SetState(kPausing);
      DoPause(base::Bind(&Pipeline::OnTeardownStateTransition, this));
      break;

    case kStopping:
    case kStopped:
      NOTREACHED() << "Unexpected state for teardown: " << state_;
      break;
    // default: intentionally left out to force new states to cause compiler
    // errors.
  };
}

void Pipeline::DoSeek(base::TimeDelta seek_timestamp,
                      bool skip_demuxer_seek,
                      const PipelineStatusCB& done_cb) {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK(!pending_callbacks_.get());
  SerialRunner::Queue bound_fns;

  if (!skip_demuxer_seek) {
    bound_fns.Push(base::Bind(
        &Demuxer::Seek, demuxer_, seek_timestamp));
  }

  if (audio_renderer_) {
    bound_fns.Push(base::Bind(
        &AudioRenderer::Preroll, audio_renderer_, seek_timestamp));
  }

  if (video_renderer_) {
    bound_fns.Push(base::Bind(
        &VideoRenderer::Preroll, video_renderer_, seek_timestamp));
  }

  pending_callbacks_ = SerialRunner::Run(bound_fns, done_cb);
}

void Pipeline::OnAudioUnderflow() {
  if (!message_loop_->BelongsToCurrentThread()) {
    message_loop_->PostTask(FROM_HERE, base::Bind(
        &Pipeline::OnAudioUnderflow, this));
    return;
  }

  if (state_ != kStarted)
    return;

  if (audio_renderer_)
    audio_renderer_->ResumeAfterUnderflow(true);
}

void Pipeline::StartClockIfWaitingForTimeUpdate_Locked() {
  lock_.AssertAcquired();
  if (!waiting_for_clock_update_)
    return;

  waiting_for_clock_update_ = false;
  clock_->Play();
}

}  // namespace media
