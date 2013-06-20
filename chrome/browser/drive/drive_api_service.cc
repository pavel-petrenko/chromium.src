// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/drive/drive_api_service.h"

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/message_loop/message_loop_proxy.h"
#include "base/strings/stringprintf.h"
#include "base/task_runner_util.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/values.h"
#include "chrome/browser/drive/drive_api_util.h"
#include "chrome/browser/google_apis/auth_service.h"
#include "chrome/browser/google_apis/drive_api_parser.h"
#include "chrome/browser/google_apis/drive_api_requests.h"
#include "chrome/browser/google_apis/gdata_wapi_parser.h"
#include "chrome/browser/google_apis/request_sender.h"
#include "chrome/browser/google_apis/time_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace google_apis {

namespace {

// OAuth2 scopes for Drive API.
const char kDriveScope[] = "https://www.googleapis.com/auth/drive";
const char kDriveAppsReadonlyScope[] =
    "https://www.googleapis.com/auth/drive.apps.readonly";

// Expected max number of files resources in a http request.
// Be careful not to use something too small because it might overload the
// server. Be careful not to use something too large because it takes longer
// time to fetch the result without UI response.
const int kMaxNumFilesResourcePerRequest = 500;
const int kMaxNumFilesResourcePerRequestForSearch = 50;

scoped_ptr<ResourceList> ParseChangeListJsonToResourceList(
    scoped_ptr<base::Value> value) {
  scoped_ptr<ChangeList> change_list(ChangeList::CreateFrom(*value));
  if (!change_list) {
    return scoped_ptr<ResourceList>();
  }

  return ResourceList::CreateFromChangeList(*change_list);
}

scoped_ptr<ResourceList> ParseFileListJsonToResourceList(
    scoped_ptr<base::Value> value) {
  scoped_ptr<FileList> file_list(FileList::CreateFrom(*value));
  if (!file_list) {
    return scoped_ptr<ResourceList>();
  }

  return ResourceList::CreateFromFileList(*file_list);
}

// Parses JSON value representing either ChangeList or FileList into
// ResourceList.
scoped_ptr<ResourceList> ParseResourceListOnBlockingPool(
    scoped_ptr<base::Value> value) {
  DCHECK(value);

  // Dispatch the parsing based on kind field.
  if (ChangeList::HasChangeListKind(*value)) {
    return ParseChangeListJsonToResourceList(value.Pass());
  }
  if (FileList::HasFileListKind(*value)) {
    return ParseFileListJsonToResourceList(value.Pass());
  }

  // The value type is unknown, so give up to parse and return an error.
  return scoped_ptr<ResourceList>();
}

// Callback invoked when the parsing of resource list is completed,
// regardless whether it is succeeded or not.
void DidParseResourceListOnBlockingPool(
    const GetResourceListCallback& callback,
    scoped_ptr<ResourceList> resource_list) {
  GDataErrorCode error = resource_list ? HTTP_SUCCESS : GDATA_PARSE_ERROR;
  callback.Run(error, resource_list.Pass());
}

// Sends a task to parse the JSON value into ResourceList on blocking pool,
// with a callback which is called when the task is done.
void ParseResourceListOnBlockingPoolAndRun(
    const GetResourceListCallback& callback,
    GDataErrorCode error,
    scoped_ptr<base::Value> value) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  if (error != HTTP_SUCCESS) {
    // An error occurs, so run callback immediately.
    callback.Run(error, scoped_ptr<ResourceList>());
    return;
  }

  PostTaskAndReplyWithResult(
      BrowserThread::GetBlockingPool(),
      FROM_HERE,
      base::Bind(&ParseResourceListOnBlockingPool, base::Passed(&value)),
      base::Bind(&DidParseResourceListOnBlockingPool, callback));
}

// Parses the FileResource value to ResourceEntry and runs |callback| on the
// UI thread.
void ParseResourceEntryAndRun(
    const GetResourceEntryCallback& callback,
    GDataErrorCode error,
    scoped_ptr<FileResource> value) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  if (!value) {
    callback.Run(error, scoped_ptr<ResourceEntry>());
    return;
  }

  // Converting to ResourceEntry is cheap enough to do on UI thread.
  scoped_ptr<ResourceEntry> entry =
      ResourceEntry::CreateFromFileResource(*value);
  if (!entry) {
    callback.Run(GDATA_PARSE_ERROR, scoped_ptr<ResourceEntry>());
    return;
  }

  callback.Run(error, entry.Pass());
}

// Parses the JSON value to AppList runs |callback| on the UI thread
// once parsing is done.
void ParseAppListAndRun(const google_apis::GetAppListCallback& callback,
                        google_apis::GDataErrorCode error,
                        scoped_ptr<base::Value> value) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  if (!value) {
    callback.Run(error, scoped_ptr<google_apis::AppList>());
    return;
  }

  // Parsing AppList is cheap enough to do on UI thread.
  scoped_ptr<google_apis::AppList> app_list =
      google_apis::AppList::CreateFrom(*value);
  if (!app_list) {
    callback.Run(google_apis::GDATA_PARSE_ERROR,
                 scoped_ptr<google_apis::AppList>());
    return;
  }

  callback.Run(error, app_list.Pass());
}

// Parses the FileResource value to ResourceEntry for upload range request,
// and runs |callback| on the UI thread.
void ParseResourceEntryForUploadRangeAndRun(
    const UploadRangeCallback& callback,
    const UploadRangeResponse& response,
    scoped_ptr<FileResource> value) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  if (!value) {
    callback.Run(response, scoped_ptr<ResourceEntry>());
    return;
  }

  // Converting to ResourceEntry is cheap enough to do on UI thread.
  scoped_ptr<ResourceEntry> entry =
      ResourceEntry::CreateFromFileResource(*value);
  if (!entry) {
    callback.Run(UploadRangeResponse(GDATA_PARSE_ERROR,
                                     response.start_position_received,
                                     response.end_position_received),
                 scoped_ptr<ResourceEntry>());
    return;
  }

  callback.Run(response, entry.Pass());
}

void ExtractOpenUrlAndRun(const std::string& app_id,
                          const AuthorizeAppCallback& callback,
                          GDataErrorCode error,
                          scoped_ptr<FileResource> value) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  if (!value) {
    callback.Run(error, GURL());
    return;
  }

  const std::vector<FileResource::OpenWithLink>& open_with_links =
      value->open_with_links();
  for (size_t i = 0; i < open_with_links.size(); ++i) {
    if (open_with_links[i].app_id == app_id) {
      callback.Run(HTTP_SUCCESS, open_with_links[i].open_url);
      return;
    }
  }

  // Not found.
  callback.Run(GDATA_OTHER_ERROR, GURL());
}

// The resource ID for the root directory for Drive API is defined in the spec:
// https://developers.google.com/drive/folder
const char kDriveApiRootDirectoryResourceId[] = "root";

}  // namespace

DriveAPIService::DriveAPIService(
    net::URLRequestContextGetter* url_request_context_getter,
    const GURL& base_url,
    const std::string& custom_user_agent)
    : url_request_context_getter_(url_request_context_getter),
      profile_(NULL),
      url_generator_(base_url),
      custom_user_agent_(custom_user_agent) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

DriveAPIService::~DriveAPIService() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (sender_.get())
    sender_->auth_service()->RemoveObserver(this);
}

void DriveAPIService::Initialize(Profile* profile) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  profile_ = profile;

  std::vector<std::string> scopes;
  scopes.push_back(kDriveScope);
  scopes.push_back(kDriveAppsReadonlyScope);
  sender_.reset(new RequestSender(profile,
                                  url_request_context_getter_,
                                  scopes,
                                  custom_user_agent_));
  sender_->Initialize();

  sender_->auth_service()->AddObserver(this);
}

void DriveAPIService::AddObserver(DriveServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void DriveAPIService::RemoveObserver(DriveServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

bool DriveAPIService::CanSendRequest() const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  return HasRefreshToken();
}

std::string DriveAPIService::CanonicalizeResourceId(
    const std::string& resource_id) const {
  return drive::util::CanonicalizeResourceId(resource_id);
}

std::string DriveAPIService::GetRootResourceId() const {
  return kDriveApiRootDirectoryResourceId;
}

CancelCallback DriveAPIService::GetAllResourceList(
    const GetResourceListCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  // The simplest way to fetch the all resources list looks files.list method,
  // but it seems impossible to know the returned list's changestamp.
  // Thus, instead, we use changes.list method with includeDeleted=false here.
  // The returned list should contain only resources currently existing.
  return sender_->StartRequestWithRetry(
      new GetChangelistRequest(
          sender_.get(),
          url_request_context_getter_,
          url_generator_,
          false,  // include deleted
          0,
          kMaxNumFilesResourcePerRequest,
          base::Bind(&ParseResourceListOnBlockingPoolAndRun, callback)));
}

CancelCallback DriveAPIService::GetResourceListInDirectory(
    const std::string& directory_resource_id,
    const GetResourceListCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!directory_resource_id.empty());
  DCHECK(!callback.is_null());

  // Because children.list method on Drive API v2 returns only the list of
  // children's references, but we need all file resource list.
  // So, here we use files.list method instead, with setting parents query.
  // After the migration from GData WAPI to Drive API v2, we should clean the
  // code up by moving the resposibility to include "parents" in the query
  // to client side.
  // We aren't interested in files in trash in this context, neither.
  return sender_->StartRequestWithRetry(
      new GetFilelistRequest(
          sender_.get(),
          url_request_context_getter_,
          url_generator_,
          base::StringPrintf(
              "'%s' in parents and trashed = false",
              drive::util::EscapeQueryStringValue(
                  directory_resource_id).c_str()),
          kMaxNumFilesResourcePerRequest,
          base::Bind(&ParseResourceListOnBlockingPoolAndRun, callback)));
}

CancelCallback DriveAPIService::Search(
    const std::string& search_query,
    const GetResourceListCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!search_query.empty());
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(
      new GetFilelistRequest(
          sender_.get(),
          url_request_context_getter_,
          url_generator_,
          drive::util::TranslateQuery(search_query),
          kMaxNumFilesResourcePerRequestForSearch,
          base::Bind(&ParseResourceListOnBlockingPoolAndRun, callback)));
}

CancelCallback DriveAPIService::SearchByTitle(
    const std::string& title,
    const std::string& directory_resource_id,
    const GetResourceListCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!title.empty());
  DCHECK(!callback.is_null());

  std::string query;
  base::StringAppendF(&query, "title = '%s'",
                      drive::util::EscapeQueryStringValue(title).c_str());
  if (!directory_resource_id.empty()) {
    base::StringAppendF(
        &query, " and '%s' in parents",
        drive::util::EscapeQueryStringValue(directory_resource_id).c_str());
  }
  query += " and trashed = false";

  return sender_->StartRequestWithRetry(
      new GetFilelistRequest(
          sender_.get(),
          url_request_context_getter_,
          url_generator_,
          query,
          kMaxNumFilesResourcePerRequest,
          base::Bind(&ParseResourceListOnBlockingPoolAndRun, callback)));
}

CancelCallback DriveAPIService::GetChangeList(
    int64 start_changestamp,
    const GetResourceListCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(
      new GetChangelistRequest(
          sender_.get(),
          url_request_context_getter_,
          url_generator_,
          true,  // include deleted
          start_changestamp,
          kMaxNumFilesResourcePerRequest,
          base::Bind(&ParseResourceListOnBlockingPoolAndRun, callback)));
}

CancelCallback DriveAPIService::ContinueGetResourceList(
    const GURL& override_url,
    const GetResourceListCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(
      new drive::ContinueGetFileListRequest(
          sender_.get(),
          url_request_context_getter_,
          override_url,
          base::Bind(&ParseResourceListOnBlockingPoolAndRun, callback)));
}

CancelCallback DriveAPIService::GetResourceEntry(
    const std::string& resource_id,
    const GetResourceEntryCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(new GetFileRequest(
      sender_.get(),
      url_request_context_getter_,
      url_generator_,
      resource_id,
      base::Bind(&ParseResourceEntryAndRun, callback)));
}

CancelCallback DriveAPIService::GetAboutResource(
    const GetAboutResourceCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(
      new GetAboutRequest(
          sender_.get(),
          url_request_context_getter_,
          url_generator_,
          callback));
}

CancelCallback DriveAPIService::GetAppList(const GetAppListCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(new GetApplistRequest(
      sender_.get(),
      url_request_context_getter_,
      url_generator_,
      base::Bind(&ParseAppListAndRun, callback)));
}

CancelCallback DriveAPIService::DownloadFile(
    const base::FilePath& virtual_path,
    const base::FilePath& local_cache_path,
    const GURL& download_url,
    const DownloadActionCallback& download_action_callback,
    const GetContentCallback& get_content_callback,
    const ProgressCallback& progress_callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!download_action_callback.is_null());
  // get_content_callback may be null.

  return sender_->StartRequestWithRetry(
      new DownloadFileRequest(sender_.get(),
                              url_request_context_getter_,
                              download_action_callback,
                              get_content_callback,
                              progress_callback,
                              download_url,
                              virtual_path,
                              local_cache_path));
}

CancelCallback DriveAPIService::DeleteResource(
    const std::string& resource_id,
    const std::string& etag,
    const EntryActionCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(new drive::TrashResourceRequest(
      sender_.get(),
      url_request_context_getter_,
      url_generator_,
      resource_id,
      callback));
}

CancelCallback DriveAPIService::AddNewDirectory(
    const std::string& parent_resource_id,
    const std::string& directory_name,
    const GetResourceEntryCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(
      new drive::CreateDirectoryRequest(
          sender_.get(),
          url_request_context_getter_,
          url_generator_,
          parent_resource_id,
          directory_name,
          base::Bind(&ParseResourceEntryAndRun, callback)));
}

CancelCallback DriveAPIService::CopyResource(
    const std::string& resource_id,
    const std::string& parent_resource_id,
    const std::string& new_name,
    const GetResourceEntryCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(
      new drive::CopyResourceRequest(
          sender_.get(),
          url_request_context_getter_,
          url_generator_,
          resource_id,
          parent_resource_id,
          new_name,
          base::Bind(&ParseResourceEntryAndRun, callback)));
}

CancelCallback DriveAPIService::CopyHostedDocument(
    const std::string& resource_id,
    const std::string& new_name,
    const GetResourceEntryCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(
      new drive::CopyResourceRequest(
          sender_.get(),
          url_request_context_getter_,
          url_generator_,
          resource_id,
          std::string(),  // parent_resource_id.
          new_name,
          base::Bind(&ParseResourceEntryAndRun, callback)));
}

CancelCallback DriveAPIService::RenameResource(
    const std::string& resource_id,
    const std::string& new_name,
    const EntryActionCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(
      new drive::RenameResourceRequest(
          sender_.get(),
          url_request_context_getter_,
          url_generator_,
          resource_id,
          new_name,
          callback));
}

CancelCallback DriveAPIService::TouchResource(
    const std::string& resource_id,
    const base::Time& modified_date,
    const base::Time& last_viewed_by_me_date,
    const GetResourceEntryCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!modified_date.is_null());
  DCHECK(!last_viewed_by_me_date.is_null());
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(
      new drive::TouchResourceRequest(
          sender_.get(),
          url_request_context_getter_,
          url_generator_,
          resource_id,
          modified_date,
          last_viewed_by_me_date,
          base::Bind(&ParseResourceEntryAndRun, callback)));
}

CancelCallback DriveAPIService::AddResourceToDirectory(
    const std::string& parent_resource_id,
    const std::string& resource_id,
    const EntryActionCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(
      new drive::InsertResourceRequest(
          sender_.get(),
          url_request_context_getter_,
          url_generator_,
          parent_resource_id,
          resource_id,
          callback));
}

CancelCallback DriveAPIService::RemoveResourceFromDirectory(
    const std::string& parent_resource_id,
    const std::string& resource_id,
    const EntryActionCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(
      new drive::DeleteResourceRequest(
          sender_.get(),
          url_request_context_getter_,
          url_generator_,
          parent_resource_id,
          resource_id,
          callback));
}

CancelCallback DriveAPIService::InitiateUploadNewFile(
    const base::FilePath& drive_file_path,
    const std::string& content_type,
    int64 content_length,
    const std::string& parent_resource_id,
    const std::string& title,
    const InitiateUploadCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(
      new drive::InitiateUploadNewFileRequest(
          sender_.get(),
          url_request_context_getter_,
          url_generator_,
          drive_file_path,
          content_type,
          content_length,
          parent_resource_id,
          title,
          callback));
}

CancelCallback DriveAPIService::InitiateUploadExistingFile(
    const base::FilePath& drive_file_path,
    const std::string& content_type,
    int64 content_length,
    const std::string& resource_id,
    const std::string& etag,
    const InitiateUploadCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(
      new drive::InitiateUploadExistingFileRequest(
          sender_.get(),
          url_request_context_getter_,
          url_generator_,
          drive_file_path,
          content_type,
          content_length,
          resource_id,
          etag,
          callback));
}

CancelCallback DriveAPIService::ResumeUpload(
    const base::FilePath& drive_file_path,
    const GURL& upload_url,
    int64 start_position,
    int64 end_position,
    int64 content_length,
    const std::string& content_type,
    const base::FilePath& local_file_path,
    const UploadRangeCallback& callback,
    const ProgressCallback& progress_callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(
      new drive::ResumeUploadRequest(
          sender_.get(),
          url_request_context_getter_,
          drive_file_path,
          upload_url,
          start_position,
          end_position,
          content_length,
          content_type,
          local_file_path,
          base::Bind(&ParseResourceEntryForUploadRangeAndRun, callback),
          progress_callback));
}

CancelCallback DriveAPIService::GetUploadStatus(
    const base::FilePath& drive_file_path,
    const GURL& upload_url,
    int64 content_length,
    const UploadRangeCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(new drive::GetUploadStatusRequest(
      sender_.get(),
      url_request_context_getter_,
      drive_file_path,
      upload_url,
      content_length,
      base::Bind(&ParseResourceEntryForUploadRangeAndRun, callback)));
}

CancelCallback DriveAPIService::AuthorizeApp(
    const std::string& resource_id,
    const std::string& app_id,
    const AuthorizeAppCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  return sender_->StartRequestWithRetry(new GetFileRequest(
      sender_.get(),
      url_request_context_getter_,
      url_generator_,
      resource_id,
      base::Bind(&ExtractOpenUrlAndRun, app_id, callback)));
}

bool DriveAPIService::HasAccessToken() const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  return sender_->auth_service()->HasAccessToken();
}

bool DriveAPIService::HasRefreshToken() const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  return sender_->auth_service()->HasRefreshToken();
}

void DriveAPIService::ClearAccessToken() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  return sender_->auth_service()->ClearAccessToken();
}

void DriveAPIService::ClearRefreshToken() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  return sender_->auth_service()->ClearRefreshToken();
}

void DriveAPIService::OnOAuth2RefreshTokenChanged() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (CanSendRequest()) {
    FOR_EACH_OBSERVER(
        DriveServiceObserver, observers_, OnReadyToSendRequests());
  } else if (!HasRefreshToken()) {
    FOR_EACH_OBSERVER(
        DriveServiceObserver, observers_, OnRefreshTokenInvalid());
  }
}

}  // namespace google_apis
