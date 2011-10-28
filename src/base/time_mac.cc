// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/time.h"

#include <CoreFoundation/CFDate.h>
#include <CoreFoundation/CFTimeZone.h>
#include <mach/mach_time.h>
#include <sys/time.h>
#include <time.h>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/mac/scoped_cftyperef.h"

namespace base {

// The Time routines in this file use Mach and CoreFoundation APIs, since the
// POSIX definition of time_t in Mac OS X wraps around after 2038--and
// there are already cookie expiration dates, etc., past that time out in
// the field.  Using CFDate prevents that problem, and using mach_absolute_time
// for TimeTicks gives us nice high-resolution interval timing.

// Time -----------------------------------------------------------------------

// Core Foundation uses a double second count since 2001-01-01 00:00:00 UTC.
// The UNIX epoch is 1970-01-01 00:00:00 UTC.
// Windows uses a Gregorian epoch of 1601.  We need to match this internally
// so that our time representations match across all platforms.  See bug 14734.
//   irb(main):010:0> Time.at(0).getutc()
//   => Thu Jan 01 00:00:00 UTC 1970
//   irb(main):011:0> Time.at(-11644473600).getutc()
//   => Mon Jan 01 00:00:00 UTC 1601
static const int64 kWindowsEpochDeltaSeconds = GG_INT64_C(11644473600);
static const int64 kWindowsEpochDeltaMilliseconds =
    kWindowsEpochDeltaSeconds * Time::kMillisecondsPerSecond;

// static
const int64 Time::kWindowsEpochDeltaMicroseconds =
    kWindowsEpochDeltaSeconds * Time::kMicrosecondsPerSecond;

// Some functions in time.cc use time_t directly, so we provide an offset
// to convert from time_t (Unix epoch) and internal (Windows epoch).
// static
const int64 Time::kTimeTToMicrosecondsOffset = kWindowsEpochDeltaMicroseconds;

// static
Time Time::Now() {
  CFAbsoluteTime now =
      CFAbsoluteTimeGetCurrent() + kCFAbsoluteTimeIntervalSince1970;
  return Time(static_cast<int64>(now * kMicrosecondsPerSecond) +
      kWindowsEpochDeltaMicroseconds);
}

// static
Time Time::NowFromSystemTime() {
  // Just use Now() because Now() returns the system time.
  return Now();
}

// static
Time Time::FromExploded(bool is_local, const Exploded& exploded) {
  CFGregorianDate date;
  date.second = exploded.second +
      exploded.millisecond / static_cast<double>(kMillisecondsPerSecond);
  date.minute = exploded.minute;
  date.hour = exploded.hour;
  date.day = exploded.day_of_month;
  date.month = exploded.month;
  date.year = exploded.year;

  base::mac::ScopedCFTypeRef<CFTimeZoneRef>
      time_zone(is_local ? CFTimeZoneCopySystem() : NULL);
  CFAbsoluteTime seconds = CFGregorianDateGetAbsoluteTime(date, time_zone) +
      kCFAbsoluteTimeIntervalSince1970;
  return Time(static_cast<int64>(seconds * kMicrosecondsPerSecond) +
      kWindowsEpochDeltaMicroseconds);
}

void Time::Explode(bool is_local, Exploded* exploded) const {
  CFAbsoluteTime seconds =
      ((static_cast<double>(us_) - kWindowsEpochDeltaMicroseconds) /
      kMicrosecondsPerSecond) - kCFAbsoluteTimeIntervalSince1970;

  base::mac::ScopedCFTypeRef<CFTimeZoneRef>
      time_zone(is_local ? CFTimeZoneCopySystem() : NULL);
  CFGregorianDate date = CFAbsoluteTimeGetGregorianDate(seconds, time_zone);

  exploded->year = date.year;
  exploded->month = date.month;
  exploded->day_of_month = date.day;
  exploded->hour = date.hour;
  exploded->minute = date.minute;
  exploded->second = date.second;
  exploded->millisecond  =
      static_cast<int>(date.second * kMillisecondsPerSecond) %
      kMillisecondsPerSecond;
}

// TimeTicks ------------------------------------------------------------------

// static
TimeTicks TimeTicks::Now() {
  uint64_t absolute_micro;

  static mach_timebase_info_data_t timebase_info;
  if (timebase_info.denom == 0) {
    // Zero-initialization of statics guarantees that denom will be 0 before
    // calling mach_timebase_info.  mach_timebase_info will never set denom to
    // 0 as that would be invalid, so the zero-check can be used to determine
    // whether mach_timebase_info has already been called.  This is
    // recommended by Apple's QA1398.
    kern_return_t kr = mach_timebase_info(&timebase_info);
    DCHECK_EQ(KERN_SUCCESS, kr);
  }

  // mach_absolute_time is it when it comes to ticks on the Mac.  Other calls
  // with less precision (such as TickCount) just call through to
  // mach_absolute_time.

  // timebase_info converts absolute time tick units into nanoseconds.  Convert
  // to microseconds up front to stave off overflows.
  absolute_micro = mach_absolute_time() / Time::kNanosecondsPerMicrosecond *
                   timebase_info.numer / timebase_info.denom;

  // Don't bother with the rollover handling that the Windows version does.
  // With numer and denom = 1 (the expected case), the 64-bit absolute time
  // reported in nanoseconds is enough to last nearly 585 years.

  return TimeTicks(absolute_micro);
}

// static
TimeTicks TimeTicks::HighResNow() {
  return Now();
}

}  // namespace base
