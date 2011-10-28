// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/spellchecker/spellcheck_profile.h"

#include "base/file_util.h"
#include "base/lazy_instance.h"
#include "base/string_split.h"
#include "base/string_util.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/spellchecker/spellcheck_host.h"
#include "chrome/browser/spellchecker/spellcheck_host_metrics.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/pref_names.h"

namespace {
base::LazyInstance<SpellCheckProfile::CustomWordList> g_empty_list(
    base::LINKER_INITIALIZED);
}  // namespace

SpellCheckProfile::SpellCheckProfile(const FilePath& profile_dir)
    : host_ready_(false),
      profile_dir_(profile_dir) {
}

SpellCheckProfile::~SpellCheckProfile() {
  if (host_.get())
    host_->UnsetProfile();
}

SpellCheckHost* SpellCheckProfile::GetHost() {
  return host_ready_ ? host_.get() : NULL;
}

SpellCheckProfile::ReinitializeResult SpellCheckProfile::ReinitializeHost(
    bool force,
    bool enable,
    const std::string& language,
    net::URLRequestContextGetter* request_context) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI) || IsTesting());
  // If we are already loading the spellchecker, and this is just a hint to
  // load the spellchecker, do nothing.
  if (!force && host_.get())
    return REINITIALIZE_DID_NOTHING;

  host_ready_ = false;

  bool host_deleted = false;
  if (host_.get()) {
    host_->UnsetProfile();
    host_ = NULL;
    host_deleted = true;
  }

  if (enable) {
    // Retrieve the (perhaps updated recently) dictionary name from preferences.
    host_ = CreateHost(this, language, request_context, metrics_.get());
    return REINITIALIZE_CREATED_HOST;
  }

  return host_deleted ? REINITIALIZE_REMOVED_HOST : REINITIALIZE_DID_NOTHING;
}

SpellCheckHost* SpellCheckProfile::CreateHost(
    SpellCheckProfileProvider* provider,
    const std::string& language,
    net::URLRequestContextGetter* request_context,
    SpellCheckHostMetrics* metrics) {
  return SpellCheckHost::Create(
      provider, language, request_context, metrics);
}

bool SpellCheckProfile::IsTesting() const {
  return false;
}

void SpellCheckProfile::StartRecordingMetrics(bool spellcheck_enabled) {
  metrics_.reset(new SpellCheckHostMetrics());
  metrics_->RecordEnabledStats(spellcheck_enabled);
}

void SpellCheckProfile::SpellCheckHostInitialized(
    SpellCheckProfile::CustomWordList* custom_words) {
  host_ready_ = !!host_.get() && host_->IsReady();
  custom_words_.reset(custom_words);
  if (metrics_.get()) {
    int count = custom_words_.get() ? custom_words_->size() : 0;
    metrics_->RecordCustomWordCountStats(count);
  }
}

const SpellCheckProfile::CustomWordList&
SpellCheckProfile::GetCustomWords() const {
  return custom_words_.get() ? *custom_words_ : g_empty_list.Get();
}

void SpellCheckProfile::CustomWordAddedLocally(const std::string& word) {
  if (!custom_words_.get())
    custom_words_.reset(new CustomWordList());
  custom_words_->push_back(word);
  if (metrics_.get())
    metrics_->RecordCustomWordCountStats(custom_words_->size());
}

void SpellCheckProfile::LoadCustomDictionary(CustomWordList* custom_words) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE) || IsTesting());

  if (!custom_words)
    return;

  std::string contents;
  file_util::ReadFileToString(GetCustomDictionaryPath(), &contents);
  if (contents.empty())
    return;

  CustomWordList list_of_words;
  base::SplitString(contents, '\n', &list_of_words);
  for (size_t i = 0; i < list_of_words.size(); ++i) {
    if (list_of_words[i] != "")
      custom_words->push_back(list_of_words[i]);
  }
}

void SpellCheckProfile::WriteWordToCustomDictionary(const std::string& word) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE) || IsTesting());

  // Stored in UTF-8.
  DCHECK(IsStringUTF8(word));

  std::string word_to_add(word + "\n");
  FILE* f = file_util::OpenFile(GetCustomDictionaryPath(), "a+");
  if (f) {
    fputs(word_to_add.c_str(), f);
    file_util::CloseFile(f);
  }
}

const FilePath& SpellCheckProfile::GetCustomDictionaryPath() {
  if (!custom_dictionary_path_.get()) {
    custom_dictionary_path_.reset(
        new FilePath(profile_dir_.Append(chrome::kCustomDictionaryFileName)));
  }

  return *custom_dictionary_path_;
}
