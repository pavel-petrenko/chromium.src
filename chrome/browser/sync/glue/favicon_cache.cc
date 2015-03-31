// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/glue/favicon_cache.h"

#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "components/favicon/core/favicon_service.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "sync/api/time.h"
#include "sync/protocol/favicon_image_specifics.pb.h"
#include "sync/protocol/favicon_tracking_specifics.pb.h"
#include "sync/protocol/sync.pb.h"
#include "ui/gfx/favicon_size.h"

namespace browser_sync {

// Synced favicon storage and tracking.
// Note: we don't use the favicon service for storing these because these
// favicons are not necessarily associated with any local navigation, and
// hence would not work with the current expiration logic. We have custom
// expiration logic based on visit time/bookmark status/etc.
// See crbug.com/122890.
struct SyncedFaviconInfo {
  explicit SyncedFaviconInfo(const GURL& favicon_url)
      : favicon_url(favicon_url),
        is_bookmarked(false),
        received_local_update(false) {}

  // The actual favicon data.
  // TODO(zea): don't keep around the actual data for locally sourced
  // favicons (UI can access those directly).
  favicon_base::FaviconRawBitmapResult bitmap_data[NUM_SIZES];
  // The URL this favicon was loaded from.
  const GURL favicon_url;
  // Is the favicon for a bookmarked page?
  bool is_bookmarked;
  // The last time a tab needed this favicon.
  // Note: Do not modify this directly! It should only be modified via
  // UpdateFaviconVisitTime(..).
  base::Time last_visit_time;
  // Whether we've received a local update for this favicon since starting up.
  bool received_local_update;

 private:
  DISALLOW_COPY_AND_ASSIGN(SyncedFaviconInfo);
};

// Information for handling local favicon updates. Used in
// OnFaviconDataAvailable.
struct LocalFaviconUpdateInfo {
  LocalFaviconUpdateInfo()
      : new_image(false),
        new_tracking(false),
        image_needs_rewrite(false),
        favicon_info(NULL) {}

  bool new_image;
  bool new_tracking;
  bool image_needs_rewrite;
  SyncedFaviconInfo* favicon_info;
};

namespace {

// Maximum number of favicons to keep in memory (0 means no limit).
const size_t kMaxFaviconsInMem = 0;

// Maximum width/height resolution supported.
const int kMaxFaviconResolution = 16;

// Returns a mask of the supported favicon types.
// TODO(zea): Supporting other favicons types will involve some work in the
// favicon service and navigation controller. See crbug.com/181068.
int SupportedFaviconTypes() { return favicon_base::FAVICON; }

// Returns the appropriate IconSize to use for a given gfx::Size pixel
// dimensions.
IconSize GetIconSizeBinFromBitmapResult(const gfx::Size& pixel_size) {
  int max_size =
      (pixel_size.width() > pixel_size.height() ?
       pixel_size.width() : pixel_size.height());
  // TODO(zea): re-enable 64p and 32p resolutions once we support them.
  if (max_size > 64)
    return SIZE_INVALID;
  else if (max_size > 32)
    return SIZE_INVALID;
  else if (max_size > 16)
    return SIZE_INVALID;
  else
    return SIZE_16;
}

// Helper for debug statements.
std::string IconSizeToString(IconSize icon_size) {
  switch (icon_size) {
    case SIZE_16:
      return "16";
    case SIZE_32:
      return "32";
    case SIZE_64:
      return "64";
    default:
      return "INVALID";
  }
}

// Extract the favicon url from either of the favicon types.
GURL GetFaviconURLFromSpecifics(const sync_pb::EntitySpecifics& specifics) {
  if (specifics.has_favicon_tracking())
    return GURL(specifics.favicon_tracking().favicon_url());
  else
    return GURL(specifics.favicon_image().favicon_url());
}

// Convert protobuf image data into a FaviconRawBitmapResult.
favicon_base::FaviconRawBitmapResult GetImageDataFromSpecifics(
    const sync_pb::FaviconData& favicon_data) {
  base::RefCountedString* temp_string =
      new base::RefCountedString();
  temp_string->data() = favicon_data.favicon();
  favicon_base::FaviconRawBitmapResult bitmap_result;
  bitmap_result.bitmap_data = temp_string;
  bitmap_result.pixel_size.set_height(favicon_data.height());
  bitmap_result.pixel_size.set_width(favicon_data.width());
  return bitmap_result;
}

// Convert a FaviconRawBitmapResult into protobuf image data.
void FillSpecificsWithImageData(
    const favicon_base::FaviconRawBitmapResult& bitmap_result,
    sync_pb::FaviconData* favicon_data) {
  if (!bitmap_result.bitmap_data.get())
    return;
  favicon_data->set_height(bitmap_result.pixel_size.height());
  favicon_data->set_width(bitmap_result.pixel_size.width());
  favicon_data->set_favicon(bitmap_result.bitmap_data->front(),
                            bitmap_result.bitmap_data->size());
}

// Build a FaviconImageSpecifics from a SyncedFaviconInfo.
void BuildImageSpecifics(
    const SyncedFaviconInfo* favicon_info,
    sync_pb::FaviconImageSpecifics* image_specifics) {
  image_specifics->set_favicon_url(favicon_info->favicon_url.spec());
  FillSpecificsWithImageData(favicon_info->bitmap_data[SIZE_16],
                             image_specifics->mutable_favicon_web());
  // TODO(zea): bring this back if we can handle the load.
  // FillSpecificsWithImageData(favicon_info->bitmap_data[SIZE_32],
  //                            image_specifics->mutable_favicon_web_32());
  // FillSpecificsWithImageData(favicon_info->bitmap_data[SIZE_64],
  //                            image_specifics->mutable_favicon_touch_64());
}

// Build a FaviconTrackingSpecifics from a SyncedFaviconInfo.
void BuildTrackingSpecifics(
    const SyncedFaviconInfo* favicon_info,
    sync_pb::FaviconTrackingSpecifics* tracking_specifics) {
  tracking_specifics->set_favicon_url(favicon_info->favicon_url.spec());
  tracking_specifics->set_last_visit_time_ms(
      syncer::TimeToProtoTime(favicon_info->last_visit_time));
  tracking_specifics->set_is_bookmarked(favicon_info->is_bookmarked);
}

// Updates |favicon_info| with the image data in |bitmap_result|.
bool UpdateFaviconFromBitmapResult(
    const favicon_base::FaviconRawBitmapResult& bitmap_result,
    SyncedFaviconInfo* favicon_info) {
  DCHECK_EQ(favicon_info->favicon_url, bitmap_result.icon_url);
  if (!bitmap_result.is_valid()) {
    DVLOG(1) << "Received invalid favicon at " << bitmap_result.icon_url.spec();
    return false;
  }

  IconSize icon_size = GetIconSizeBinFromBitmapResult(
      bitmap_result.pixel_size);
  if (icon_size == SIZE_INVALID) {
    DVLOG(1) << "Ignoring unsupported resolution "
             << bitmap_result.pixel_size.height() << "x"
             << bitmap_result.pixel_size.width();
    return false;
  } else if (!favicon_info->bitmap_data[icon_size].bitmap_data.get() ||
             !favicon_info->received_local_update) {
    DVLOG(1) << "Storing " << IconSizeToString(icon_size) << "p"
             << " favicon for " << favicon_info->favicon_url.spec()
             << " with size " << bitmap_result.bitmap_data->size()
             << " bytes.";
    favicon_info->bitmap_data[icon_size] = bitmap_result;
    favicon_info->received_local_update = true;
    return true;
  } else {
    // We only allow updating the image data once per restart.
    DVLOG(2) << "Ignoring local update for " << bitmap_result.icon_url.spec();
    return false;
  }
}

bool FaviconInfoHasImages(const SyncedFaviconInfo& favicon_info) {
  return favicon_info.bitmap_data[SIZE_16].bitmap_data.get() ||
         favicon_info.bitmap_data[SIZE_32].bitmap_data.get() ||
         favicon_info.bitmap_data[SIZE_64].bitmap_data.get();
}

bool FaviconInfoHasTracking(const SyncedFaviconInfo& favicon_info) {
  return !favicon_info.last_visit_time.is_null();
}

bool FaviconInfoHasValidTypeData(const SyncedFaviconInfo& favicon_info,
                             syncer::ModelType type) {
  if (type == syncer::FAVICON_IMAGES)
    return FaviconInfoHasImages(favicon_info);
  else if (type == syncer::FAVICON_TRACKING)
    return FaviconInfoHasTracking(favicon_info);
  NOTREACHED();
  return false;
}

}  // namespace

FaviconCache::FaviconCache(Profile* profile, int max_sync_favicon_limit)
    : profile_(profile),
      max_sync_favicon_limit_(max_sync_favicon_limit),
      history_service_observer_(this),
      weak_ptr_factory_(this) {
  history::HistoryService* hs = NULL;
  if (profile_) {
    hs = HistoryServiceFactory::GetForProfile(
        profile_, ServiceAccessType::EXPLICIT_ACCESS);
  }
  if (hs)
    history_service_observer_.Add(hs);
  DVLOG(1) << "Setting favicon limit to " << max_sync_favicon_limit;
}

FaviconCache::~FaviconCache() {}

syncer::SyncMergeResult FaviconCache::MergeDataAndStartSyncing(
    syncer::ModelType type,
    const syncer::SyncDataList& initial_sync_data,
    scoped_ptr<syncer::SyncChangeProcessor> sync_processor,
    scoped_ptr<syncer::SyncErrorFactory> error_handler) {
  DCHECK(type == syncer::FAVICON_IMAGES || type == syncer::FAVICON_TRACKING);
  if (type == syncer::FAVICON_IMAGES)
    favicon_images_sync_processor_ = sync_processor.Pass();
  else
    favicon_tracking_sync_processor_ = sync_processor.Pass();

  syncer::SyncMergeResult merge_result(type);
  merge_result.set_num_items_before_association(synced_favicons_.size());
  std::set<GURL> unsynced_favicon_urls;
  for (FaviconMap::const_iterator iter = synced_favicons_.begin();
       iter != synced_favicons_.end(); ++iter) {
    if (FaviconInfoHasValidTypeData(*(iter->second), type))
      unsynced_favicon_urls.insert(iter->first);
  }

  syncer::SyncChangeList local_changes;
  for (syncer::SyncDataList::const_iterator iter = initial_sync_data.begin();
       iter != initial_sync_data.end(); ++iter) {
    GURL remote_url = GetFaviconURLFromSpecifics(iter->GetSpecifics());
    GURL favicon_url = GetLocalFaviconFromSyncedData(*iter);
    if (favicon_url.is_valid()) {
      unsynced_favicon_urls.erase(favicon_url);
      MergeSyncFavicon(*iter, &local_changes);
      merge_result.set_num_items_modified(
          merge_result.num_items_modified() + 1);
    } else {
      AddLocalFaviconFromSyncedData(*iter);
      merge_result.set_num_items_added(merge_result.num_items_added() + 1);
    }
  }

  // Rather than trigger a bunch of deletions when we set up sync, we drop
  // local favicons. Those pages that are currently open are likely to result in
  // loading new favicons/refreshing old favicons anyways, at which point
  // they'll be re-added and the appropriate synced favicons will be evicted.
  // TODO(zea): implement a smarter ordering of the which favicons to drop.
  int available_favicons = max_sync_favicon_limit_ - initial_sync_data.size();
  UMA_HISTOGRAM_BOOLEAN("Sync.FaviconsAvailableAtMerge",
                        available_favicons > 0);
  for (std::set<GURL>::const_iterator iter = unsynced_favicon_urls.begin();
       iter != unsynced_favicon_urls.end(); ++iter) {
    if (available_favicons > 0) {
      local_changes.push_back(
          syncer::SyncChange(FROM_HERE,
                             syncer::SyncChange::ACTION_ADD,
                             CreateSyncDataFromLocalFavicon(type, *iter)));
      available_favicons--;
    } else {
      FaviconMap::iterator favicon_iter = synced_favicons_.find(*iter);
      DVLOG(1) << "Dropping local favicon "
               << favicon_iter->second->favicon_url.spec();
      DropPartialFavicon(favicon_iter, type);
      merge_result.set_num_items_deleted(merge_result.num_items_deleted() + 1);
    }
  }
  UMA_HISTOGRAM_COUNTS_10000("Sync.FaviconCount", synced_favicons_.size());
  merge_result.set_num_items_after_association(synced_favicons_.size());

  if (type == syncer::FAVICON_IMAGES) {
    favicon_images_sync_processor_->ProcessSyncChanges(FROM_HERE,
                                                       local_changes);
  } else {
    favicon_tracking_sync_processor_->ProcessSyncChanges(FROM_HERE,
                                                         local_changes);
  }
  return merge_result;
}

void FaviconCache::StopSyncing(syncer::ModelType type) {
  favicon_images_sync_processor_.reset();
  favicon_tracking_sync_processor_.reset();
  cancelable_task_tracker_.TryCancelAll();
  page_task_map_.clear();
}

syncer::SyncDataList FaviconCache::GetAllSyncData(syncer::ModelType type)
    const {
  syncer::SyncDataList data_list;
  for (FaviconMap::const_iterator iter = synced_favicons_.begin();
       iter != synced_favicons_.end(); ++iter) {
    if ((type == syncer::FAVICON_IMAGES &&
         FaviconInfoHasImages(*iter->second)) ||
        (type == syncer::FAVICON_TRACKING &&
         FaviconInfoHasTracking(*iter->second))) {
      data_list.push_back(CreateSyncDataFromLocalFavicon(type, iter->first));
    }
  }
  return data_list;
}

syncer::SyncError FaviconCache::ProcessSyncChanges(
    const tracked_objects::Location& from_here,
    const syncer::SyncChangeList& change_list) {
  if (!favicon_images_sync_processor_.get() ||
      !favicon_tracking_sync_processor_.get()) {
    return syncer::SyncError(FROM_HERE,
                             syncer::SyncError::DATATYPE_ERROR,
                             "One or both favicon types disabled.",
                             change_list[0].sync_data().GetDataType());
  }

  syncer::SyncChangeList new_changes;
  syncer::SyncError error;
  syncer::ModelType type = syncer::UNSPECIFIED;
  for (syncer::SyncChangeList::const_iterator iter = change_list.begin();
      iter != change_list.end(); ++iter) {
    type = iter->sync_data().GetDataType();
    DCHECK(type == syncer::FAVICON_IMAGES || type == syncer::FAVICON_TRACKING);
    GURL favicon_url =
        GetFaviconURLFromSpecifics(iter->sync_data().GetSpecifics());
    if (!favicon_url.is_valid()) {
      error.Reset(FROM_HERE, "Received invalid favicon url.", type);
      break;
    }
    FaviconMap::iterator favicon_iter = synced_favicons_.find(favicon_url);
    if (iter->change_type() == syncer::SyncChange::ACTION_DELETE) {
      if (favicon_iter == synced_favicons_.end()) {
        // Two clients might wind up deleting different parts of the same
        // favicon, so ignore this.
        continue;
      } else {
        DVLOG(1) << "Deleting favicon at " << favicon_url.spec();
        // If we only have partial data for the favicon (which implies orphaned
        // nodes), delete the local favicon only if the type corresponds to the
        // partial data we have. If we do have orphaned nodes, we rely on the
        // expiration logic to remove them eventually.
        DropPartialFavicon(favicon_iter, type);
      }
    } else if (iter->change_type() == syncer::SyncChange::ACTION_UPDATE ||
               iter->change_type() == syncer::SyncChange::ACTION_ADD) {
      // Adds and updates are treated the same due to the lack of strong
      // consistency (it's possible we'll receive an update for a tracking info
      // before we've received the add for the image, and should handle both
      // gracefully).
      if (favicon_iter == synced_favicons_.end()) {
        DVLOG(1) << "Adding favicon at " << favicon_url.spec();
        AddLocalFaviconFromSyncedData(iter->sync_data());
      } else {
        DVLOG(1) << "Updating favicon at " << favicon_url.spec();
        MergeSyncFavicon(iter->sync_data(), &new_changes);
      }
    } else {
      error.Reset(FROM_HERE, "Invalid action received.", type);
      break;
    }
  }

  // Note: we deliberately do not expire favicons here. If we received new
  // favicons and are now over the limit, the next local favicon change will
  // trigger the necessary expiration.
  if (!error.IsSet() && !new_changes.empty()) {
    if (type == syncer::FAVICON_IMAGES) {
        favicon_images_sync_processor_->ProcessSyncChanges(FROM_HERE,
                                                           new_changes);
    } else {
        favicon_tracking_sync_processor_->ProcessSyncChanges(FROM_HERE,
                                                             new_changes);
    }
  }

  return error;
}

void FaviconCache::OnPageFaviconUpdated(const GURL& page_url) {
  DCHECK(page_url.is_valid());

  // If a favicon load is already happening for this url, let it finish.
  if (page_task_map_.find(page_url) != page_task_map_.end())
    return;

  PageFaviconMap::const_iterator url_iter = page_favicon_map_.find(page_url);
  if (url_iter != page_favicon_map_.end()) {
    FaviconMap::const_iterator icon_iter =
        synced_favicons_.find(url_iter->second);
    // TODO(zea): consider what to do when only a subset of supported
    // resolutions are available.
    if (icon_iter != synced_favicons_.end() &&
        icon_iter->second->bitmap_data[SIZE_16].bitmap_data.get()) {
      DVLOG(2) << "Using cached favicon url for " << page_url.spec()
               << ": " << icon_iter->second->favicon_url.spec();
      UpdateFaviconVisitTime(icon_iter->second->favicon_url, base::Time::Now());
      UpdateSyncState(icon_iter->second->favicon_url,
                      syncer::SyncChange::ACTION_INVALID,
                      syncer::SyncChange::ACTION_UPDATE);
      return;
    }
  }

  DVLOG(1) << "Triggering favicon load for url " << page_url.spec();

  if (!profile_) {
    page_task_map_[page_url] = 0;  // For testing only.
    return;
  }
  FaviconService* favicon_service = FaviconServiceFactory::GetForProfile(
      profile_, ServiceAccessType::EXPLICIT_ACCESS);
  if (!favicon_service)
    return;
  // TODO(zea): This appears to only fetch one favicon (best match based on
  // desired_size_in_dip). Figure out a way to fetch all favicons we support.
  // See crbug.com/181068.
  base::CancelableTaskTracker::TaskId id =
      favicon_service->GetFaviconForPageURL(
          page_url,
          SupportedFaviconTypes(),
          kMaxFaviconResolution,
          base::Bind(&FaviconCache::OnFaviconDataAvailable,
                     weak_ptr_factory_.GetWeakPtr(),
                     page_url),
          &cancelable_task_tracker_);
  page_task_map_[page_url] = id;
}

void FaviconCache::OnFaviconVisited(const GURL& page_url,
                                    const GURL& favicon_url) {
  DCHECK(page_url.is_valid());
  if (!favicon_url.is_valid() ||
      synced_favicons_.find(favicon_url) == synced_favicons_.end()) {
    // TODO(zea): consider triggering a favicon load if we have some but not
    // all desired resolutions?
    OnPageFaviconUpdated(page_url);
    return;
  }

  DVLOG(1) << "Associating " << page_url.spec() << " with favicon at "
           << favicon_url.spec() << " and marking visited.";
  page_favicon_map_[page_url] = favicon_url;
  bool had_tracking = FaviconInfoHasTracking(
      *synced_favicons_.find(favicon_url)->second);
  UpdateFaviconVisitTime(favicon_url, base::Time::Now());

  UpdateSyncState(favicon_url,
                  syncer::SyncChange::ACTION_INVALID,
                  (had_tracking ?
                   syncer::SyncChange::ACTION_UPDATE :
                   syncer::SyncChange::ACTION_ADD));
}

bool FaviconCache::GetSyncedFaviconForFaviconURL(
    const GURL& favicon_url,
    scoped_refptr<base::RefCountedMemory>* favicon_png) const {
  if (!favicon_url.is_valid())
    return false;
  FaviconMap::const_iterator iter = synced_favicons_.find(favicon_url);

  UMA_HISTOGRAM_BOOLEAN("Sync.FaviconCacheLookupSucceeded",
                        iter != synced_favicons_.end());
  if (iter == synced_favicons_.end())
    return false;

  // TODO(zea): support getting other resolutions.
  if (!iter->second->bitmap_data[SIZE_16].bitmap_data.get())
    return false;

  *favicon_png = iter->second->bitmap_data[SIZE_16].bitmap_data;
  return true;
}

bool FaviconCache::GetSyncedFaviconForPageURL(
    const GURL& page_url,
    scoped_refptr<base::RefCountedMemory>* favicon_png) const {
  if (!page_url.is_valid())
    return false;
  PageFaviconMap::const_iterator iter = page_favicon_map_.find(page_url);

  if (iter == page_favicon_map_.end())
    return false;

  return GetSyncedFaviconForFaviconURL(iter->second, favicon_png);
}

void FaviconCache::OnReceivedSyncFavicon(const GURL& page_url,
                                         const GURL& icon_url,
                                         const std::string& icon_bytes,
                                         int64 visit_time_ms) {
  if (!icon_url.is_valid() || !page_url.is_valid() || icon_url.SchemeIs("data"))
    return;
  DVLOG(1) << "Associating " << page_url.spec() << " with favicon at "
           << icon_url.spec();
  page_favicon_map_[page_url] = icon_url;

  // If there is no actual image, it means there either is no synced
  // favicon, or it's on its way (race condition).
  // TODO(zea): potentially trigger a favicon web download here (delayed?).
  if (icon_bytes.size() == 0)
    return;

  // Post a task to do the actual association because this method may have been
  // called while in a transaction.
  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&FaviconCache::OnReceivedSyncFaviconImpl,
                 weak_ptr_factory_.GetWeakPtr(),
                 icon_url,
                 icon_bytes,
                 visit_time_ms));
}

void FaviconCache::OnReceivedSyncFaviconImpl(
    const GURL& icon_url,
    const std::string& icon_bytes,
    int64 visit_time_ms) {
  // If this favicon is already synced, do nothing else.
  if (synced_favicons_.find(icon_url) != synced_favicons_.end())
    return;

  // Don't add any more favicons once we hit our in memory limit.
  // TODO(zea): UMA this.
  if (kMaxFaviconsInMem != 0 && synced_favicons_.size() > kMaxFaviconsInMem)
    return;

  SyncedFaviconInfo* favicon_info = GetFaviconInfo(icon_url);
  if (!favicon_info)
    return;  // We reached the in-memory limit.
  base::RefCountedString* temp_string = new base::RefCountedString();
  temp_string->data() = icon_bytes;
  favicon_info->bitmap_data[SIZE_16].bitmap_data = temp_string;
  // We assume legacy favicons are 16x16.
  favicon_info->bitmap_data[SIZE_16].pixel_size.set_width(16);
  favicon_info->bitmap_data[SIZE_16].pixel_size.set_height(16);
  bool added_tracking = !FaviconInfoHasTracking(*favicon_info);
  UpdateFaviconVisitTime(icon_url,
                         syncer::ProtoTimeToTime(visit_time_ms));

  UpdateSyncState(icon_url,
                  syncer::SyncChange::ACTION_ADD,
                  (added_tracking ?
                   syncer::SyncChange::ACTION_ADD :
                   syncer::SyncChange::ACTION_UPDATE));
}

bool FaviconCache::FaviconRecencyFunctor::operator()(
    const linked_ptr<SyncedFaviconInfo>& lhs,
    const linked_ptr<SyncedFaviconInfo>& rhs) const {
  // TODO(zea): incorporate bookmarked status here once we care about it.
  if (lhs->last_visit_time < rhs->last_visit_time)
    return true;
  else if (lhs->last_visit_time == rhs->last_visit_time)
    return lhs->favicon_url.spec() < rhs->favicon_url.spec();
  return false;
}

void FaviconCache::OnFaviconDataAvailable(
    const GURL& page_url,
    const std::vector<favicon_base::FaviconRawBitmapResult>& bitmap_results) {
  PageTaskMap::iterator page_iter = page_task_map_.find(page_url);
  if (page_iter == page_task_map_.end())
    return;
  page_task_map_.erase(page_iter);

  if (bitmap_results.size() == 0) {
    // Either the favicon isn't loaded yet or there is no valid favicon.
    // We already cleared the task id, so just return.
    DVLOG(1) << "Favicon load failed for page " << page_url.spec();
    return;
  }

  base::Time now = base::Time::Now();
  std::map<GURL, LocalFaviconUpdateInfo> favicon_updates;
  for (size_t i = 0; i < bitmap_results.size(); ++i) {
    const favicon_base::FaviconRawBitmapResult& bitmap_result =
        bitmap_results[i];
    GURL favicon_url = bitmap_result.icon_url;
    if (!favicon_url.is_valid() || favicon_url.SchemeIs("data"))
      continue;  // Can happen if the page is still loading.

    SyncedFaviconInfo* favicon_info = GetFaviconInfo(favicon_url);
    if (!favicon_info)
      return;  // We reached the in-memory limit.

    favicon_updates[favicon_url].new_image |=
        !FaviconInfoHasImages(*favicon_info);
    favicon_updates[favicon_url].new_tracking |=
        !FaviconInfoHasTracking(*favicon_info);
    favicon_updates[favicon_url].image_needs_rewrite |=
        UpdateFaviconFromBitmapResult(bitmap_result, favicon_info);
    favicon_updates[favicon_url].favicon_info = favicon_info;
  }

  for (std::map<GURL, LocalFaviconUpdateInfo>::const_iterator
           iter = favicon_updates.begin(); iter != favicon_updates.end();
       ++iter) {
    SyncedFaviconInfo* favicon_info = iter->second.favicon_info;
    const GURL& favicon_url = favicon_info->favicon_url;

    // TODO(zea): support multiple favicon urls per page.
    page_favicon_map_[page_url] = favicon_url;

    if (!favicon_info->last_visit_time.is_null()) {
      UMA_HISTOGRAM_COUNTS_10000(
          "Sync.FaviconVisitPeriod",
          (now - favicon_info->last_visit_time).InHours());
    }
    favicon_info->received_local_update = true;
    UpdateFaviconVisitTime(favicon_url, now);

    syncer::SyncChange::SyncChangeType image_change =
        syncer::SyncChange::ACTION_INVALID;
    if (iter->second.new_image)
      image_change = syncer::SyncChange::ACTION_ADD;
    else if (iter->second.image_needs_rewrite)
      image_change = syncer::SyncChange::ACTION_UPDATE;
    syncer::SyncChange::SyncChangeType tracking_change =
        syncer::SyncChange::ACTION_UPDATE;
    if (iter->second.new_tracking)
      tracking_change = syncer::SyncChange::ACTION_ADD;
    UpdateSyncState(favicon_url, image_change, tracking_change);
  }
}

void FaviconCache::UpdateSyncState(
    const GURL& icon_url,
    syncer::SyncChange::SyncChangeType image_change_type,
    syncer::SyncChange::SyncChangeType tracking_change_type) {
  DCHECK(icon_url.is_valid());
  // It's possible that we'll receive a favicon update before both types
  // have finished setting up. In that case ignore the update.
  // TODO(zea): consider tracking these skipped updates somehow?
  if (!favicon_images_sync_processor_.get() ||
      !favicon_tracking_sync_processor_.get()) {
    return;
  }

  FaviconMap::const_iterator iter = synced_favicons_.find(icon_url);
  DCHECK(iter != synced_favicons_.end());
  const SyncedFaviconInfo* favicon_info = iter->second.get();

  syncer::SyncChangeList image_changes;
  syncer::SyncChangeList tracking_changes;
  if (image_change_type != syncer::SyncChange::ACTION_INVALID) {
    sync_pb::EntitySpecifics new_specifics;
    sync_pb::FaviconImageSpecifics* image_specifics =
        new_specifics.mutable_favicon_image();
    BuildImageSpecifics(favicon_info, image_specifics);

    image_changes.push_back(
        syncer::SyncChange(FROM_HERE,
                           image_change_type,
                           syncer::SyncData::CreateLocalData(
                               icon_url.spec(),
                               icon_url.spec(),
                               new_specifics)));
  }
  if (tracking_change_type != syncer::SyncChange::ACTION_INVALID) {
    sync_pb::EntitySpecifics new_specifics;
    sync_pb::FaviconTrackingSpecifics* tracking_specifics =
        new_specifics.mutable_favicon_tracking();
    BuildTrackingSpecifics(favicon_info, tracking_specifics);

    tracking_changes.push_back(
        syncer::SyncChange(FROM_HERE,
                           tracking_change_type,
                           syncer::SyncData::CreateLocalData(
                               icon_url.spec(),
                               icon_url.spec(),
                               new_specifics)));
  }
  ExpireFaviconsIfNecessary(&image_changes, &tracking_changes);
  if (!image_changes.empty()) {
    favicon_images_sync_processor_->ProcessSyncChanges(FROM_HERE,
                                                       image_changes);
  }
  if (!tracking_changes.empty()) {
    favicon_tracking_sync_processor_->ProcessSyncChanges(FROM_HERE,
                                                         tracking_changes);
  }
}

SyncedFaviconInfo* FaviconCache::GetFaviconInfo(
    const GURL& icon_url) {
  DCHECK_EQ(recent_favicons_.size(), synced_favicons_.size());
  if (synced_favicons_.count(icon_url) != 0)
    return synced_favicons_[icon_url].get();

  // TODO(zea): implement in-memory eviction.
  DVLOG(1) << "Adding favicon info for " << icon_url.spec();
  SyncedFaviconInfo* favicon_info = new SyncedFaviconInfo(icon_url);
  synced_favicons_[icon_url] = make_linked_ptr(favicon_info);
  recent_favicons_.insert(synced_favicons_[icon_url]);
  DCHECK_EQ(recent_favicons_.size(), synced_favicons_.size());
  return favicon_info;
}

void FaviconCache::UpdateFaviconVisitTime(const GURL& icon_url,
                                          base::Time time) {
  DCHECK_EQ(recent_favicons_.size(), synced_favicons_.size());
  FaviconMap::const_iterator iter = synced_favicons_.find(icon_url);
  DCHECK(iter != synced_favicons_.end());
  if (iter->second->last_visit_time >= time)
    return;
  // Erase, update the time, then re-insert to maintain ordering.
  recent_favicons_.erase(iter->second);
  DVLOG(1) << "Updating " << icon_url.spec() << " visit time to "
           << syncer::GetTimeDebugString(time);
  iter->second->last_visit_time = time;
  recent_favicons_.insert(iter->second);

  if (VLOG_IS_ON(2)) {
    for (RecencySet::const_iterator iter = recent_favicons_.begin();
         iter != recent_favicons_.end(); ++iter) {
      DVLOG(2) << "Favicon " << iter->get()->favicon_url.spec() << ": "
               << syncer::GetTimeDebugString(iter->get()->last_visit_time);
    }
  }
  DCHECK_EQ(recent_favicons_.size(), synced_favicons_.size());
}

void FaviconCache::ExpireFaviconsIfNecessary(
    syncer::SyncChangeList* image_changes,
    syncer::SyncChangeList* tracking_changes) {
  DCHECK_EQ(recent_favicons_.size(), synced_favicons_.size());
  // TODO(zea): once we have in-memory eviction, we'll need to track sync
  // favicon count separately from the synced_favicons_/recent_favicons_.

  // Iterate until we've removed the necessary amount. |recent_favicons_| is
  // already in recency order, so just start from the beginning.
  // TODO(zea): to reduce thrashing, consider removing more than the minimum.
  while (recent_favicons_.size() > max_sync_favicon_limit_) {
    linked_ptr<SyncedFaviconInfo> candidate = *recent_favicons_.begin();
    DVLOG(1) << "Expiring favicon " << candidate->favicon_url.spec();
    DeleteSyncedFavicon(synced_favicons_.find(candidate->favicon_url),
                        image_changes,
                        tracking_changes);
  }
  DCHECK_EQ(recent_favicons_.size(), synced_favicons_.size());
}

GURL FaviconCache::GetLocalFaviconFromSyncedData(
    const syncer::SyncData& sync_favicon) const {
  syncer::ModelType type = sync_favicon.GetDataType();
  DCHECK(type == syncer::FAVICON_IMAGES || type == syncer::FAVICON_TRACKING);
  GURL favicon_url = GetFaviconURLFromSpecifics(sync_favicon.GetSpecifics());
  return (synced_favicons_.count(favicon_url) > 0 ? favicon_url : GURL());
}

void FaviconCache::MergeSyncFavicon(const syncer::SyncData& sync_favicon,
                                    syncer::SyncChangeList* sync_changes) {
  syncer::ModelType type = sync_favicon.GetDataType();
  DCHECK(type == syncer::FAVICON_IMAGES || type == syncer::FAVICON_TRACKING);
  sync_pb::EntitySpecifics new_specifics;
  GURL favicon_url = GetFaviconURLFromSpecifics(sync_favicon.GetSpecifics());
  FaviconMap::const_iterator iter = synced_favicons_.find(favicon_url);
  DCHECK(iter != synced_favicons_.end());
  SyncedFaviconInfo* favicon_info = iter->second.get();
  if (type == syncer::FAVICON_IMAGES) {
    sync_pb::FaviconImageSpecifics image_specifics =
        sync_favicon.GetSpecifics().favicon_image();

    // Remote image data always clobbers local image data.
    bool needs_update = false;
    if (image_specifics.has_favicon_web()) {
      favicon_info->bitmap_data[SIZE_16] = GetImageDataFromSpecifics(
          image_specifics.favicon_web());
    } else if (favicon_info->bitmap_data[SIZE_16].bitmap_data.get()) {
      needs_update = true;
    }
    if (image_specifics.has_favicon_web_32()) {
      favicon_info->bitmap_data[SIZE_32] = GetImageDataFromSpecifics(
          image_specifics.favicon_web_32());
    } else if (favicon_info->bitmap_data[SIZE_32].bitmap_data.get()) {
      needs_update = true;
    }
    if (image_specifics.has_favicon_touch_64()) {
      favicon_info->bitmap_data[SIZE_64] = GetImageDataFromSpecifics(
          image_specifics.favicon_touch_64());
    } else if (favicon_info->bitmap_data[SIZE_64].bitmap_data.get()) {
      needs_update = true;
    }

    if (needs_update)
      BuildImageSpecifics(favicon_info, new_specifics.mutable_favicon_image());
  } else {
    sync_pb::FaviconTrackingSpecifics tracking_specifics =
        sync_favicon.GetSpecifics().favicon_tracking();

    // Tracking data is merged, such that bookmark data is the logical OR
    // of the two, and last visit time is the most recent.

    base::Time last_visit =  syncer::ProtoTimeToTime(
        tracking_specifics.last_visit_time_ms());
    // Due to crbug.com/258196, there are tracking nodes out there with
    // null visit times. If this is one of those, artificially make it a valid
    // visit time, so we know the node exists and update it properly on the next
    // real visit.
    if (last_visit.is_null())
      last_visit = last_visit + base::TimeDelta::FromMilliseconds(1);
    UpdateFaviconVisitTime(favicon_url, last_visit);
    favicon_info->is_bookmarked = (favicon_info->is_bookmarked ||
                                   tracking_specifics.is_bookmarked());

    if (syncer::TimeToProtoTime(favicon_info->last_visit_time) !=
            tracking_specifics.last_visit_time_ms() ||
        favicon_info->is_bookmarked != tracking_specifics.is_bookmarked()) {
      BuildTrackingSpecifics(favicon_info,
                             new_specifics.mutable_favicon_tracking());
    }
    DCHECK(!favicon_info->last_visit_time.is_null());
  }

  if (new_specifics.has_favicon_image() ||
      new_specifics.has_favicon_tracking()) {
    sync_changes->push_back(syncer::SyncChange(
        FROM_HERE,
        syncer::SyncChange::ACTION_UPDATE,
        syncer::SyncData::CreateLocalData(favicon_url.spec(),
                                          favicon_url.spec(),
                                          new_specifics)));
  }
}

void FaviconCache::AddLocalFaviconFromSyncedData(
    const syncer::SyncData& sync_favicon) {
  syncer::ModelType type = sync_favicon.GetDataType();
  DCHECK(type == syncer::FAVICON_IMAGES || type == syncer::FAVICON_TRACKING);
  if (type == syncer::FAVICON_IMAGES) {
    sync_pb::FaviconImageSpecifics image_specifics =
        sync_favicon.GetSpecifics().favicon_image();
    GURL favicon_url = GURL(image_specifics.favicon_url());
    DCHECK(favicon_url.is_valid());
    DCHECK(!synced_favicons_.count(favicon_url));

    SyncedFaviconInfo* favicon_info = GetFaviconInfo(favicon_url);
    if (!favicon_info)
      return;  // We reached the in-memory limit.
    if (image_specifics.has_favicon_web()) {
      favicon_info->bitmap_data[SIZE_16] = GetImageDataFromSpecifics(
          image_specifics.favicon_web());
    }
    if (image_specifics.has_favicon_web_32()) {
      favicon_info->bitmap_data[SIZE_32] = GetImageDataFromSpecifics(
          image_specifics.favicon_web_32());
    }
    if (image_specifics.has_favicon_touch_64()) {
      favicon_info->bitmap_data[SIZE_64] = GetImageDataFromSpecifics(
          image_specifics.favicon_touch_64());
    }
  } else {
    sync_pb::FaviconTrackingSpecifics tracking_specifics =
        sync_favicon.GetSpecifics().favicon_tracking();
    GURL favicon_url = GURL(tracking_specifics.favicon_url());
    DCHECK(favicon_url.is_valid());
    DCHECK(!synced_favicons_.count(favicon_url));

    SyncedFaviconInfo* favicon_info = GetFaviconInfo(favicon_url);
    if (!favicon_info)
      return;  // We reached the in-memory limit.
    base::Time last_visit =  syncer::ProtoTimeToTime(
        tracking_specifics.last_visit_time_ms());
    // Due to crbug.com/258196, there are tracking nodes out there with
    // null visit times. If this is one of those, artificially make it a valid
    // visit time, so we know the node exists and update it properly on the next
    // real visit.
    if (last_visit.is_null())
      last_visit = last_visit + base::TimeDelta::FromMilliseconds(1);
    UpdateFaviconVisitTime(favicon_url, last_visit);
    favicon_info->is_bookmarked = tracking_specifics.is_bookmarked();
    DCHECK(!favicon_info->last_visit_time.is_null());
  }
}

syncer::SyncData FaviconCache::CreateSyncDataFromLocalFavicon(
    syncer::ModelType type,
    const GURL& favicon_url) const {
  DCHECK(type == syncer::FAVICON_IMAGES || type == syncer::FAVICON_TRACKING);
  DCHECK(favicon_url.is_valid());
  FaviconMap::const_iterator iter = synced_favicons_.find(favicon_url);
  DCHECK(iter != synced_favicons_.end());
  SyncedFaviconInfo* favicon_info = iter->second.get();

  syncer::SyncData data;
  sync_pb::EntitySpecifics specifics;
  if (type == syncer::FAVICON_IMAGES) {
    sync_pb::FaviconImageSpecifics* image_specifics =
        specifics.mutable_favicon_image();
    BuildImageSpecifics(favicon_info, image_specifics);
  } else {
    sync_pb::FaviconTrackingSpecifics* tracking_specifics =
        specifics.mutable_favicon_tracking();
    BuildTrackingSpecifics(favicon_info, tracking_specifics);
  }
  data = syncer::SyncData::CreateLocalData(favicon_url.spec(),
                                           favicon_url.spec(),
                                           specifics);
  return data;
}

void FaviconCache::DeleteSyncedFavicons(const std::set<GURL>& favicon_urls) {
  syncer::SyncChangeList image_deletions, tracking_deletions;
  for (std::set<GURL>::const_iterator iter = favicon_urls.begin();
       iter != favicon_urls.end(); ++iter) {
    FaviconMap::iterator favicon_iter = synced_favicons_.find(*iter);
    if (favicon_iter == synced_favicons_.end())
      continue;
    DeleteSyncedFavicon(favicon_iter,
                        &image_deletions,
                        &tracking_deletions);
  }
  DVLOG(1) << "Deleting " << image_deletions.size() << " synced favicons.";
  if (favicon_images_sync_processor_.get()) {
    favicon_images_sync_processor_->ProcessSyncChanges(FROM_HERE,
                                                       image_deletions);
  }
  if (favicon_tracking_sync_processor_.get()) {
    favicon_tracking_sync_processor_->ProcessSyncChanges(FROM_HERE,
                                                         tracking_deletions);
  }
}

void FaviconCache::DeleteSyncedFavicon(
    FaviconMap::iterator favicon_iter,
    syncer::SyncChangeList* image_changes,
    syncer::SyncChangeList* tracking_changes) {
  linked_ptr<SyncedFaviconInfo> favicon_info = favicon_iter->second;
  if (FaviconInfoHasImages(*(favicon_iter->second))) {
    DVLOG(1) << "Deleting image for "
             << favicon_iter->second.get()->favicon_url;
    image_changes->push_back(
        syncer::SyncChange(FROM_HERE,
                           syncer::SyncChange::ACTION_DELETE,
                           syncer::SyncData::CreateLocalDelete(
                               favicon_info->favicon_url.spec(),
                               syncer::FAVICON_IMAGES)));
  }
  if (FaviconInfoHasTracking(*(favicon_iter->second))) {
    DVLOG(1) << "Deleting tracking for "
             << favicon_iter->second.get()->favicon_url;
    tracking_changes->push_back(
        syncer::SyncChange(FROM_HERE,
                           syncer::SyncChange::ACTION_DELETE,
                           syncer::SyncData::CreateLocalDelete(
                               favicon_info->favicon_url.spec(),
                               syncer::FAVICON_TRACKING)));
  }
  DropSyncedFavicon(favicon_iter);
}

void FaviconCache::DropSyncedFavicon(FaviconMap::iterator favicon_iter) {
  DVLOG(1) << "Dropping favicon " << favicon_iter->second.get()->favicon_url;
  recent_favicons_.erase(favicon_iter->second);
  synced_favicons_.erase(favicon_iter);
}

void FaviconCache::DropPartialFavicon(FaviconMap::iterator favicon_iter,
                                      syncer::ModelType type) {
  // If the type being dropped has no valid data, do nothing.
  if ((type == syncer::FAVICON_TRACKING &&
       !FaviconInfoHasTracking(*favicon_iter->second)) ||
      (type == syncer::FAVICON_IMAGES &&
       !FaviconInfoHasImages(*favicon_iter->second))) {
    return;
  }

  // If the type being dropped is the only type with valid data, just delete
  // the favicon altogether.
  if ((type == syncer::FAVICON_TRACKING &&
       !FaviconInfoHasImages(*favicon_iter->second)) ||
      (type == syncer::FAVICON_IMAGES &&
       !FaviconInfoHasTracking(*favicon_iter->second))) {
    DropSyncedFavicon(favicon_iter);
    return;
  }

  if (type == syncer::FAVICON_IMAGES) {
    DVLOG(1) << "Dropping favicon image "
             << favicon_iter->second.get()->favicon_url;
    for (int i = 0; i < NUM_SIZES; ++i) {
      favicon_iter->second->bitmap_data[i] =
          favicon_base::FaviconRawBitmapResult();
    }
    DCHECK(!FaviconInfoHasImages(*favicon_iter->second));
  } else {
    DCHECK_EQ(type, syncer::FAVICON_TRACKING);
    DVLOG(1) << "Dropping favicon tracking "
             << favicon_iter->second.get()->favicon_url;
    recent_favicons_.erase(favicon_iter->second);
    favicon_iter->second->last_visit_time = base::Time();
    favicon_iter->second->is_bookmarked = false;
    recent_favicons_.insert(favicon_iter->second);
    DCHECK(!FaviconInfoHasTracking(*favicon_iter->second));
  }
}

size_t FaviconCache::NumFaviconsForTest() const {
  return synced_favicons_.size();
}

size_t FaviconCache::NumTasksForTest() const {
  return page_task_map_.size();
}

void FaviconCache::OnURLsDeleted(history::HistoryService* history_service,
                                 bool all_history,
                                 bool expired,
                                 const history::URLRows& deleted_rows,
                                 const std::set<GURL>& favicon_urls) {
  // We only care about actual user (or sync) deletions.
  if (expired)
    return;

  if (!all_history) {
    DeleteSyncedFavicons(favicon_urls);
    return;
  }

  // All history was cleared: just delete all favicons.
  DVLOG(1) << "History clear detected, deleting all synced favicons.";
  syncer::SyncChangeList image_deletions, tracking_deletions;
  while (!synced_favicons_.empty()) {
    DeleteSyncedFavicon(synced_favicons_.begin(), &image_deletions,
                        &tracking_deletions);
  }

  if (favicon_images_sync_processor_.get()) {
    favicon_images_sync_processor_->ProcessSyncChanges(FROM_HERE,
                                                       image_deletions);
  }
  if (favicon_tracking_sync_processor_.get()) {
    favicon_tracking_sync_processor_->ProcessSyncChanges(FROM_HERE,
                                                         tracking_deletions);
  }
}

}  // namespace browser_sync
