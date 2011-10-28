// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data_file_system_helper.h"

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/file_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "content/browser/browser_thread.h"
#include "webkit/fileapi/file_system_quota_util.h"
#include "webkit/fileapi/file_system_context.h"
#include "webkit/fileapi/file_system_types.h"
#include "webkit/fileapi/sandbox_mount_point_provider.h"

namespace {

// An implementation of the BrowsingDataFileSystemHelper interface that pulls
// data from a given |profile| and returns a list of FileSystemInfo items to a
// client.
class BrowsingDataFileSystemHelperImpl : public BrowsingDataFileSystemHelper {
 public:
  // BrowsingDataFileSystemHelper implementation
  explicit BrowsingDataFileSystemHelperImpl(Profile* profile);
  virtual void StartFetching(const base::Callback<
      void(const std::list<FileSystemInfo>&)>& callback) OVERRIDE;
  virtual void CancelNotification() OVERRIDE;
  virtual void DeleteFileSystemOrigin(const GURL& origin) OVERRIDE;

 private:
  virtual ~BrowsingDataFileSystemHelperImpl();

  // Enumerates all filesystem files, storing the resulting list into
  // file_system_file_ for later use. This must be called on the FILE thread.
  void FetchFileSystemInfoInFileThread();

  // Triggers the success callback as the end of a StartFetching workflow. This
  // must be called on the UI thread.
  void NotifyOnUIThread();

  // Deletes all file systems associated with |origin|. This must be called on
  // the FILE thread.
  void DeleteFileSystemOriginInFileThread(const GURL& origin);

  // We don't own the Profile object. Clients are responsible for destroying the
  // object when it's no longer used.
  Profile* profile_;

  // Holds the current list of file systems returned to the client after
  // StartFetching is called. This only mutates in the FILE thread.
  std::list<FileSystemInfo> file_system_info_;

  // Holds the callback passed in at the beginning of the StartFetching workflow
  // so that it can be triggered via NotifyOnUIThread. This only mutates on the
  // UI thread.
  base::Callback<void(const std::list<FileSystemInfo>&)> completion_callback_;

  // Indicates whether or not we're currently fetching information: set to true
  // when StartFetching is called on the UI thread, and reset to false when
  // NotifyOnUIThread triggers the success callback.
  // This property only mutates on the UI thread.
  bool is_fetching_;

  DISALLOW_COPY_AND_ASSIGN(BrowsingDataFileSystemHelperImpl);
};

BrowsingDataFileSystemHelperImpl::BrowsingDataFileSystemHelperImpl(
    Profile* profile)
    : profile_(profile),
      is_fetching_(false) {
  DCHECK(profile_);
}

BrowsingDataFileSystemHelperImpl::~BrowsingDataFileSystemHelperImpl() {
}

void BrowsingDataFileSystemHelperImpl::StartFetching(
    const base::Callback<void(const std::list<FileSystemInfo>&)>& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!is_fetching_);
  DCHECK_EQ(false, callback.is_null());
  is_fetching_ = true;
  completion_callback_ = callback;
  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(
          &BrowsingDataFileSystemHelperImpl::FetchFileSystemInfoInFileThread,
          this));
}

void BrowsingDataFileSystemHelperImpl::CancelNotification() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  completion_callback_.Reset();
}

void BrowsingDataFileSystemHelperImpl::DeleteFileSystemOrigin(
    const GURL& origin) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(
          &BrowsingDataFileSystemHelperImpl::DeleteFileSystemOriginInFileThread,
          this, origin));
}

void BrowsingDataFileSystemHelperImpl::FetchFileSystemInfoInFileThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  scoped_ptr<fileapi::SandboxMountPointProvider::OriginEnumerator>
      origin_enumerator(profile_->GetFileSystemContext()->path_manager()->
          sandbox_provider()->CreateOriginEnumerator());

  // We don't own this pointer; it's a magic singleton generated by the
  // profile's FileSystemContext. Deleting it would be a bad idea.
  fileapi::FileSystemQuotaUtil* quota_util = profile_->
      GetFileSystemContext()->GetQuotaUtil(fileapi::kFileSystemTypeTemporary);

  GURL current;
  while (!(current = origin_enumerator->Next()).is_empty()) {
    if (current.SchemeIs(chrome::kExtensionScheme)) {
      // Extension state is not considered browsing data.
      continue;
    }
    // We can call these synchronous methods as we've already verified that
    // we're running on the FILE thread.
    int64 persistent_usage = quota_util->GetOriginUsageOnFileThread(current,
        fileapi::kFileSystemTypePersistent);
    int64 temporary_usage = quota_util->GetOriginUsageOnFileThread(current,
        fileapi::kFileSystemTypeTemporary);
    file_system_info_.push_back(
        FileSystemInfo(
            current,
            origin_enumerator->HasFileSystemType(
                fileapi::kFileSystemTypePersistent),
            origin_enumerator->HasFileSystemType(
                fileapi::kFileSystemTypeTemporary),
            persistent_usage,
            temporary_usage));
  }

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&BrowsingDataFileSystemHelperImpl::NotifyOnUIThread, this));
}

void BrowsingDataFileSystemHelperImpl::NotifyOnUIThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(is_fetching_);
  // completion_callback_ mutates only in the UI thread, so we're safe to test
  // it here.
  if (!completion_callback_.is_null()) {
    completion_callback_.Run(file_system_info_);
    completion_callback_.Reset();
  }
  is_fetching_ = false;
}

void BrowsingDataFileSystemHelperImpl::DeleteFileSystemOriginInFileThread(
    const GURL& origin) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  profile_->GetFileSystemContext()->DeleteDataForOriginOnFileThread(origin);
}

}  // namespace

BrowsingDataFileSystemHelper::FileSystemInfo::FileSystemInfo(
    const GURL& origin,
    bool has_persistent,
    bool has_temporary,
    int64 usage_persistent,
    int64 usage_temporary)
    : origin(origin),
      has_persistent(has_persistent),
      has_temporary(has_temporary),
      usage_persistent(usage_persistent),
      usage_temporary(usage_temporary) {
}

BrowsingDataFileSystemHelper::FileSystemInfo::~FileSystemInfo() {}

// static
BrowsingDataFileSystemHelper* BrowsingDataFileSystemHelper::Create(
    Profile* profile) {
  return new BrowsingDataFileSystemHelperImpl(profile);
}

CannedBrowsingDataFileSystemHelper::CannedBrowsingDataFileSystemHelper(
    Profile* /* profile */)
    : is_fetching_(false) {
}

CannedBrowsingDataFileSystemHelper::CannedBrowsingDataFileSystemHelper()
    : is_fetching_(false) {
}

CannedBrowsingDataFileSystemHelper::~CannedBrowsingDataFileSystemHelper() {}

CannedBrowsingDataFileSystemHelper*
    CannedBrowsingDataFileSystemHelper::Clone() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  CannedBrowsingDataFileSystemHelper* clone =
      new CannedBrowsingDataFileSystemHelper();
  // This list only mutates on the UI thread, so it's safe to work with it here
  // (given the DCHECK above).
  clone->file_system_info_ = file_system_info_;
  return clone;
}

void CannedBrowsingDataFileSystemHelper::AddFileSystem(
    const GURL& origin, const fileapi::FileSystemType type, const int64 size) {

  // This canned implementation of AddFileSystem uses an O(n^2) algorithm; which
  // is fine, as it isn't meant for use in a high-volume context. If it turns
  // out that we want to start using this in a context with many, many origins,
  // we should think about reworking the implementation.
  bool duplicate_origin = false;
  for (std::list<FileSystemInfo>::iterator
           file_system = file_system_info_.begin();
       file_system != file_system_info_.end();
       ++file_system) {
    if (file_system->origin == origin) {
      if (type == fileapi::kFileSystemTypePersistent) {
        file_system->has_persistent = true;
        file_system->usage_persistent = size;
      } else {
        file_system->has_temporary = true;
        file_system->usage_temporary = size;
      }
      duplicate_origin = true;
      break;
    }
  }
  if (duplicate_origin)
    return;

  file_system_info_.push_back(FileSystemInfo(
      origin,
      (type == fileapi::kFileSystemTypePersistent),
      (type == fileapi::kFileSystemTypeTemporary),
      (type == fileapi::kFileSystemTypePersistent) ? size : 0,
      (type == fileapi::kFileSystemTypeTemporary) ? size : 0));
}

void CannedBrowsingDataFileSystemHelper::Reset() {
  file_system_info_.clear();
}

bool CannedBrowsingDataFileSystemHelper::empty() const {
  return file_system_info_.empty();
}

void CannedBrowsingDataFileSystemHelper::StartFetching(
    const base::Callback<void(const std::list<FileSystemInfo>&)>& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!is_fetching_);
  DCHECK_EQ(false, callback.is_null());
  is_fetching_ = true;
  completion_callback_ = callback;

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&CannedBrowsingDataFileSystemHelper::NotifyOnUIThread, this));
}

void CannedBrowsingDataFileSystemHelper::NotifyOnUIThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(is_fetching_);
  if (!completion_callback_.is_null()) {
    completion_callback_.Run(file_system_info_);
    completion_callback_.Reset();
  }
  is_fetching_ = false;
}

void CannedBrowsingDataFileSystemHelper::CancelNotification() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  completion_callback_.Reset();
}
