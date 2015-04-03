// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/debug/frame_timing_tracker.h"

#include <algorithm>
#include <limits>

#include "base/metrics/histogram.h"
#include "cc/trees/proxy.h"

namespace cc {

FrameTimingTracker::CompositeTimingEvent::CompositeTimingEvent(
    int _frame_id,
    base::TimeTicks _timestamp)
    : frame_id(_frame_id), timestamp(_timestamp) {
}

FrameTimingTracker::CompositeTimingEvent::~CompositeTimingEvent() {
}

FrameTimingTracker::MainFrameTimingEvent::MainFrameTimingEvent(
    int frame_id,
    base::TimeTicks timestamp,
    base::TimeTicks end_time)
    : frame_id(frame_id), timestamp(timestamp), end_time(end_time) {
}

FrameTimingTracker::MainFrameTimingEvent::~MainFrameTimingEvent() {
}

// static
scoped_ptr<FrameTimingTracker> FrameTimingTracker::Create() {
  return make_scoped_ptr(new FrameTimingTracker);
}

FrameTimingTracker::FrameTimingTracker() {
}

FrameTimingTracker::~FrameTimingTracker() {
}

void FrameTimingTracker::SaveTimeStamps(
    base::TimeTicks timestamp,
    const std::vector<FrameAndRectIds>& frame_ids) {
  if (!composite_events_)
    composite_events_.reset(new CompositeTimingSet);
  for (const auto& pair : frame_ids) {
    (*composite_events_)[pair.second].push_back(
        CompositeTimingEvent(pair.first, timestamp));
  }
}

void FrameTimingTracker::SaveMainFrameTimeStamps(
    const std::vector<int64_t>& request_ids,
    base::TimeTicks main_frame_time,
    base::TimeTicks end_time,
    int source_frame_number) {
  if (!main_frame_events_)
    main_frame_events_.reset(new MainFrameTimingSet);
  for (const auto& request : request_ids) {
    std::vector<MainFrameTimingEvent>& events = (*main_frame_events_)[request];
    events.push_back(
        MainFrameTimingEvent(source_frame_number, main_frame_time, end_time));
  }
}

scoped_ptr<FrameTimingTracker::CompositeTimingSet>
FrameTimingTracker::GroupCompositeCountsByRectId() {
  if (!composite_events_)
    return make_scoped_ptr(new CompositeTimingSet);
  for (auto& infos : *composite_events_) {
    std::sort(
        infos.second.begin(), infos.second.end(),
        [](const CompositeTimingEvent& lhs, const CompositeTimingEvent& rhs) {
          return lhs.timestamp < rhs.timestamp;
        });
  }
  return composite_events_.Pass();
}

scoped_ptr<FrameTimingTracker::MainFrameTimingSet>
FrameTimingTracker::GroupMainFrameCountsByRectId() {
  if (!main_frame_events_)
    return make_scoped_ptr(new MainFrameTimingSet);
  for (auto& infos : *main_frame_events_) {
    std::sort(
        infos.second.begin(), infos.second.end(),
        [](const MainFrameTimingEvent& lhs, const MainFrameTimingEvent& rhs) {
          return lhs.timestamp < rhs.timestamp;
        });
  }
  return main_frame_events_.Pass();
}

}  // namespace cc
