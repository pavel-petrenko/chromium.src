// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CORE_BROWSER_HISTORY_SERVICE_H_
#define COMPONENTS_HISTORY_CORE_BROWSER_HISTORY_SERVICE_H_

#include <set>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_list.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/strings/string16.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "components/favicon_base/favicon_callback.h"
#include "components/favicon_base/favicon_usage_data.h"
#include "components/history/core/browser/delete_directive_handler.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/keyword_id.h"
#include "components/history/core/browser/typed_url_syncable_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "sql/init_status.h"
#include "sync/api/syncable_service.h"
#include "ui/base/page_transition_types.h"

class GURL;
class HistoryQuickProviderTest;
class HistoryURLProvider;
class HistoryURLProviderTest;
class InMemoryURLIndexTest;
class SkBitmap;
class SyncBookmarkDataTypeControllerTest;
class TestingProfile;

namespace base {
class FilePath;
class Thread;
}

namespace favicon {
class FaviconService;
}

namespace history {

struct DownloadRow;
struct HistoryAddPageArgs;
class HistoryBackend;
class HistoryClient;
class HistoryDBTask;
class HistoryDatabase;
struct HistoryDatabaseParams;
class HistoryQueryTest;
class HistoryServiceObserver;
class HistoryServiceTest;
class InMemoryHistoryBackend;
struct KeywordSearchTermVisit;
class PageUsageData;
class URLDatabase;
class VisitDelegate;
class VisitFilter;
class WebHistoryService;

// The history service records page titles, and visit times, as well as
// (eventually) information about autocomplete.
//
// This service is thread safe. Each request callback is invoked in the
// thread that made the request.
class HistoryService : public syncer::SyncableService, public KeyedService {
 public:
  // Miscellaneous commonly-used types.
  typedef std::vector<PageUsageData*> PageUsageDataList;

  // Callback for value asynchronously returned by TopHosts().
  typedef base::Callback<void(const TopHostsList&)> TopHostsCallback;

  // Must call Init after construction. The empty constructor provided only for
  // unit tests. When using the full constructor, |history_client| may only be
  // null during testing, while |visit_delegate| may be null if the embedder use
  // another way to track visited links.
  HistoryService();
  HistoryService(scoped_ptr<HistoryClient> history_client,
                 scoped_ptr<VisitDelegate> visit_delegate);
  ~HistoryService() override;

  // Initializes the history service, returning true on success. On false, do
  // not call any other functions. The given directory will be used for storing
  // the history files. |languages| is a comma-separated list of languages to
  // use when interpreting URLs, it must not be empty (except during testing).
  bool Init(const std::string& languages,
            const HistoryDatabaseParams& history_database_params) {
    return Init(false, languages, history_database_params);
  }

  // Triggers the backend to load if it hasn't already, and then returns whether
  // it's finished loading.
  // Note: Virtual needed for mocking.
  virtual bool BackendLoaded();

  // Returns true if the backend has finished loading.
  bool backend_loaded() const { return backend_loaded_; }

#if defined(OS_IOS)
  // Causes the history backend to commit any in-progress transactions. Called
  // when the application is being backgrounded.
  void HandleBackgrounding();
#endif

  // Context ids are used to scope page IDs (see AddPage). These contexts
  // must tell us when they are being invalidated so that we can clear
  // out any cached data associated with that context.
  void ClearCachedDataForContextID(ContextID context_id);

  // Triggers the backend to load if it hasn't already, and then returns the
  // in-memory URL database. The returned pointer may be null if the in-memory
  // database has not been loaded yet. This pointer is owned by the history
  // system. Callers should not store or cache this value.
  //
  // TODO(brettw) this should return the InMemoryHistoryBackend.
  URLDatabase* InMemoryDatabase();

  // Following functions get URL information from in-memory database.
  // They return false if database is not available (e.g. not loaded yet) or the
  // URL does not exist.

  // Reads the number of times the user has typed the given URL.
  bool GetTypedCountForURL(const GURL& url, int* typed_count);

  // Reads the last visit time for the given URL.
  bool GetLastVisitTimeForURL(const GURL& url, base::Time* last_visit);

  // Reads the number of times this URL has been visited.
  bool GetVisitCountForURL(const GURL& url, int* visit_count);

  // Returns a pointer to the TypedUrlSyncableService owned by HistoryBackend.
  // This method should only be called from the history thread, because the
  // returned service is intended to be accessed only via the history thread.
  TypedUrlSyncableService* GetTypedUrlSyncableService() const;

  // KeyedService:
  void Shutdown() override;

  // Computes the |num_hosts| most-visited hostnames in the past 30 days and
  // returns a list of those hosts paired with their visit counts. The following
  // caveats apply:
  // 1. Hostnames are stripped of their 'www.' prefix. Visits to foo.com and
  //    www.foo.com are summed into the resultant foo.com entry.
  // 2. Ports and schemes are ignored. Visits to http://foo.com/ and
  //    https://foo.com:567/ are summed into the resultant foo.com entry.
  // 3. If the history is abnormally large and diverse, the function will give
  //    up early and return an approximate list.
  // 4. Only http://, https://, and ftp:// URLs are counted.
  //
  // Note: Virtual needed for mocking.
  virtual void TopHosts(int num_hosts, const TopHostsCallback& callback) const;

  // Navigation ----------------------------------------------------------------

  // Adds the given canonical URL to history with the given time as the visit
  // time. Referrer may be the empty string.
  //
  // The supplied context id is used to scope the given page ID. Page IDs
  // are only unique inside a given context, so we need that to differentiate
  // them.
  //
  // The context/page ids can be null if there is no meaningful tracking
  // information that can be performed on the given URL. The 'nav_entry_id'
  // should be the unique ID of the current navigation entry in the given
  // process.
  //
  // 'redirects' is an array of redirect URLs leading to this page, with the
  // page itself as the last item (so when there is no redirect, it will have
  // one entry). If there are no redirects, this array may also be empty for
  // the convenience of callers.
  //
  // 'did_replace_entry' is true when the navigation entry for this page has
  // replaced the existing entry. A non-user initiated redirect causes such
  // replacement.
  //
  // All "Add Page" functions will update the visited link database.
  void AddPage(const GURL& url,
               base::Time time,
               ContextID context_id,
               int nav_entry_id,
               const GURL& referrer,
               const RedirectList& redirects,
               ui::PageTransition transition,
               VisitSource visit_source,
               bool did_replace_entry);

  // For adding pages to history where no tracking information can be done.
  void AddPage(const GURL& url, base::Time time, VisitSource visit_source);

  // All AddPage variants end up here.
  void AddPage(const HistoryAddPageArgs& add_page_args);

  // Adds an entry for the specified url without creating a visit. This should
  // only be used when bookmarking a page, otherwise the row leaks in the
  // history db (it never gets cleaned).
  void AddPageNoVisitForBookmark(const GURL& url, const base::string16& title);

  // Sets the title for the given page. The page should be in history. If it
  // is not, this operation is ignored.
  void SetPageTitle(const GURL& url, const base::string16& title);

  // Updates the history database with a page's ending time stamp information.
  // The page can be identified by the combination of the context id, the
  // navigation entry id and the url.
  void UpdateWithPageEndTime(ContextID context_id,
                             int nav_entry_id,
                             const GURL& url,
                             base::Time end_ts);

  // Querying ------------------------------------------------------------------

  // Returns the information about the requested URL. If the URL is found,
  // success will be true and the information will be in the URLRow parameter.
  // On success, the visits, if requested, will be sorted by date. If they have
  // not been requested, the pointer will be valid, but the vector will be
  // empty.
  //
  // If success is false, neither the row nor the vector will be valid.
  typedef base::Callback<void(
      bool,  // Success flag, when false, nothing else is valid.
      const URLRow&,
      const VisitVector&)> QueryURLCallback;

  // Queries the basic information about the URL in the history database. If
  // the caller is interested in the visits (each time the URL is visited),
  // set |want_visits| to true. If these are not needed, the function will be
  // faster by setting this to false.
  base::CancelableTaskTracker::TaskId QueryURL(
      const GURL& url,
      bool want_visits,
      const QueryURLCallback& callback,
      base::CancelableTaskTracker* tracker);

  // Provides the result of a query. See QueryResults in history_types.h.
  // The common use will be to use QueryResults.Swap to suck the contents of
  // the results out of the passed in parameter and take ownership of them.
  typedef base::Callback<void(QueryResults*)> QueryHistoryCallback;

  // Queries all history with the given options (see QueryOptions in
  // history_types.h).  If empty, all results matching the given options
  // will be returned.
  base::CancelableTaskTracker::TaskId QueryHistory(
      const base::string16& text_query,
      const QueryOptions& options,
      const QueryHistoryCallback& callback,
      base::CancelableTaskTracker* tracker);

  // Called when the results of QueryRedirectsFrom are available.
  // The given vector will contain a list of all redirects, not counting
  // the original page. If A redirects to B which redirects to C, the vector
  // will contain [B, C], and A will be in 'from_url'.
  //
  // For QueryRedirectsTo, the order is reversed. For A->B->C, the vector will
  // contain [B, A] and C will be in 'to_url'.
  //
  // If there is no such URL in the database or the most recent visit has no
  // redirect, the vector will be empty. If the given page has redirected to
  // multiple destinations, this will pick a random one.
  typedef base::Callback<void(const RedirectList*)> QueryRedirectsCallback;

  // Schedules a query for the most recent redirect coming out of the given
  // URL. See the RedirectQuerySource above, which is guaranteed to be called
  // if the request is not canceled.
  base::CancelableTaskTracker::TaskId QueryRedirectsFrom(
      const GURL& from_url,
      const QueryRedirectsCallback& callback,
      base::CancelableTaskTracker* tracker);

  // Schedules a query to get the most recent redirects ending at the given
  // URL.
  base::CancelableTaskTracker::TaskId QueryRedirectsTo(
      const GURL& to_url,
      const QueryRedirectsCallback& callback,
      base::CancelableTaskTracker* tracker);

  // Requests the number of user-visible visits (i.e. no redirects or subframes)
  // to all urls on the same scheme/host/port as |url|.  This is only valid for
  // HTTP and HTTPS URLs.
  typedef base::Callback<void(
      bool,         // Were we able to determine the # of visits?
      int,          // Number of visits.
      base::Time)>  // Time of first visit. Only set if bool
                    // is true and int is > 0.
      GetVisibleVisitCountToHostCallback;

  base::CancelableTaskTracker::TaskId GetVisibleVisitCountToHost(
      const GURL& url,
      const GetVisibleVisitCountToHostCallback& callback,
      base::CancelableTaskTracker* tracker);

  // Request the |result_count| most visited URLs and the chain of
  // redirects leading to each of these URLs. |days_back| is the
  // number of days of history to use. Used by TopSites.
  typedef base::Callback<void(const MostVisitedURLList*)>
      QueryMostVisitedURLsCallback;

  base::CancelableTaskTracker::TaskId QueryMostVisitedURLs(
      int result_count,
      int days_back,
      const QueryMostVisitedURLsCallback& callback,
      base::CancelableTaskTracker* tracker);

  // Request the |result_count| URLs filtered and sorted based on the |filter|.
  // If |extended_info| is true, additional data will be provided in the
  // results. Computing this additional data is expensive, likely to become
  // more expensive as additional data points are added in future changes, and
  // not useful in most cases. Set |extended_info| to true only if you
  // explicitly require the additional data.
  typedef base::Callback<void(const FilteredURLList*)>
      QueryFilteredURLsCallback;

  base::CancelableTaskTracker::TaskId QueryFilteredURLs(
      int result_count,
      const VisitFilter& filter,
      bool extended_info,
      const QueryFilteredURLsCallback& callback,
      base::CancelableTaskTracker* tracker);

  // Database management operations --------------------------------------------

  // Delete all the information related to a single url.
  void DeleteURL(const GURL& url);

  // Delete all the information related to a list of urls.  (Deleting
  // URLs one by one is slow as it has to flush to disk each time.)
  void DeleteURLsForTest(const std::vector<GURL>& urls);

  // Removes all visits in the selected time range (including the
  // start time), updating the URLs accordingly. This deletes any
  // associated data. This function also deletes the associated
  // favicons, if they are no longer referenced. |callback| runs when
  // the expiration is complete. You may use null Time values to do an
  // unbounded delete in either direction.
  // If |restrict_urls| is not empty, only visits to the URLs in this set are
  // removed.
  void ExpireHistoryBetween(const std::set<GURL>& restrict_urls,
                            base::Time begin_time,
                            base::Time end_time,
                            const base::Closure& callback,
                            base::CancelableTaskTracker* tracker);

  // Removes all visits to specified URLs in specific time ranges.
  // This is the equivalent ExpireHistoryBetween() once for each element in the
  // vector. The fields of |ExpireHistoryArgs| map directly to the arguments of
  // of ExpireHistoryBetween().
  void ExpireHistory(const std::vector<ExpireHistoryArgs>& expire_list,
                     const base::Closure& callback,
                     base::CancelableTaskTracker* tracker);

  // Removes all visits to the given URLs in the specified time range. Calls
  // ExpireHistoryBetween() to delete local visits, and handles deletion of
  // synced visits if appropriate.
  void ExpireLocalAndRemoteHistoryBetween(WebHistoryService* web_history,
                                          const std::set<GURL>& restrict_urls,
                                          base::Time begin_time,
                                          base::Time end_time,
                                          const base::Closure& callback,
                                          base::CancelableTaskTracker* tracker);

  // Processes the given |delete_directive| and sends it to the
  // SyncChangeProcessor (if it exists).  Returns any error resulting
  // from sending the delete directive to sync.
  syncer::SyncError ProcessLocalDeleteDirective(
      const sync_pb::HistoryDeleteDirectiveSpecifics& delete_directive);

  // Downloads -----------------------------------------------------------------

  // Implemented by the caller of 'CreateDownload' below, and is called when the
  // history service has created a new entry for a download in the history db.
  typedef base::Callback<void(bool)> DownloadCreateCallback;

  // Begins a history request to create a new row for a download. 'info'
  // contains all the download's creation state, and 'callback' runs when the
  // history service request is complete. The callback is called on the thread
  // that calls CreateDownload().
  void CreateDownload(const DownloadRow& info,
                      const DownloadCreateCallback& callback);

  // Implemented by the caller of 'GetNextDownloadId' below, and is called with
  // the maximum id of all downloads records in the database plus 1.
  typedef base::Callback<void(uint32)> DownloadIdCallback;

  // Responds on the calling thread with the maximum id of all downloads records
  // in the database plus 1.
  void GetNextDownloadId(const DownloadIdCallback& callback);

  // Implemented by the caller of 'QueryDownloads' below, and is called when the
  // history service has retrieved a list of all download state. The call
  typedef base::Callback<void(scoped_ptr<std::vector<DownloadRow>>)>
      DownloadQueryCallback;

  // Begins a history request to retrieve the state of all downloads in the
  // history db. 'callback' runs when the history service request is complete,
  // at which point 'info' contains an array of DownloadRow, one per
  // download. The callback is called on the thread that calls QueryDownloads().
  void QueryDownloads(const DownloadQueryCallback& callback);

  // Called to update the history service about the current state of a download.
  // This is a 'fire and forget' query, so just pass the relevant state info to
  // the database with no need for a callback.
  void UpdateDownload(const DownloadRow& data);

  // Permanently remove some downloads from the history system. This is a 'fire
  // and forget' operation.
  void RemoveDownloads(const std::set<uint32>& ids);

  // Keyword search terms -----------------------------------------------------

  // Sets the search terms for the specified url and keyword. url_id gives the
  // id of the url, keyword_id the id of the keyword and term the search term.
  void SetKeywordSearchTermsForURL(const GURL& url,
                                   KeywordID keyword_id,
                                   const base::string16& term);

  // Deletes all search terms for the specified keyword.
  void DeleteAllSearchTermsForKeyword(KeywordID keyword_id);

  // Deletes any search term corresponding to |url|.
  void DeleteKeywordSearchTermForURL(const GURL& url);

  // Deletes all URL and search term entries matching the given |term| and
  // |keyword_id|.
  void DeleteMatchingURLsForKeyword(KeywordID keyword_id,
                                    const base::string16& term);

  // Bookmarks -----------------------------------------------------------------

  // Notification that a URL is no longer bookmarked.
  void URLsNoLongerBookmarked(const std::set<GURL>& urls);

  // Observers -----------------------------------------------------------------

  // Adds/Removes an Observer.
  void AddObserver(HistoryServiceObserver* observer);
  void RemoveObserver(HistoryServiceObserver* observer);

  // Generic Stuff -------------------------------------------------------------

  // Schedules a HistoryDBTask for running on the history backend thread. See
  // HistoryDBTask for details on what this does. Takes ownership of |task|.
  virtual base::CancelableTaskTracker::TaskId ScheduleDBTask(
      scoped_ptr<HistoryDBTask> task,
      base::CancelableTaskTracker* tracker);

  // This callback is invoked when favicon change for urls.
  typedef base::Callback<void(const std::set<GURL>&)> OnFaviconChangedCallback;

  // Add a callback to the list. The callback will remain registered until the
  // returned Subscription is destroyed. This must occurs before HistoryService
  // is destroyed.
  scoped_ptr<base::CallbackList<void(const std::set<GURL>&)>::Subscription>
  AddFaviconChangedCallback(const OnFaviconChangedCallback& callback)
      WARN_UNUSED_RESULT;

  // Testing -------------------------------------------------------------------

  // Runs |flushed| after bouncing off the history thread.
  void FlushForTest(const base::Closure& flushed);

  // Designed for unit tests, this passes the given task on to the history
  // backend to be called once the history backend has terminated. This allows
  // callers to know when the history thread is complete and the database files
  // can be deleted and the next test run. Otherwise, the history thread may
  // still be running, causing problems in subsequent tests.
  //
  // There can be only one closing task, so this will override any previously
  // set task. We will take ownership of the pointer and delete it when done.
  // The task will be run on the calling thread (this function is threadsafe).
  void SetOnBackendDestroyTask(const base::Closure& task);

  // Used for unit testing and potentially importing to get known information
  // into the database. This assumes the URL doesn't exist in the database
  //
  // Calling this function many times may be slow because each call will
  // dispatch to the history thread and will be a separate database
  // transaction. If this functionality is needed for importing many URLs,
  // callers should use AddPagesWithDetails() instead.
  //
  // Note that this routine (and AddPageWithDetails()) always adds a single
  // visit using the |last_visit| timestamp, and a PageTransition type of LINK,
  // if |visit_source| != SYNCED.
  void AddPageWithDetails(const GURL& url,
                          const base::string16& title,
                          int visit_count,
                          int typed_count,
                          base::Time last_visit,
                          bool hidden,
                          VisitSource visit_source);

  // The same as AddPageWithDetails() but takes a vector.
  void AddPagesWithDetails(const URLRows& info, VisitSource visit_source);

  base::WeakPtr<HistoryService> AsWeakPtr();

  // syncer::SyncableService implementation.
  syncer::SyncMergeResult MergeDataAndStartSyncing(
      syncer::ModelType type,
      const syncer::SyncDataList& initial_sync_data,
      scoped_ptr<syncer::SyncChangeProcessor> sync_processor,
      scoped_ptr<syncer::SyncErrorFactory> error_handler) override;
  void StopSyncing(syncer::ModelType type) override;
  syncer::SyncDataList GetAllSyncData(syncer::ModelType type) const override;
  syncer::SyncError ProcessSyncChanges(
      const tracked_objects::Location& from_here,
      const syncer::SyncChangeList& change_list) override;

 protected:
  // These are not currently used, hopefully we can do something in the future
  // to ensure that the most important things happen first.
  enum SchedulePriority {
    PRIORITY_UI,      // The highest priority (must respond to UI events).
    PRIORITY_NORMAL,  // Normal stuff like adding a page.
    PRIORITY_LOW,     // Low priority things like indexing or expiration.
  };

 private:
  class BackendDelegate;
  friend class base::RefCountedThreadSafe<HistoryService>;
  friend class BackendDelegate;
  friend class favicon::FaviconService;
  friend class HistoryBackend;
  friend class HistoryQueryTest;
  friend class ::HistoryQuickProviderTest;
  friend class HistoryServiceTest;
  friend class ::HistoryURLProvider;
  friend class ::HistoryURLProviderTest;
  friend class ::InMemoryURLIndexTest;
  friend class ::SyncBookmarkDataTypeControllerTest;
  friend class ::TestingProfile;

  // Called on shutdown, this will tell the history backend to complete and
  // will release pointers to it. No other functions should be called once
  // cleanup has happened that may dispatch to the history thread (because it
  // will be null).
  //
  // In practice, this will be called by the service manager (BrowserProcess)
  // when it is being destroyed. Because that reference is being destroyed, it
  // should be impossible for anybody else to call the service, even if it is
  // still in memory (pending requests may be holding a reference to us).
  void Cleanup();

  // Low-level Init().  Same as the public version, but adds a |no_db| parameter
  // that is only set by unittests which causes the backend to not init its DB.
  bool Init(bool no_db,
            const std::string& languages,
            const HistoryDatabaseParams& history_database_params);

  // Called by the HistoryURLProvider class to schedule an autocomplete, it
  // will be called back on the internal history thread with the history
  // database so it can query. See history_autocomplete.cc for a diagram.
  void ScheduleAutocomplete(
      const base::Callback<void(HistoryBackend*, URLDatabase*)>& callback);

  // Notification from the backend that it has finished loading. Sends
  // notification (NOTIFY_HISTORY_LOADED) and sets backend_loaded_ to true.
  void OnDBLoaded();

  // Helper function for getting URL information.
  // Reads a URLRow from in-memory database. Returns false if database is not
  // available or the URL does not exist.
  bool GetRowForURL(const GURL& url, URLRow* url_row);

  // Observers ----------------------------------------------------------------

  // Notify all HistoryServiceObservers registered that user is visiting a URL.
  // The |row| ID will be set to the value that is currently in effect in the
  // main history database. |redirects| is the list of redirects leading up to
  // the URL. If we have a redirect chain A -> B -> C and user is visiting C,
  // then |redirects[0]=B| and |redirects[1]=A|. If there are no redirects,
  // |redirects| is an empty vector.
  void NotifyURLVisited(ui::PageTransition transition,
                        const URLRow& row,
                        const RedirectList& redirects,
                        base::Time visit_time);

  // Notify all HistoryServiceObservers registered that URLs have been added or
  // modified. |changed_urls| contains the list of affects URLs.
  void NotifyURLsModified(const URLRows& changed_urls);

  // Notify all HistoryServiceObservers registered that URLs have been deleted.
  // |all_history| is set to true, if all the URLs are deleted.
  //               When set to true, |deleted_rows| and |favicon_urls| are
  //               undefined.
  // |expired| is set to true, if the URL deletion is due to expiration.
  // |deleted_rows| list of the deleted URLs.
  // |favicon_urls| list of favicon URLs that correspond to the deleted URLs.
  void NotifyURLsDeleted(bool all_history,
                         bool expired,
                         const URLRows& deleted_rows,
                         const std::set<GURL>& favicon_urls);

  // Notify all HistoryServiceObservers registered that the
  // HistoryService has finished loading.
  void NotifyHistoryServiceLoaded();

  // Notify all HistoryServiceObservers registered that HistoryService is being
  // deleted.
  void NotifyHistoryServiceBeingDeleted();

  // Notify all HistoryServiceObservers registered that a keyword search term
  // has been updated. |row| contains the URL information for search |term|.
  // |keyword_id| associated with a URL and search term.
  void NotifyKeywordSearchTermUpdated(const URLRow& row,
                                      KeywordID keyword_id,
                                      const base::string16& term);

  // Notify all HistoryServiceObservers registered that keyword search term is
  // deleted. |url_id| is the id of the url row.
  void NotifyKeywordSearchTermDeleted(URLID url_id);

  // Favicon -------------------------------------------------------------------

  // These favicon methods are exposed to the FaviconService. Instead of calling
  // these methods directly you should call the respective method on the
  // FaviconService.

  // Used by FaviconService to get the favicon bitmaps from the history backend
  // whose edge sizes most closely match |desired_sizes| for |icon_types|. If
  // |desired_sizes| has a '0' entry, the largest favicon bitmap for
  // |icon_types| is returned. The returned FaviconBitmapResults will have at
  // most one result for each entry in |desired_sizes|. If a favicon bitmap is
  // determined to be the best candidate for multiple |desired_sizes| there will
  // be fewer results.
  // If |icon_types| has several types, results for only a single type will be
  // returned in the priority of TOUCH_PRECOMPOSED_ICON, TOUCH_ICON, and
  // FAVICON.
  base::CancelableTaskTracker::TaskId GetFavicons(
      const std::vector<GURL>& icon_urls,
      int icon_types,
      const std::vector<int>& desired_sizes,
      const favicon_base::FaviconResultsCallback& callback,
      base::CancelableTaskTracker* tracker);

  // Used by the FaviconService to get favicons mapped to |page_url| for
  // |icon_types| whose edge sizes most closely match |desired_sizes|. If
  // |desired_sizes| has a '0' entry, the largest favicon bitmap for
  // |icon_types| is returned. The returned FaviconBitmapResults will have at
  // most one result for each entry in |desired_sizes|. If a favicon bitmap is
  // determined to be the best candidate for multiple |desired_sizes| there
  // will be fewer results. If |icon_types| has several types, results for only
  // a single type will be returned in the priority of TOUCH_PRECOMPOSED_ICON,
  // TOUCH_ICON, and FAVICON.
  base::CancelableTaskTracker::TaskId GetFaviconsForURL(
      const GURL& page_url,
      int icon_types,
      const std::vector<int>& desired_sizes,
      const favicon_base::FaviconResultsCallback& callback,
      base::CancelableTaskTracker* tracker);

  // Used by FaviconService to find the first favicon bitmap whose width and
  // height are greater than that of |minimum_size_in_pixels|. This searches
  // for icons by IconType. Each element of |icon_types| is a bitmask of
  // IconTypes indicating the types to search for.
  // If the largest icon of |icon_types[0]| is not larger than
  // |minimum_size_in_pixel|, the next icon types of
  // |icon_types| will be searched and so on.
  // If no icon is larger than |minimum_size_in_pixel|, the largest one of all
  // icon types in |icon_types| is returned.
  // This feature is especially useful when some types of icon is preferred as
  // long as its size is larger than a specific value.
  base::CancelableTaskTracker::TaskId GetLargestFaviconForURL(
      const GURL& page_url,
      const std::vector<int>& icon_types,
      int minimum_size_in_pixels,
      const favicon_base::FaviconRawBitmapCallback& callback,
      base::CancelableTaskTracker* tracker);

  // Used by the FaviconService to get the favicon bitmap which most closely
  // matches |desired_size| from the favicon with |favicon_id| from the history
  // backend. If |desired_size| is 0, the largest favicon bitmap for
  // |favicon_id| is returned.
  base::CancelableTaskTracker::TaskId GetFaviconForID(
      favicon_base::FaviconID favicon_id,
      int desired_size,
      const favicon_base::FaviconResultsCallback& callback,
      base::CancelableTaskTracker* tracker);

  // Used by the FaviconService to replace the favicon mappings to |page_url|
  // for |icon_types| on the history backend.
  // Sample |icon_urls|:
  //  { ICON_URL1 -> TOUCH_ICON, known to the database,
  //    ICON_URL2 -> TOUCH_ICON, not known to the database,
  //    ICON_URL3 -> TOUCH_PRECOMPOSED_ICON, known to the database }
  // The new mappings are computed from |icon_urls| with these rules:
  // 1) Any urls in |icon_urls| which are not already known to the database are
  //    rejected.
  //    Sample new mappings to |page_url|: { ICON_URL1, ICON_URL3 }
  // 2) If |icon_types| has multiple types, the mappings are only set for the
  //    largest icon type.
  //    Sample new mappings to |page_url|: { ICON_URL3 }
  // |icon_types| can only have multiple IconTypes if
  // |icon_types| == TOUCH_ICON | TOUCH_PRECOMPOSED_ICON.
  // The favicon bitmaps whose edge sizes most closely match |desired_sizes|
  // from the favicons which were just mapped to |page_url| are returned. If
  // |desired_sizes| has a '0' entry, the largest favicon bitmap is returned.
  base::CancelableTaskTracker::TaskId UpdateFaviconMappingsAndFetch(
      const GURL& page_url,
      const std::vector<GURL>& icon_urls,
      int icon_types,
      const std::vector<int>& desired_sizes,
      const favicon_base::FaviconResultsCallback& callback,
      base::CancelableTaskTracker* tracker);

  // Used by FaviconService to set a favicon for |page_url| and |icon_url| with
  // |pixel_size|.
  // Example:
  //   |page_url|: www.google.com
  // 2 favicons in history for |page_url|:
  //   www.google.com/a.ico  16x16
  //   www.google.com/b.ico  32x32
  // MergeFavicon(|page_url|, www.google.com/a.ico, ..., ..., 16x16)
  //
  // Merging occurs in the following manner:
  // 1) |page_url| is set to map to only to |icon_url|. In order to not lose
  //    data, favicon bitmaps mapped to |page_url| but not to |icon_url| are
  //    copied to the favicon at |icon_url|.
  //    For the example above, |page_url| will only be mapped to a.ico.
  //    The 32x32 favicon bitmap at b.ico is copied to a.ico
  // 2) |bitmap_data| is added to the favicon at |icon_url|, overwriting any
  //    favicon bitmaps of |pixel_size|.
  //    For the example above, |bitmap_data| overwrites the 16x16 favicon
  //    bitmap for a.ico.
  // TODO(pkotwicz): Remove once no longer required by sync.
  void MergeFavicon(const GURL& page_url,
                    const GURL& icon_url,
                    favicon_base::IconType icon_type,
                    scoped_refptr<base::RefCountedMemory> bitmap_data,
                    const gfx::Size& pixel_size);

  // Used by the FaviconService to replace all of the favicon bitmaps mapped to
  // |page_url| for |icon_type|.
  // Use MergeFavicon() if |bitmaps| is incomplete, and favicon bitmaps in the
  // database should be preserved if possible. For instance, favicon bitmaps
  // from sync are 1x only. MergeFavicon() is used to avoid deleting the 2x
  // favicon bitmap if it is present in the history backend.
  void SetFavicons(const GURL& page_url,
                   favicon_base::IconType icon_type,
                   const GURL& icon_url,
                   const std::vector<SkBitmap>& bitmaps);

  // Used by the FaviconService to mark the favicon for the page as being out
  // of date.
  void SetFaviconsOutOfDateForPage(const GURL& page_url);

  // Used by the FaviconService for importing many favicons for many pages at
  // once. The pages must exist, any favicon sets for unknown pages will be
  // discarded. Existing favicons will not be overwritten.
  void SetImportedFavicons(
      const favicon_base::FaviconUsageDataList& favicon_usage);

  // Sets the in-memory URL database. This is called by the backend once the
  // database is loaded to make it available.
  void SetInMemoryBackend(scoped_ptr<InMemoryHistoryBackend> mem_backend);

  // Called by our BackendDelegate when there is a problem reading the database.
  void NotifyProfileError(sql::InitStatus init_status);

  // Call to schedule a given task for running on the history thread with the
  // specified priority. The task will have ownership taken.
  void ScheduleTask(SchedulePriority priority, const base::Closure& task);

  // Invokes all callback registered by AddFaviconChangedCallback.
  void NotifyFaviconChanged(const std::set<GURL>& changed_favicons);

  base::ThreadChecker thread_checker_;

  // The thread used by the history service to run complicated operations.
  // |thread_| is null once Cleanup() is called.
  base::Thread* thread_;

  // This class has most of the implementation and runs on the 'thread_'.
  // You MUST communicate with this class ONLY through the thread_'s
  // message_loop().
  //
  // This pointer will be null once Cleanup() has been called, meaning no
  // more calls should be made to the history thread.
  scoped_refptr<HistoryBackend> history_backend_;

  // A cache of the user-typed URLs kept in memory that is used by the
  // autocomplete system. This will be null until the database has been created
  // on the background thread.
  // TODO(mrossetti): Consider changing ownership. See http://crbug.com/138321
  scoped_ptr<InMemoryHistoryBackend> in_memory_backend_;

  // The history client, may be null when testing.
  scoped_ptr<HistoryClient> history_client_;

  // The history service will inform its VisitDelegate of URLs recorded and
  // removed from the history database. This may be null during testing.
  scoped_ptr<VisitDelegate> visit_delegate_;

  // Has the backend finished loading? The backend is loaded once Init has
  // completed.
  bool backend_loaded_;

  base::ObserverList<HistoryServiceObserver> observers_;
  base::CallbackList<void(const std::set<GURL>&)>
      favicon_changed_callback_list_;

  DeleteDirectiveHandler delete_directive_handler_;

  // All vended weak pointers are invalidated in Cleanup().
  base::WeakPtrFactory<HistoryService> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(HistoryService);
};

}  // namespace history

#endif  // COMPONENTS_HISTORY_CORE_BROWSER_HISTORY_SERVICE_H_
