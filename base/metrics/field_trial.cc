// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/metrics/field_trial.h"

#include "base/build_time.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "base/sha1.h"
#include "base/stringprintf.h"
#include "base/string_util.h"
#include "base/sys_byteorder.h"
#include "base/utf_string_conversions.h"

namespace base {

namespace {

// Created a time value based on |year|, |month| and |day_of_month| parameters.
Time CreateTimeFromParams(int year, int month, int day_of_month) {
  DCHECK_GT(year, 1970);
  DCHECK_GT(month, 0);
  DCHECK_LT(month, 13);
  DCHECK_GT(day_of_month, 0);
  DCHECK_LT(day_of_month, 32);

  Time::Exploded exploded;
  exploded.year = year;
  exploded.month = month;
  exploded.day_of_week = 0;  // Should be unused.
  exploded.day_of_month = day_of_month;
  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;

  return Time::FromLocalExploded(exploded);
}

}  // namespace

static const char kHistogramFieldTrialSeparator('_');

// statics
const int FieldTrial::kNotFinalized = -1;
const int FieldTrial::kDefaultGroupNumber = 0;
bool FieldTrial::enable_benchmarking_ = false;

const char FieldTrialList::kPersistentStringSeparator('/');
int FieldTrialList::kExpirationYearInFuture = 0;

//------------------------------------------------------------------------------
// FieldTrial methods and members.

FieldTrial::FieldTrial(const std::string& name,
                       const Probability total_probability,
                       const std::string& default_group_name)
    : name_(name),
      divisor_(total_probability),
      default_group_name_(default_group_name),
      random_(static_cast<Probability>(divisor_ * RandDouble())),
      accumulated_group_probability_(0),
      next_group_number_(kDefaultGroupNumber + 1),
      group_(kNotFinalized),
      enable_field_trial_(true),
      forced_(false) {
  DCHECK_GT(total_probability, 0);
  DCHECK(!name_.empty());
  DCHECK(!default_group_name_.empty());
}

FieldTrial::EntropyProvider::~EntropyProvider() {
}

void FieldTrial::UseOneTimeRandomization() {
  // No need to specify randomization when the group choice was forced.
  if (forced_)
    return;
  DCHECK_EQ(group_, kNotFinalized);
  DCHECK_EQ(kDefaultGroupNumber + 1, next_group_number_);
  const EntropyProvider* entropy_provider =
      FieldTrialList::GetEntropyProviderForOneTimeRandomization();
  if (!entropy_provider) {
    NOTREACHED();
    Disable();
    return;
  }

  random_ = static_cast<Probability>(
      divisor_ * entropy_provider->GetEntropyForTrial(name_));
}

void FieldTrial::Disable() {
  enable_field_trial_ = false;

  // In case we are disabled after initialization, we need to switch
  // the trial to the default group.
  if (group_ != kNotFinalized) {
    // Only reset when not already the default group, because in case we were
    // forced to the default group, the group number may not be
    // kDefaultGroupNumber, so we should keep it as is.
    if (group_name_ != default_group_name_)
      SetGroupChoice(default_group_name_, kDefaultGroupNumber);
  }
}

int FieldTrial::AppendGroup(const std::string& name,
                            Probability group_probability) {
  // When the group choice was previously forced, we only need to return the
  // the id of the chosen group, and anything can be returned for the others.
  if (forced_) {
    DCHECK(!group_name_.empty());
    if (name == group_name_) {
      return group_;
    }
    DCHECK_NE(next_group_number_, group_);
    // We still return different numbers each time, in case some caller need
    // them to be different.
    return next_group_number_++;
  }

  DCHECK_LE(group_probability, divisor_);
  DCHECK_GE(group_probability, 0);

  if (enable_benchmarking_ || !enable_field_trial_)
    group_probability = 0;

  accumulated_group_probability_ += group_probability;

  DCHECK_LE(accumulated_group_probability_, divisor_);
  if (group_ == kNotFinalized && accumulated_group_probability_ > random_) {
    // This is the group that crossed the random line, so we do the assignment.
    SetGroupChoice(name, next_group_number_);
    FieldTrialList::NotifyFieldTrialGroupSelection(name_, group_name_);
  }
  return next_group_number_++;
}

int FieldTrial::group() {
  if (group_ == kNotFinalized) {
    accumulated_group_probability_ = divisor_;
    // Here it's OK to use kDefaultGroupNumber
    // since we can't be forced and not finalized.
    DCHECK(!forced_);
    SetGroupChoice(default_group_name_, kDefaultGroupNumber);
    FieldTrialList::NotifyFieldTrialGroupSelection(name_, group_name_);
  }
  return group_;
}

std::string FieldTrial::group_name() {
  group();  // call group() to make sure group assignment was done.
  DCHECK(!group_name_.empty());
  return group_name_;
}

bool FieldTrial::GetSelectedGroup(SelectedGroup* selected_group) {
  if (group_ == kNotFinalized)
    return false;
  selected_group->trial = name_;
  selected_group->group = group_name_;
  return true;
}

// static
std::string FieldTrial::MakeName(const std::string& name_prefix,
                                 const std::string& trial_name) {
  std::string big_string(name_prefix);
  big_string.append(1, kHistogramFieldTrialSeparator);
  return big_string.append(FieldTrialList::FindFullName(trial_name));
}

// static
void FieldTrial::EnableBenchmarking() {
  DCHECK_EQ(0u, FieldTrialList::GetFieldTrialCount());
  enable_benchmarking_ = true;
}

void FieldTrial::SetForced() {
  // We might have been forced before (e.g., by CreateFieldTrial) and it's
  // first come first served, e.g., command line switch has precedence.
  if (forced_)
    return;
  // Explicit forcing should only be for cases where we want to set the group
  // probabilities before the hard coded field trial setup is executed. So
  // there must have been at least one non-default group appended at that point.
  DCHECK_GT(next_group_number_, kDefaultGroupNumber + 1);

  // And we must finalize the group choice before we mark ourselves as forced.
  group();
  forced_ = true;
}

FieldTrial::~FieldTrial() {}

void FieldTrial::SetGroupChoice(const std::string& name, int number) {
  group_ = number;
  if (name.empty())
    StringAppendF(&group_name_, "%d", group_);
  else
    group_name_ = name;
  DVLOG(1) << "Field trial: " << name_ << " Group choice:" << group_name_;
}

//------------------------------------------------------------------------------
// FieldTrialList methods and members.

// static
FieldTrialList* FieldTrialList::global_ = NULL;

// static
bool FieldTrialList::used_without_global_ = false;

FieldTrialList::Observer::~Observer() {
}

FieldTrialList::FieldTrialList(
    const FieldTrial::EntropyProvider* entropy_provider)
    : application_start_time_(TimeTicks::Now()),
      entropy_provider_(entropy_provider),
      observer_list_(new ObserverListThreadSafe<FieldTrialList::Observer>(
          ObserverListBase<FieldTrialList::Observer>::NOTIFY_EXISTING_ONLY)) {
  DCHECK(!global_);
  DCHECK(!used_without_global_);
  global_ = this;

  Time::Exploded exploded;
  Time two_years_from_now =
      Time::NowFromSystemTime() + TimeDelta::FromDays(730);
  two_years_from_now.LocalExplode(&exploded);
  kExpirationYearInFuture = exploded.year;
}

FieldTrialList::~FieldTrialList() {
  AutoLock auto_lock(lock_);
  while (!registered_.empty()) {
    RegistrationList::iterator it = registered_.begin();
    it->second->Release();
    registered_.erase(it->first);
  }
  DCHECK_EQ(this, global_);
  global_ = NULL;
}

// static
FieldTrial* FieldTrialList::FactoryGetFieldTrial(
    const std::string& name,
    FieldTrial::Probability total_probability,
    const std::string& default_group_name,
    const int year,
    const int month,
    const int day_of_month,
    int* default_group_number) {
  if (default_group_number)
    *default_group_number = FieldTrial::kDefaultGroupNumber;
  // Check if the field trial has already been created in some other way.
  FieldTrial* existing_trial = Find(name);
  if (existing_trial) {
    CHECK(existing_trial->forced_);
    // If the field trial has already been forced, check whether it was forced
    // to the default group. Return the chosen group number, in that case..
    if (default_group_number &&
        default_group_name == existing_trial->default_group_name()) {
      *default_group_number = existing_trial->group();
    }
    return existing_trial;
  }

  FieldTrial* field_trial =
      new FieldTrial(name, total_probability, default_group_name);
  if (GetBuildTime() > CreateTimeFromParams(year, month, day_of_month))
    field_trial->Disable();
  FieldTrialList::Register(field_trial);
  return field_trial;
}

// static
FieldTrial* FieldTrialList::Find(const std::string& name) {
  if (!global_)
    return NULL;
  AutoLock auto_lock(global_->lock_);
  return global_->PreLockedFind(name);
}

// static
int FieldTrialList::FindValue(const std::string& name) {
  FieldTrial* field_trial = Find(name);
  if (field_trial)
    return field_trial->group();
  return FieldTrial::kNotFinalized;
}

// static
std::string FieldTrialList::FindFullName(const std::string& name) {
  FieldTrial* field_trial = Find(name);
  if (field_trial)
    return field_trial->group_name();
  return "";
}

// static
bool FieldTrialList::TrialExists(const std::string& name) {
  return Find(name) != NULL;
}

// static
void FieldTrialList::StatesToString(std::string* output) {
  DCHECK(output->empty());
  if (!global_)
    return;
  AutoLock auto_lock(global_->lock_);

  for (RegistrationList::iterator it = global_->registered_.begin();
       it != global_->registered_.end(); ++it) {
    const std::string& name = it->first;
    std::string group_name = it->second->group_name_internal();
    if (group_name.empty())
      continue;  // Should not include uninitialized trials.
    DCHECK_EQ(name.find(kPersistentStringSeparator), std::string::npos);
    DCHECK_EQ(group_name.find(kPersistentStringSeparator), std::string::npos);
    output->append(name);
    output->append(1, kPersistentStringSeparator);
    output->append(group_name);
    output->append(1, kPersistentStringSeparator);
  }
}

// static
void FieldTrialList::GetFieldTrialSelectedGroups(
    FieldTrial::SelectedGroups* selected_groups) {
  DCHECK(selected_groups->empty());
  if (!global_)
    return;
  AutoLock auto_lock(global_->lock_);

  for (RegistrationList::iterator it = global_->registered_.begin();
       it != global_->registered_.end(); ++it) {
    FieldTrial::SelectedGroup selected_group;
    if (it->second->GetSelectedGroup(&selected_group))
      selected_groups->push_back(selected_group);
  }
}

// static
bool FieldTrialList::CreateTrialsFromString(const std::string& trials_string) {
  DCHECK(global_);
  if (trials_string.empty() || !global_)
    return true;

  size_t next_item = 0;
  while (next_item < trials_string.length()) {
    size_t name_end = trials_string.find(kPersistentStringSeparator, next_item);
    if (name_end == trials_string.npos || next_item == name_end)
      return false;
    size_t group_name_end = trials_string.find(kPersistentStringSeparator,
                                               name_end + 1);
    if (group_name_end == trials_string.npos || name_end + 1 == group_name_end)
      return false;
    std::string name(trials_string, next_item, name_end - next_item);
    std::string group_name(trials_string, name_end + 1,
                           group_name_end - name_end - 1);
    next_item = group_name_end + 1;

    if (!CreateFieldTrial(name, group_name))
      return false;
  }
  return true;
}

// static
FieldTrial* FieldTrialList::CreateFieldTrial(
    const std::string& name,
    const std::string& group_name) {
  DCHECK(global_);
  DCHECK_GE(name.size(), 0u);
  DCHECK_GE(group_name.size(), 0u);
  if (name.empty() || group_name.empty() || !global_)
    return NULL;

  FieldTrial* field_trial = FieldTrialList::Find(name);
  if (field_trial) {
    // In single process mode, or when we force them from the command line,
    // we may have already created the field trial.
    if (field_trial->group_name_internal() != group_name)
      return NULL;
    return field_trial;
  }
  const int kTotalProbability = 100;
  field_trial = new FieldTrial(name, kTotalProbability, group_name);
  // This is where we may assign a group number different from
  // kDefaultGroupNumber to the default group.
  field_trial->AppendGroup(group_name, kTotalProbability);
  field_trial->forced_ = true;
  FieldTrialList::Register(field_trial);
  return field_trial;
}

// static
void FieldTrialList::AddObserver(Observer* observer) {
  if (!global_)
    return;
  DCHECK(global_);
  global_->observer_list_->AddObserver(observer);
}

// static
void FieldTrialList::RemoveObserver(Observer* observer) {
  if (!global_)
    return;
  DCHECK(global_);
  global_->observer_list_->RemoveObserver(observer);
}

// static
void FieldTrialList::NotifyFieldTrialGroupSelection(
    const std::string& name,
    const std::string& group_name) {
  if (!global_)
    return;
  DCHECK(global_);
  global_->observer_list_->Notify(
      &FieldTrialList::Observer::OnFieldTrialGroupFinalized,
      name,
      group_name);
}

// static
size_t FieldTrialList::GetFieldTrialCount() {
  if (!global_)
    return 0;
  AutoLock auto_lock(global_->lock_);
  return global_->registered_.size();
}

// static
const FieldTrial::EntropyProvider*
    FieldTrialList::GetEntropyProviderForOneTimeRandomization() {
  if (!global_) {
    used_without_global_ = true;
    return NULL;
  }

  return global_->entropy_provider_.get();
}

// static
bool FieldTrialList::IsOneTimeRandomizationEnabled() {
  return GetEntropyProviderForOneTimeRandomization() != NULL;
}

FieldTrial* FieldTrialList::PreLockedFind(const std::string& name) {
  RegistrationList::iterator it = registered_.find(name);
  if (registered_.end() == it)
    return NULL;
  return it->second;
}

// static
void FieldTrialList::Register(FieldTrial* trial) {
  if (!global_) {
    used_without_global_ = true;
    return;
  }
  AutoLock auto_lock(global_->lock_);
  DCHECK(!global_->PreLockedFind(trial->name()));
  trial->AddRef();
  global_->registered_[trial->name()] = trial;
}

}  // namespace base
