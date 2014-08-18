// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autocomplete/base_search_provider.h"

#include "base/i18n/case_conversion.h"
#include "base/prefs/pref_registry_simple.h"
#include "base/prefs/pref_service.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_scheme_classifier.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service_factory.h"
#include "chrome/browser/history/history_service.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/common/pref_names.h"
#include "components/metrics/proto/omnibox_event.pb.h"
#include "components/metrics/proto/omnibox_input_type.pb.h"
#include "components/omnibox/autocomplete_provider_listener.h"
#include "components/omnibox/omnibox_field_trial.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"
#include "components/sync_driver/sync_prefs.h"
#include "net/base/escape.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

using metrics::OmniboxEventProto;

// SuggestionDeletionHandler -------------------------------------------------

// This class handles making requests to the server in order to delete
// personalized suggestions.
class SuggestionDeletionHandler : public net::URLFetcherDelegate {
 public:
  typedef base::Callback<void(bool, SuggestionDeletionHandler*)>
      DeletionCompletedCallback;

  SuggestionDeletionHandler(
      const std::string& deletion_url,
      Profile* profile,
      const DeletionCompletedCallback& callback);

  virtual ~SuggestionDeletionHandler();

 private:
  // net::URLFetcherDelegate:
  virtual void OnURLFetchComplete(const net::URLFetcher* source) OVERRIDE;

  scoped_ptr<net::URLFetcher> deletion_fetcher_;
  DeletionCompletedCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(SuggestionDeletionHandler);
};

SuggestionDeletionHandler::SuggestionDeletionHandler(
    const std::string& deletion_url,
    Profile* profile,
    const DeletionCompletedCallback& callback) : callback_(callback) {
  GURL url(deletion_url);
  DCHECK(url.is_valid());

  deletion_fetcher_.reset(net::URLFetcher::Create(
      BaseSearchProvider::kDeletionURLFetcherID,
      url,
      net::URLFetcher::GET,
      this));
  deletion_fetcher_->SetRequestContext(profile->GetRequestContext());
  deletion_fetcher_->Start();
}

SuggestionDeletionHandler::~SuggestionDeletionHandler() {
}

void SuggestionDeletionHandler::OnURLFetchComplete(
    const net::URLFetcher* source) {
  DCHECK(source == deletion_fetcher_.get());
  callback_.Run(
      source->GetStatus().is_success() && (source->GetResponseCode() == 200),
      this);
}

// BaseSearchProvider ---------------------------------------------------------

// static
const int BaseSearchProvider::kDefaultProviderURLFetcherID = 1;
const int BaseSearchProvider::kKeywordProviderURLFetcherID = 2;
const int BaseSearchProvider::kDeletionURLFetcherID = 3;

BaseSearchProvider::BaseSearchProvider(TemplateURLService* template_url_service,
                                       Profile* profile,
                                       AutocompleteProvider::Type type)
    : AutocompleteProvider(type),
      template_url_service_(template_url_service),
      profile_(profile),
      field_trial_triggered_(false),
      field_trial_triggered_in_session_(false) {
}

// static
bool BaseSearchProvider::ShouldPrefetch(const AutocompleteMatch& match) {
  return match.GetAdditionalInfo(kShouldPrefetchKey) == kTrue;
}

// static
AutocompleteMatch BaseSearchProvider::CreateSearchSuggestion(
    const base::string16& suggestion,
    AutocompleteMatchType::Type type,
    bool from_keyword_provider,
    const TemplateURL* template_url,
    const SearchTermsData& search_terms_data) {
  // This call uses a number of default values.  For instance, it assumes that
  // if this match is from a keyword provider than the user is in keyword mode.
  return CreateSearchSuggestion(
      NULL, AutocompleteInput(), from_keyword_provider,
      SearchSuggestionParser::SuggestResult(
          suggestion, type, suggestion, base::string16(), base::string16(),
          base::string16(), base::string16(), std::string(), std::string(),
          from_keyword_provider, 0, false, false, base::string16()),
      template_url, search_terms_data, 0, false);
}

void BaseSearchProvider::DeleteMatch(const AutocompleteMatch& match) {
  DCHECK(match.deletable);
  if (!match.GetAdditionalInfo(BaseSearchProvider::kDeletionUrlKey).empty()) {
    deletion_handlers_.push_back(new SuggestionDeletionHandler(
        match.GetAdditionalInfo(BaseSearchProvider::kDeletionUrlKey),
        profile_,
        base::Bind(&BaseSearchProvider::OnDeletionComplete,
                   base::Unretained(this))));
  }

  HistoryService* const history_service =
      HistoryServiceFactory::GetForProfile(profile_, Profile::EXPLICIT_ACCESS);
  TemplateURL* template_url =
      match.GetTemplateURL(template_url_service_, false);
  // This may be NULL if the template corresponding to the keyword has been
  // deleted or there is no keyword set.
  if (template_url != NULL) {
    history_service->DeleteMatchingURLsForKeyword(template_url->id(),
                                                  match.contents);
  }

  // Immediately update the list of matches to show the match was deleted,
  // regardless of whether the server request actually succeeds.
  DeleteMatchFromMatches(match);
}

void BaseSearchProvider::AddProviderInfo(ProvidersInfo* provider_info) const {
  provider_info->push_back(metrics::OmniboxEventProto_ProviderInfo());
  metrics::OmniboxEventProto_ProviderInfo& new_entry = provider_info->back();
  new_entry.set_provider(AsOmniboxEventProviderType());
  new_entry.set_provider_done(done_);
  std::vector<uint32> field_trial_hashes;
  OmniboxFieldTrial::GetActiveSuggestFieldTrialHashes(&field_trial_hashes);
  for (size_t i = 0; i < field_trial_hashes.size(); ++i) {
    if (field_trial_triggered_)
      new_entry.mutable_field_trial_triggered()->Add(field_trial_hashes[i]);
    if (field_trial_triggered_in_session_) {
      new_entry.mutable_field_trial_triggered_in_session()->Add(
          field_trial_hashes[i]);
    }
  }
}

// static
const char BaseSearchProvider::kRelevanceFromServerKey[] =
    "relevance_from_server";
const char BaseSearchProvider::kShouldPrefetchKey[] = "should_prefetch";
const char BaseSearchProvider::kSuggestMetadataKey[] = "suggest_metadata";
const char BaseSearchProvider::kDeletionUrlKey[] = "deletion_url";
const char BaseSearchProvider::kTrue[] = "true";
const char BaseSearchProvider::kFalse[] = "false";

BaseSearchProvider::~BaseSearchProvider() {}

void BaseSearchProvider::SetDeletionURL(const std::string& deletion_url,
                                        AutocompleteMatch* match) {
  if (deletion_url.empty())
    return;
  if (!template_url_service_)
    return;
  GURL url =
      template_url_service_->GetDefaultSearchProvider()->GenerateSearchURL(
          template_url_service_->search_terms_data());
  url = url.GetOrigin().Resolve(deletion_url);
  if (url.is_valid()) {
    match->RecordAdditionalInfo(BaseSearchProvider::kDeletionUrlKey,
        url.spec());
    match->deletable = true;
  }
}

// static
AutocompleteMatch BaseSearchProvider::CreateSearchSuggestion(
    AutocompleteProvider* autocomplete_provider,
    const AutocompleteInput& input,
    const bool in_keyword_mode,
    const SearchSuggestionParser::SuggestResult& suggestion,
    const TemplateURL* template_url,
    const SearchTermsData& search_terms_data,
    int accepted_suggestion,
    bool append_extra_query_params) {
  AutocompleteMatch match(autocomplete_provider, suggestion.relevance(), false,
                          suggestion.type());

  if (!template_url)
    return match;
  match.keyword = template_url->keyword();
  match.contents = suggestion.match_contents();
  match.contents_class = suggestion.match_contents_class();
  match.answer_contents = suggestion.answer_contents();
  match.answer_type = suggestion.answer_type();
  if (suggestion.type() == AutocompleteMatchType::SEARCH_SUGGEST_INFINITE) {
    match.RecordAdditionalInfo(
        kACMatchPropertyInputText, base::UTF16ToUTF8(input.text()));
    match.RecordAdditionalInfo(
        kACMatchPropertyContentsPrefix,
        base::UTF16ToUTF8(suggestion.match_contents_prefix()));
    match.RecordAdditionalInfo(
        kACMatchPropertyContentsStartIndex,
        static_cast<int>(
            suggestion.suggestion().length() - match.contents.length()));
  }

  if (!suggestion.annotation().empty())
    match.description = suggestion.annotation();

  // suggestion.match_contents() should have already been collapsed.
  match.allowed_to_be_default_match =
      (!in_keyword_mode || suggestion.from_keyword_provider()) &&
      (base::CollapseWhitespace(input.text(), false) ==
       suggestion.match_contents());

  // When the user forced a query, we need to make sure all the fill_into_edit
  // values preserve that property.  Otherwise, if the user starts editing a
  // suggestion, non-Search results will suddenly appear.
  if (input.type() == metrics::OmniboxInputType::FORCED_QUERY)
    match.fill_into_edit.assign(base::ASCIIToUTF16("?"));
  if (suggestion.from_keyword_provider())
    match.fill_into_edit.append(match.keyword + base::char16(' '));
  if (!input.prevent_inline_autocomplete() &&
      (!in_keyword_mode || suggestion.from_keyword_provider()) &&
      StartsWith(suggestion.suggestion(), input.text(), false)) {
    match.inline_autocompletion =
        suggestion.suggestion().substr(input.text().length());
    match.allowed_to_be_default_match = true;
  }
  match.fill_into_edit.append(suggestion.suggestion());

  const TemplateURLRef& search_url = template_url->url_ref();
  DCHECK(search_url.SupportsReplacement(search_terms_data));
  match.search_terms_args.reset(
      new TemplateURLRef::SearchTermsArgs(suggestion.suggestion()));
  match.search_terms_args->original_query = input.text();
  match.search_terms_args->accepted_suggestion = accepted_suggestion;
  match.search_terms_args->enable_omnibox_start_margin = true;
  match.search_terms_args->suggest_query_params =
      suggestion.suggest_query_params();
  match.search_terms_args->append_extra_query_params =
      append_extra_query_params;
  // This is the destination URL sans assisted query stats.  This must be set
  // so the AutocompleteController can properly de-dupe; the controller will
  // eventually overwrite it before it reaches the user.
  match.destination_url =
      GURL(search_url.ReplaceSearchTerms(*match.search_terms_args.get(),
                                         search_terms_data));

  // Search results don't look like URLs.
  match.transition = suggestion.from_keyword_provider() ?
      content::PAGE_TRANSITION_KEYWORD : content::PAGE_TRANSITION_GENERATED;

  return match;
}

// static
bool BaseSearchProvider::ZeroSuggestEnabled(
    const GURL& suggest_url,
    const TemplateURL* template_url,
    OmniboxEventProto::PageClassification page_classification,
    const SearchTermsData& search_terms_data,
    Profile* profile) {
  if (!OmniboxFieldTrial::InZeroSuggestFieldTrial())
    return false;

  // Make sure we are sending the suggest request through HTTPS to prevent
  // exposing the current page URL or personalized results without encryption.
  if (!suggest_url.SchemeIs(url::kHttpsScheme))
    return false;

  // Don't show zero suggest on the NTP.
  // TODO(hfung): Experiment with showing MostVisited zero suggest on NTP
  // under the conditions described in crbug.com/305366.
  if ((page_classification ==
       OmniboxEventProto::INSTANT_NTP_WITH_FAKEBOX_AS_STARTING_FOCUS) ||
      (page_classification ==
       OmniboxEventProto::INSTANT_NTP_WITH_OMNIBOX_AS_STARTING_FOCUS))
    return false;

  // Don't run if there's no profile or in incognito mode.
  if (profile == NULL || profile->IsOffTheRecord())
    return false;

  // Don't run if we can't get preferences or search suggest is not enabled.
  PrefService* prefs = profile->GetPrefs();
  if (!prefs->GetBoolean(prefs::kSearchSuggestEnabled))
    return false;

  // Only make the request if we know that the provider supports zero suggest
  // (currently only the prepopulated Google provider).
  if (template_url == NULL ||
      !template_url->SupportsReplacement(search_terms_data) ||
      TemplateURLPrepopulateData::GetEngineType(
          *template_url, search_terms_data) != SEARCH_ENGINE_GOOGLE)
    return false;

  return true;
}

// static
bool BaseSearchProvider::CanSendURL(
    const GURL& current_page_url,
    const GURL& suggest_url,
    const TemplateURL* template_url,
    OmniboxEventProto::PageClassification page_classification,
    const SearchTermsData& search_terms_data,
    Profile* profile) {
  if (!ZeroSuggestEnabled(suggest_url, template_url, page_classification,
                          search_terms_data, profile))
    return false;

  if (!current_page_url.is_valid())
    return false;

  // Only allow HTTP URLs or HTTPS URLs for the same domain as the search
  // provider.
  if ((current_page_url.scheme() != url::kHttpScheme) &&
      ((current_page_url.scheme() != url::kHttpsScheme) ||
       !net::registry_controlled_domains::SameDomainOrHost(
           current_page_url, suggest_url,
           net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES)))
    return false;

  // Check field trials and settings allow sending the URL on suggest requests.
  ProfileSyncService* service =
      ProfileSyncServiceFactory::GetInstance()->GetForProfile(profile);
  sync_driver::SyncPrefs sync_prefs(profile->GetPrefs());
  if (service == NULL ||
      !service->IsSyncEnabledAndLoggedIn() ||
      !sync_prefs.GetPreferredDataTypes(syncer::UserTypes()).Has(
          syncer::PROXY_TABS) ||
      service->GetEncryptedDataTypes().Has(syncer::SESSIONS))
    return false;

  return true;
}

void BaseSearchProvider::AddMatchToMap(
    const SearchSuggestionParser::SuggestResult& result,
    const std::string& metadata,
    int accepted_suggestion,
    bool mark_as_deletable,
    bool in_keyword_mode,
    MatchMap* map) {
  AutocompleteMatch match = CreateSearchSuggestion(
      this, GetInput(result.from_keyword_provider()), in_keyword_mode, result,
      GetTemplateURL(result.from_keyword_provider()),
      template_url_service_->search_terms_data(), accepted_suggestion,
      ShouldAppendExtraParams(result));
  if (!match.destination_url.is_valid())
    return;
  match.search_terms_args->bookmark_bar_pinned =
      profile_->GetPrefs()->GetBoolean(prefs::kShowBookmarkBar);
  match.RecordAdditionalInfo(kRelevanceFromServerKey,
                             result.relevance_from_server() ? kTrue : kFalse);
  match.RecordAdditionalInfo(kShouldPrefetchKey,
                             result.should_prefetch() ? kTrue : kFalse);
  SetDeletionURL(result.deletion_url(), &match);
  if (mark_as_deletable)
    match.deletable = true;
  // Metadata is needed only for prefetching queries.
  if (result.should_prefetch())
    match.RecordAdditionalInfo(kSuggestMetadataKey, metadata);

  // Try to add |match| to |map|.  If a match for this suggestion is
  // already in |map|, replace it if |match| is more relevant.
  // NOTE: Keep this ToLower() call in sync with url_database.cc.
  MatchKey match_key(
      std::make_pair(base::i18n::ToLower(result.suggestion()),
                     match.search_terms_args->suggest_query_params));
  const std::pair<MatchMap::iterator, bool> i(
       map->insert(std::make_pair(match_key, match)));

  bool should_prefetch = result.should_prefetch();
  if (!i.second) {
    // NOTE: We purposefully do a direct relevance comparison here instead of
    // using AutocompleteMatch::MoreRelevant(), so that we'll prefer "items
    // added first" rather than "items alphabetically first" when the scores
    // are equal. The only case this matters is when a user has results with
    // the same score that differ only by capitalization; because the history
    // system returns results sorted by recency, this means we'll pick the most
    // recent such result even if the precision of our relevance score is too
    // low to distinguish the two.
    if (match.relevance > i.first->second.relevance) {
      match.duplicate_matches.insert(match.duplicate_matches.end(),
                                     i.first->second.duplicate_matches.begin(),
                                     i.first->second.duplicate_matches.end());
      i.first->second.duplicate_matches.clear();
      match.duplicate_matches.push_back(i.first->second);
      i.first->second = match;
    } else {
      i.first->second.duplicate_matches.push_back(match);
      if (match.keyword == i.first->second.keyword) {
        // Old and new matches are from the same search provider. It is okay to
        // record one match's prefetch data onto a different match (for the same
        // query string) for the following reasons:
        // 1. Because the suggest server only sends down a query string from
        // which we construct a URL, rather than sending a full URL, and because
        // we construct URLs from query strings in the same way every time, the
        // URLs for the two matches will be the same. Therefore, we won't end up
        // prefetching something the server didn't intend.
        // 2. Presumably the server sets the prefetch bit on a match it things
        // is sufficiently relevant that the user is likely to choose it.
        // Surely setting the prefetch bit on a match of even higher relevance
        // won't violate this assumption.
        should_prefetch |= ShouldPrefetch(i.first->second);
        i.first->second.RecordAdditionalInfo(kShouldPrefetchKey,
                                             should_prefetch ? kTrue : kFalse);
        if (should_prefetch)
          i.first->second.RecordAdditionalInfo(kSuggestMetadataKey, metadata);
      }
    }
  }
}

bool BaseSearchProvider::ParseSuggestResults(
    const base::Value& root_val,
    int default_result_relevance,
    bool is_keyword_result,
    SearchSuggestionParser::Results* results) {
  if (!SearchSuggestionParser::ParseSuggestResults(
      root_val, GetInput(is_keyword_result),
      ChromeAutocompleteSchemeClassifier(profile_), default_result_relevance,
      profile_->GetPrefs()->GetString(prefs::kAcceptLanguages),
      is_keyword_result, results))
    return false;

  BitmapFetcherService* image_service =
      BitmapFetcherServiceFactory::GetForBrowserContext(profile_);
  DCHECK(image_service);
  for (std::vector<GURL>::const_iterator it =
           results->answers_image_urls.begin();
       it != results->answers_image_urls.end(); ++it)
    image_service->Prefetch(*it);

  field_trial_triggered_ |= results->field_trial_triggered;
  field_trial_triggered_in_session_ |= results->field_trial_triggered;
  return true;
}

void BaseSearchProvider::DeleteMatchFromMatches(
    const AutocompleteMatch& match) {
  for (ACMatches::iterator i(matches_.begin()); i != matches_.end(); ++i) {
    // Find the desired match to delete by checking the type and contents.
    // We can't check the destination URL, because the autocomplete controller
    // may have reformulated that. Not that while checking for matching
    // contents works for personalized suggestions, if more match types gain
    // deletion support, this algorithm may need to be re-examined.
    if (i->contents == match.contents && i->type == match.type) {
      matches_.erase(i);
      break;
    }
  }
}

void BaseSearchProvider::OnDeletionComplete(
    bool success, SuggestionDeletionHandler* handler) {
  RecordDeletionResult(success);
  SuggestionDeletionHandlers::iterator it = std::find(
      deletion_handlers_.begin(), deletion_handlers_.end(), handler);
  DCHECK(it != deletion_handlers_.end());
  deletion_handlers_.erase(it);
}
