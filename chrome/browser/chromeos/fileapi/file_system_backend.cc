// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/file_system_backend.h"

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/chromeos/fileapi/file_access_permissions.h"
#include "chrome/browser/chromeos/fileapi/file_system_backend_delegate.h"
#include "chrome/browser/media_galleries/fileapi/media_file_system_backend.h"
#include "chrome/common/url_constants.h"
#include "chromeos/dbus/cros_disks_client.h"
#include "storage/browser/fileapi/async_file_util.h"
#include "storage/browser/fileapi/external_mount_points.h"
#include "storage/browser/fileapi/file_stream_reader.h"
#include "storage/browser/fileapi/file_stream_writer.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_operation.h"
#include "storage/browser/fileapi/file_system_operation_context.h"
#include "storage/browser/fileapi/file_system_url.h"
#include "storage/common/fileapi/file_system_mount_option.h"

namespace chromeos {

// static
bool FileSystemBackend::CanHandleURL(const storage::FileSystemURL& url) {
  if (!url.is_valid())
    return false;
  return url.type() == storage::kFileSystemTypeNativeLocal ||
         url.type() == storage::kFileSystemTypeRestrictedNativeLocal ||
         url.type() == storage::kFileSystemTypeDrive ||
         url.type() == storage::kFileSystemTypeProvided ||
         url.type() == storage::kFileSystemTypeDeviceMediaAsFileStorage;
}

FileSystemBackend::FileSystemBackend(
    FileSystemBackendDelegate* drive_delegate,
    FileSystemBackendDelegate* file_system_provider_delegate,
    FileSystemBackendDelegate* mtp_delegate,
    scoped_refptr<storage::ExternalMountPoints> mount_points,
    storage::ExternalMountPoints* system_mount_points)
    : file_access_permissions_(new FileAccessPermissions()),
      local_file_util_(storage::AsyncFileUtil::CreateForLocalFileSystem()),
      drive_delegate_(drive_delegate),
      file_system_provider_delegate_(file_system_provider_delegate),
      mtp_delegate_(mtp_delegate),
      mount_points_(mount_points),
      system_mount_points_(system_mount_points) {
}

FileSystemBackend::~FileSystemBackend() {
}

void FileSystemBackend::AddSystemMountPoints() {
  // RegisterFileSystem() is no-op if the mount point with the same name
  // already exists, hence it's safe to call without checking if a mount
  // point already exists or not.
  system_mount_points_->RegisterFileSystem(
      "archive",
      storage::kFileSystemTypeNativeLocal,
      storage::FileSystemMountOption(),
      chromeos::CrosDisksClient::GetArchiveMountPoint());
  system_mount_points_->RegisterFileSystem(
      "removable", storage::kFileSystemTypeNativeLocal,
      storage::FileSystemMountOption(storage::FlushPolicy::FLUSH_ON_COMPLETION),
      chromeos::CrosDisksClient::GetRemovableDiskMountPoint());
  system_mount_points_->RegisterFileSystem(
      "oem",
      storage::kFileSystemTypeRestrictedNativeLocal,
      storage::FileSystemMountOption(),
      base::FilePath(FILE_PATH_LITERAL("/usr/share/oem")));
}

bool FileSystemBackend::CanHandleType(storage::FileSystemType type) const {
  switch (type) {
    case storage::kFileSystemTypeExternal:
    case storage::kFileSystemTypeDrive:
    case storage::kFileSystemTypeRestrictedNativeLocal:
    case storage::kFileSystemTypeNativeLocal:
    case storage::kFileSystemTypeNativeForPlatformApp:
    case storage::kFileSystemTypeDeviceMediaAsFileStorage:
    case storage::kFileSystemTypeProvided:
      return true;
    default:
      return false;
  }
}

void FileSystemBackend::Initialize(storage::FileSystemContext* context) {
}

void FileSystemBackend::ResolveURL(const storage::FileSystemURL& url,
                                   storage::OpenFileSystemMode mode,
                                   const OpenFileSystemCallback& callback) {
  std::string id;
  storage::FileSystemType type;
  std::string cracked_id;
  base::FilePath path;
  storage::FileSystemMountOption option;
  if (!mount_points_->CrackVirtualPath(
           url.virtual_path(), &id, &type, &cracked_id, &path, &option) &&
      !system_mount_points_->CrackVirtualPath(
           url.virtual_path(), &id, &type, &cracked_id, &path, &option)) {
    // Not under a mount point, so return an error, since the root is not
    // accessible.
    GURL root_url = GURL(storage::GetExternalFileSystemRootURIString(
        url.origin(), std::string()));
    callback.Run(root_url, std::string(), base::File::FILE_ERROR_SECURITY);
    return;
  }

  std::string name;
  // Construct a URL restricted to the found mount point.
  std::string root_url =
      storage::GetExternalFileSystemRootURIString(url.origin(), id);

  // For removable and archives, the file system root is the external mount
  // point plus the inner mount point.
  if (id == "archive" || id == "removable") {
    std::vector<std::string> components;
    url.virtual_path().GetComponents(&components);
    DCHECK_EQ(id, components.at(0));
    if (components.size() < 2) {
      // Unable to access /archive and /removable directories directly. The
      // inner mount name must be specified.
      callback.Run(
          GURL(root_url), std::string(), base::File::FILE_ERROR_SECURITY);
      return;
    }
    std::string inner_mount_name = components[1];
    root_url += inner_mount_name + "/";
    name = inner_mount_name;
  } else {
    name = id;
  }

  callback.Run(GURL(root_url), name, base::File::FILE_OK);
}

storage::FileSystemQuotaUtil* FileSystemBackend::GetQuotaUtil() {
  // No quota support.
  return NULL;
}

const storage::UpdateObserverList* FileSystemBackend::GetUpdateObservers(
    storage::FileSystemType type) const {
  return NULL;
}

const storage::ChangeObserverList* FileSystemBackend::GetChangeObservers(
    storage::FileSystemType type) const {
  return NULL;
}

const storage::AccessObserverList* FileSystemBackend::GetAccessObservers(
    storage::FileSystemType type) const {
  return NULL;
}

bool FileSystemBackend::IsAccessAllowed(
    const storage::FileSystemURL& url) const {
  if (!url.is_valid())
    return false;

  // No extra check is needed for isolated file systems.
  if (url.mount_type() == storage::kFileSystemTypeIsolated)
    return true;

  if (!CanHandleURL(url))
    return false;

  // If there is no origin set, then it's an internal access.
  if (url.origin().is_empty())
    return true;

  std::string extension_id = url.origin().host();
  // TODO(mtomasz): Temporarily whitelist TimeScapes. Remove this in M-31.
  // See: crbug.com/271946
  if (extension_id == "mlbmkoenclnokonejhlfakkeabdlmpek" &&
      url.type() == storage::kFileSystemTypeRestrictedNativeLocal) {
    return true;
  }

  return file_access_permissions_->HasAccessPermission(extension_id,
                                                       url.virtual_path());
}

void FileSystemBackend::GrantFileAccessToExtension(
    const std::string& extension_id, const base::FilePath& virtual_path) {
  std::string id;
  storage::FileSystemType type;
  std::string cracked_id;
  base::FilePath path;
  storage::FileSystemMountOption option;
  if (!mount_points_->CrackVirtualPath(virtual_path, &id, &type, &cracked_id,
                                       &path, &option) &&
      !system_mount_points_->CrackVirtualPath(virtual_path, &id, &type,
                                              &cracked_id, &path, &option)) {
    return;
  }

  file_access_permissions_->GrantAccessPermission(extension_id, virtual_path);
}

void FileSystemBackend::RevokeAccessForExtension(
      const std::string& extension_id) {
  file_access_permissions_->RevokePermissions(extension_id);
}

std::vector<base::FilePath> FileSystemBackend::GetRootDirectories() const {
  std::vector<storage::MountPoints::MountPointInfo> mount_points;
  mount_points_->AddMountPointInfosTo(&mount_points);
  system_mount_points_->AddMountPointInfosTo(&mount_points);

  std::vector<base::FilePath> root_dirs;
  for (size_t i = 0; i < mount_points.size(); ++i)
    root_dirs.push_back(mount_points[i].path);
  return root_dirs;
}

storage::AsyncFileUtil* FileSystemBackend::GetAsyncFileUtil(
    storage::FileSystemType type) {
  switch (type) {
    case storage::kFileSystemTypeDrive:
      return drive_delegate_->GetAsyncFileUtil(type);
    case storage::kFileSystemTypeProvided:
      return file_system_provider_delegate_->GetAsyncFileUtil(type);
    case storage::kFileSystemTypeNativeLocal:
    case storage::kFileSystemTypeRestrictedNativeLocal:
      return local_file_util_.get();
    case storage::kFileSystemTypeDeviceMediaAsFileStorage:
      return mtp_delegate_->GetAsyncFileUtil(type);
    default:
      NOTREACHED();
  }
  return NULL;
}

storage::WatcherManager* FileSystemBackend::GetWatcherManager(
    storage::FileSystemType type) {
  if (type == storage::kFileSystemTypeProvided)
    return file_system_provider_delegate_->GetWatcherManager(type);

  // TODO(mtomasz): Add support for other backends.
  return NULL;
}

storage::CopyOrMoveFileValidatorFactory*
FileSystemBackend::GetCopyOrMoveFileValidatorFactory(
    storage::FileSystemType type,
    base::File::Error* error_code) {
  DCHECK(error_code);
  *error_code = base::File::FILE_OK;
  return NULL;
}

storage::FileSystemOperation* FileSystemBackend::CreateFileSystemOperation(
    const storage::FileSystemURL& url,
    storage::FileSystemContext* context,
    base::File::Error* error_code) const {
  DCHECK(url.is_valid());

  if (!IsAccessAllowed(url)) {
    *error_code = base::File::FILE_ERROR_SECURITY;
    return NULL;
  }

  if (url.type() == storage::kFileSystemTypeDeviceMediaAsFileStorage) {
    // MTP file operations run on MediaTaskRunner.
    return storage::FileSystemOperation::Create(
        url,
        context,
        make_scoped_ptr(new storage::FileSystemOperationContext(
            context, MediaFileSystemBackend::MediaTaskRunner().get())));
  }

  DCHECK(url.type() == storage::kFileSystemTypeNativeLocal ||
         url.type() == storage::kFileSystemTypeRestrictedNativeLocal ||
         url.type() == storage::kFileSystemTypeDrive ||
         url.type() == storage::kFileSystemTypeProvided);
  return storage::FileSystemOperation::Create(
      url,
      context,
      make_scoped_ptr(new storage::FileSystemOperationContext(context)));
}

bool FileSystemBackend::SupportsStreaming(
    const storage::FileSystemURL& url) const {
  return url.type() == storage::kFileSystemTypeDrive ||
         url.type() == storage::kFileSystemTypeProvided ||
         url.type() == storage::kFileSystemTypeDeviceMediaAsFileStorage;
}

bool FileSystemBackend::HasInplaceCopyImplementation(
    storage::FileSystemType type) const {
  switch (type) {
    case storage::kFileSystemTypeDrive:
    case storage::kFileSystemTypeProvided:
    case storage::kFileSystemTypeDeviceMediaAsFileStorage:
      return true;
    case storage::kFileSystemTypeNativeLocal:
    case storage::kFileSystemTypeRestrictedNativeLocal:
      return false;
    default:
      NOTREACHED();
  }
  return true;
}

scoped_ptr<storage::FileStreamReader> FileSystemBackend::CreateFileStreamReader(
    const storage::FileSystemURL& url,
    int64 offset,
    int64 max_bytes_to_read,
    const base::Time& expected_modification_time,
    storage::FileSystemContext* context) const {
  DCHECK(url.is_valid());

  if (!IsAccessAllowed(url))
    return scoped_ptr<storage::FileStreamReader>();

  switch (url.type()) {
    case storage::kFileSystemTypeDrive:
      return drive_delegate_->CreateFileStreamReader(
          url, offset, max_bytes_to_read, expected_modification_time, context);
    case storage::kFileSystemTypeProvided:
      return file_system_provider_delegate_->CreateFileStreamReader(
          url, offset, max_bytes_to_read, expected_modification_time, context);
    case storage::kFileSystemTypeNativeLocal:
    case storage::kFileSystemTypeRestrictedNativeLocal:
      return scoped_ptr<storage::FileStreamReader>(
          storage::FileStreamReader::CreateForFileSystemFile(
              context, url, offset, expected_modification_time));
    case storage::kFileSystemTypeDeviceMediaAsFileStorage:
      return mtp_delegate_->CreateFileStreamReader(
          url, offset, max_bytes_to_read, expected_modification_time, context);
    default:
      NOTREACHED();
  }
  return scoped_ptr<storage::FileStreamReader>();
}

scoped_ptr<storage::FileStreamWriter> FileSystemBackend::CreateFileStreamWriter(
    const storage::FileSystemURL& url,
    int64 offset,
    storage::FileSystemContext* context) const {
  DCHECK(url.is_valid());

  if (!IsAccessAllowed(url))
    return scoped_ptr<storage::FileStreamWriter>();

  switch (url.type()) {
    case storage::kFileSystemTypeDrive:
      return drive_delegate_->CreateFileStreamWriter(url, offset, context);
    case storage::kFileSystemTypeProvided:
      return file_system_provider_delegate_->CreateFileStreamWriter(
          url, offset, context);
    case storage::kFileSystemTypeNativeLocal:
      return scoped_ptr<storage::FileStreamWriter>(
          storage::FileStreamWriter::CreateForLocalFile(
              context->default_file_task_runner(),
              url.path(),
              offset,
              storage::FileStreamWriter::OPEN_EXISTING_FILE));
    case storage::kFileSystemTypeRestrictedNativeLocal:
      // Restricted native local file system is read only.
      return scoped_ptr<storage::FileStreamWriter>();
    case storage::kFileSystemTypeDeviceMediaAsFileStorage:
      return mtp_delegate_->CreateFileStreamWriter(url, offset, context);
    default:
      NOTREACHED();
  }
  return scoped_ptr<storage::FileStreamWriter>();
}

bool FileSystemBackend::GetVirtualPath(const base::FilePath& filesystem_path,
                                       base::FilePath* virtual_path) const {
  return mount_points_->GetVirtualPath(filesystem_path, virtual_path) ||
         system_mount_points_->GetVirtualPath(filesystem_path, virtual_path);
}

void FileSystemBackend::GetRedirectURLForContents(
    const storage::FileSystemURL& url,
    const storage::URLCallback& callback) const {
  DCHECK(url.is_valid());

  if (!IsAccessAllowed(url))
    return callback.Run(GURL());

  switch (url.type()) {
    case storage::kFileSystemTypeDrive:
      drive_delegate_->GetRedirectURLForContents(url, callback);
      return;
    case storage::kFileSystemTypeProvided:
      file_system_provider_delegate_->GetRedirectURLForContents(url,
                                                                  callback);
      return;
    case storage::kFileSystemTypeDeviceMediaAsFileStorage:
      mtp_delegate_->GetRedirectURLForContents(url, callback);
      return;
    case storage::kFileSystemTypeNativeLocal:
    case storage::kFileSystemTypeRestrictedNativeLocal:
      callback.Run(GURL());
      return;
    default:
      NOTREACHED();
  }
  callback.Run(GURL());
}

storage::FileSystemURL FileSystemBackend::CreateInternalURL(
    storage::FileSystemContext* context,
    const base::FilePath& entry_path) const {
  base::FilePath virtual_path;
  if (!GetVirtualPath(entry_path, &virtual_path))
    return storage::FileSystemURL();

  return context->CreateCrackedFileSystemURL(
      GURL() /* origin */, storage::kFileSystemTypeExternal, virtual_path);
}

}  // namespace chromeos
