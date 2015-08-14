// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/file_manager/private_api_strings.h"

#include "base/strings/stringprintf.h"
#include "base/sys_info.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/file_manager/open_with_browser.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/system/statistics_provider.h"
#include "extensions/common/extension_l10n_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/strings/grit/app_locale_settings.h"

namespace {

// Location of the page to buy more storage for Google Drive.
const char kGoogleDriveBuyStorageUrl[] =
    "https://www.google.com/settings/storage";

// Location of the overview page about Google Drive.
const char kGoogleDriveOverviewUrl[] =
    "https://support.google.com/chromebook/?p=filemanager_drive";

// Location of Google drive redeem page.
const char kGoogleDriveRedeemUrl[] =
    "http://www.google.com/intl/en/chrome/devices/goodies.html"
    "?utm_source=filesapp&utm_medium=banner&utm_campaign=gsg";

// Location of Google Drive specific help.
const char kGoogleDriveHelpUrl[] =
    "https://support.google.com/chromebook/?p=filemanager_drivehelp";

// Location of Google Drive root.
const char kGoogleDriveRootUrl[] = "https://drive.google.com";

// Printf format
const char kHelpURLFormat[] = "https://support.google.com/chromebook/answer/%d";

// Location of the help page for low space warning in the downloads directory.
const int kDownloadsLowSpaceWarningHelpNumber = 1061547;

// Location of Files App specific help.
const int kFilesAppHelpNumber = 1056323;

// Location of the help page about connecting to Google Drive.
const int kGoogleDriveErrorHelpNumber = 2649458;

// Location of the help page about no-action-available files.
const int kNoActionForFileHelpNumber = 1700055;

#define SET_STRING(id, idr) dict->SetString(id, l10n_util::GetStringUTF16(idr))

void AddStringsForFileTypes(base::DictionaryValue* dict) {
  // TODO(crbug.com/438921): Rename string IDs to something like
  // FILE_TYPE_WHATEVER.
  SET_STRING("AUDIO_FILE_TYPE", IDS_FILE_BROWSER_AUDIO_FILE_TYPE);
  SET_STRING("EXCEL_FILE_TYPE", IDS_FILE_BROWSER_EXCEL_FILE_TYPE);
  SET_STRING("FOLDER", IDS_FILE_BROWSER_FOLDER);
  SET_STRING("GDOC_DOCUMENT_FILE_TYPE",
             IDS_FILE_BROWSER_GDOC_DOCUMENT_FILE_TYPE);
  SET_STRING("GDRAW_DOCUMENT_FILE_TYPE",
             IDS_FILE_BROWSER_GDRAW_DOCUMENT_FILE_TYPE);
  SET_STRING("GENERIC_FILE_TYPE", IDS_FILE_BROWSER_GENERIC_FILE_TYPE);
  SET_STRING("GFORM_DOCUMENT_FILE_TYPE",
             IDS_FILE_BROWSER_GFORM_DOCUMENT_FILE_TYPE);
  SET_STRING("GLINK_DOCUMENT_FILE_TYPE",
             IDS_FILE_BROWSER_GLINK_DOCUMENT_FILE_TYPE);
  SET_STRING("GMAP_DOCUMENT_FILE_TYPE",
             IDS_FILE_BROWSER_GMAP_DOCUMENT_FILE_TYPE);
  SET_STRING("GSHEET_DOCUMENT_FILE_TYPE",
             IDS_FILE_BROWSER_GSHEET_DOCUMENT_FILE_TYPE);
  SET_STRING("GSLIDES_DOCUMENT_FILE_TYPE",
             IDS_FILE_BROWSER_GSLIDES_DOCUMENT_FILE_TYPE);
  SET_STRING("GTABLE_DOCUMENT_FILE_TYPE",
             IDS_FILE_BROWSER_GTABLE_DOCUMENT_FILE_TYPE);
  SET_STRING("HTML_DOCUMENT_FILE_TYPE",
             IDS_FILE_BROWSER_HTML_DOCUMENT_FILE_TYPE);
  SET_STRING("IMAGE_FILE_TYPE", IDS_FILE_BROWSER_IMAGE_FILE_TYPE);
  SET_STRING("NO_EXTENSION_FILE_TYPE", IDS_FILE_BROWSER_NO_EXTENSION_FILE_TYPE);
  SET_STRING("PDF_DOCUMENT_FILE_TYPE", IDS_FILE_BROWSER_PDF_DOCUMENT_FILE_TYPE);
  SET_STRING("PLAIN_TEXT_FILE_TYPE", IDS_FILE_BROWSER_PLAIN_TEXT_FILE_TYPE);
  SET_STRING("POWERPOINT_PRESENTATION_FILE_TYPE",
             IDS_FILE_BROWSER_POWERPOINT_PRESENTATION_FILE_TYPE);
  SET_STRING("RAR_ARCHIVE_FILE_TYPE", IDS_FILE_BROWSER_RAR_ARCHIVE_FILE_TYPE);
  SET_STRING("TAR_ARCHIVE_FILE_TYPE", IDS_FILE_BROWSER_TAR_ARCHIVE_FILE_TYPE);
  SET_STRING("TAR_BZIP2_ARCHIVE_FILE_TYPE",
             IDS_FILE_BROWSER_TAR_BZIP2_ARCHIVE_FILE_TYPE);
  SET_STRING("TAR_GZIP_ARCHIVE_FILE_TYPE",
             IDS_FILE_BROWSER_TAR_GZIP_ARCHIVE_FILE_TYPE);
  SET_STRING("VIDEO_FILE_TYPE", IDS_FILE_BROWSER_VIDEO_FILE_TYPE);
  SET_STRING("WORD_DOCUMENT_FILE_TYPE",
             IDS_FILE_BROWSER_WORD_DOCUMENT_FILE_TYPE);
  SET_STRING("ZIP_ARCHIVE_FILE_TYPE", IDS_FILE_BROWSER_ZIP_ARCHIVE_FILE_TYPE);
}

void AddStringsForDrive(base::DictionaryValue* dict) {
  SET_STRING("DRIVE_BUY_MORE_SPACE", IDS_FILE_BROWSER_DRIVE_BUY_MORE_SPACE);
  SET_STRING("DRIVE_BUY_MORE_SPACE_LINK",
             IDS_FILE_BROWSER_DRIVE_BUY_MORE_SPACE_LINK);
  SET_STRING("DRIVE_CANNOT_REACH", IDS_FILE_BROWSER_DRIVE_CANNOT_REACH);
  SET_STRING("DRIVE_DIRECTORY_LABEL", IDS_FILE_BROWSER_DRIVE_DIRECTORY_LABEL);
  SET_STRING("DRIVE_LEARN_MORE", IDS_FILE_BROWSER_DRIVE_LEARN_MORE);
  SET_STRING("DRIVE_MENU_HELP", IDS_FILE_BROWSER_DRIVE_MENU_HELP);
  SET_STRING("DRIVE_MOBILE_CONNECTION_OPTION",
             IDS_FILE_BROWSER_DRIVE_MOBILE_CONNECTION_OPTION);
  SET_STRING("DRIVE_MY_DRIVE_LABEL", IDS_FILE_BROWSER_DRIVE_MY_DRIVE_LABEL);
  SET_STRING("DRIVE_NOT_REACHED", IDS_FILE_BROWSER_DRIVE_NOT_REACHED);
  SET_STRING("DRIVE_OFFLINE_COLLECTION_LABEL",
             IDS_FILE_BROWSER_DRIVE_OFFLINE_COLLECTION_LABEL);
  SET_STRING("DRIVE_OUT_OF_SPACE_HEADER",
             IDS_FILE_BROWSER_DRIVE_OUT_OF_SPACE_HEADER);
  SET_STRING("DRIVE_OUT_OF_SPACE_MESSAGE",
             IDS_FILE_BROWSER_DRIVE_OUT_OF_SPACE_MESSAGE);
  SET_STRING("DRIVE_RECENT_COLLECTION_LABEL",
             IDS_FILE_BROWSER_DRIVE_RECENT_COLLECTION_LABEL);
  SET_STRING("DRIVE_SHARED_WITH_ME_COLLECTION_LABEL",
             IDS_FILE_BROWSER_DRIVE_SHARED_WITH_ME_COLLECTION_LABEL);
  SET_STRING("DRIVE_SHARE_TYPE_CAN_COMMENT",
             IDS_FILE_BROWSER_DRIVE_SHARE_TYPE_CAN_COMMENT);
  SET_STRING("DRIVE_SHARE_TYPE_CAN_EDIT",
             IDS_FILE_BROWSER_DRIVE_SHARE_TYPE_CAN_EDIT);
  SET_STRING("DRIVE_SHARE_TYPE_CAN_VIEW",
             IDS_FILE_BROWSER_DRIVE_SHARE_TYPE_CAN_VIEW);
  SET_STRING("DRIVE_SHOW_HOSTED_FILES_OPTION",
             IDS_FILE_BROWSER_DRIVE_SHOW_HOSTED_FILES_OPTION);
  SET_STRING("DRIVE_SPACE_AVAILABLE_LONG",
             IDS_FILE_BROWSER_DRIVE_SPACE_AVAILABLE_LONG);
  SET_STRING("DRIVE_VISIT_DRIVE_GOOGLE_COM",
             IDS_FILE_BROWSER_DRIVE_VISIT_DRIVE_GOOGLE_COM);
  SET_STRING("DRIVE_WELCOME_CHECK_ELIGIBILITY",
             IDS_FILE_BROWSER_DRIVE_WELCOME_CHECK_ELIGIBILITY);
  SET_STRING("DRIVE_WELCOME_DISMISS", IDS_FILE_BROWSER_DRIVE_WELCOME_DISMISS);
  SET_STRING("DRIVE_WELCOME_TEXT_LONG",
             IDS_FILE_BROWSER_DRIVE_WELCOME_TEXT_LONG);
  SET_STRING("DRIVE_WELCOME_TEXT_SHORT",
             IDS_FILE_BROWSER_DRIVE_WELCOME_TEXT_SHORT);
  SET_STRING("DRIVE_WELCOME_TITLE", IDS_FILE_BROWSER_DRIVE_WELCOME_TITLE);
  SET_STRING("DRIVE_WELCOME_TITLE_ALTERNATIVE",
             IDS_FILE_BROWSER_DRIVE_WELCOME_TITLE_ALTERNATIVE);
  SET_STRING("DRIVE_WELCOME_TITLE_ALTERNATIVE_1TB",
             IDS_FILE_BROWSER_DRIVE_WELCOME_TITLE_ALTERNATIVE_1TB);
  SET_STRING("SYNC_DELETE_WITHOUT_PERMISSION_ERROR",
             IDS_FILE_BROWSER_SYNC_DELETE_WITHOUT_PERMISSION_ERROR);
  SET_STRING("SYNC_FILE_NAME", IDS_FILE_BROWSER_SYNC_FILE_NAME);
  SET_STRING("SYNC_FILE_NUMBER", IDS_FILE_BROWSER_SYNC_FILE_NUMBER);
  SET_STRING("SYNC_MISC_ERROR", IDS_FILE_BROWSER_SYNC_MISC_ERROR);
  SET_STRING("SYNC_PROGRESS_SUMMARY", IDS_FILE_BROWSER_SYNC_PROGRESS_SUMMARY);
  SET_STRING("SYNC_SERVICE_UNAVAILABLE_ERROR",
             IDS_FILE_BROWSER_SYNC_SERVICE_UNAVAILABLE_ERROR);
}

void AddStringsForGallery(base::DictionaryValue* dict) {
  SET_STRING("GALLERY_ASPECT_RATIO_16_9",
             IDS_FILE_BROWSER_GALLERY_ASPECT_RATIO_16_9);
  SET_STRING("GALLERY_ASPECT_RATIO_1_1",
             IDS_FILE_BROWSER_GALLERY_ASPECT_RATIO_1_1);
  SET_STRING("GALLERY_ASPECT_RATIO_6_4",
             IDS_FILE_BROWSER_GALLERY_ASPECT_RATIO_6_4);
  SET_STRING("GALLERY_ASPECT_RATIO_7_5",
             IDS_FILE_BROWSER_GALLERY_ASPECT_RATIO_7_5);
  SET_STRING("GALLERY_AUTOFIX", IDS_FILE_BROWSER_GALLERY_AUTOFIX);
  SET_STRING("GALLERY_BRIGHTNESS", IDS_FILE_BROWSER_GALLERY_BRIGHTNESS);
  SET_STRING("GALLERY_CANCEL_LABEL", IDS_FILE_BROWSER_CANCEL_LABEL);
  SET_STRING("GALLERY_CONFIRM_DELETE_ONE", IDS_FILE_BROWSER_CONFIRM_DELETE_ONE);
  SET_STRING("GALLERY_CONFIRM_DELETE_SOME",
             IDS_FILE_BROWSER_CONFIRM_DELETE_SOME);
  SET_STRING("GALLERY_CONTRAST", IDS_FILE_BROWSER_GALLERY_CONTRAST);
  SET_STRING("GALLERY_CROP", IDS_FILE_BROWSER_GALLERY_CROP);
  SET_STRING("GALLERY_DELETE", IDS_FILE_BROWSER_GALLERY_DELETE);
  SET_STRING("GALLERY_EDIT", IDS_FILE_BROWSER_GALLERY_EDIT);
  SET_STRING("GALLERY_EXIT", IDS_FILE_BROWSER_GALLERY_EXIT);
  SET_STRING("GALLERY_ENTER_WHEN_DONE",
             IDS_FILE_BROWSER_GALLERY_ENTER_WHEN_DONE);
  SET_STRING("GALLERY_EXPOSURE", IDS_FILE_BROWSER_GALLERY_EXPOSURE);
  SET_STRING("GALLERY_FILE_EXISTS", IDS_FILE_BROWSER_GALLERY_FILE_EXISTS);
  SET_STRING("GALLERY_FIXED", IDS_FILE_BROWSER_GALLERY_FIXED);
  SET_STRING("GALLERY_IMAGE_ERROR", IDS_FILE_BROWSER_GALLERY_IMAGE_ERROR);
  SET_STRING("GALLERY_IMAGE_OFFLINE", IDS_FILE_BROWSER_GALLERY_IMAGE_OFFLINE);
  SET_STRING("GALLERY_ITEMS_SELECTED", IDS_FILE_BROWSER_GALLERY_ITEMS_SELECTED);
  SET_STRING("GALLERY_NO_IMAGES", IDS_FILE_BROWSER_GALLERY_NO_IMAGES);
  SET_STRING("GALLERY_OK_LABEL", IDS_FILE_BROWSER_OK_LABEL);
  SET_STRING("GALLERY_OVERWRITE_BUBBLE",
             IDS_FILE_BROWSER_GALLERY_OVERWRITE_BUBBLE);
  SET_STRING("GALLERY_OVERWRITE_ORIGINAL",
             IDS_FILE_BROWSER_GALLERY_OVERWRITE_ORIGINAL);
  SET_STRING("GALLERY_PRINT", IDS_FILE_BROWSER_GALLERY_PRINT);
  SET_STRING("GALLERY_READONLY_WARNING",
             IDS_FILE_BROWSER_GALLERY_READONLY_WARNING);
  SET_STRING("GALLERY_NON_WRITABLE_FORMAT_WARNING",
             IDS_FILE_BROWSER_GALLERY_NON_WRITABLE_FORMAT_WARNING);
  SET_STRING("GALLERY_REDO", IDS_FILE_BROWSER_GALLERY_REDO);
  SET_STRING("GALLERY_ROTATE_LEFT", IDS_FILE_BROWSER_GALLERY_ROTATE_LEFT);
  SET_STRING("GALLERY_ROTATE_RIGHT", IDS_FILE_BROWSER_GALLERY_ROTATE_RIGHT);
  SET_STRING("GALLERY_SAVED", IDS_FILE_BROWSER_GALLERY_SAVED);
  SET_STRING("GALLERY_SAVE_FAILED", IDS_FILE_BROWSER_GALLERY_SAVE_FAILED);
  SET_STRING("GALLERY_SHARE", IDS_FILE_BROWSER_GALLERY_SHARE);
  SET_STRING("GALLERY_SLIDE", IDS_FILE_BROWSER_GALLERY_SLIDE);
  SET_STRING("GALLERY_SLIDESHOW", IDS_FILE_BROWSER_GALLERY_SLIDESHOW);
  SET_STRING("GALLERY_THUMBNAIL", IDS_FILE_BROWSER_GALLERY_THUMBNAIL);
  SET_STRING("GALLERY_UNDO", IDS_FILE_BROWSER_GALLERY_UNDO);
  SET_STRING("GALLERY_DONE", IDS_FILE_BROWSER_GALLERY_DONE);
}

void AddStringsForVideoPlayer(base::DictionaryValue* dict) {
  SET_STRING("VIDEO_PLAYER_LOOPED_MODE", IDS_VIDEO_PLAYER_LOOPED_MODE);
  SET_STRING("VIDEO_PLAYER_PLAYBACK_ERROR", IDS_VIDEO_PLAYER_PLAYBACK_ERROR);
  SET_STRING("VIDEO_PLAYER_PLAYING_ON", IDS_VIDEO_PLAYER_PLAYING_ON);
  SET_STRING("VIDEO_PLAYER_PLAY_ON", IDS_VIDEO_PLAYER_PLAY_ON);
  SET_STRING("VIDEO_PLAYER_PLAY_THIS_COMPUTER",
             IDS_VIDEO_PLAYER_PLAY_THIS_COMPUTER);
  SET_STRING("VIDEO_PLAYER_VIDEO_FILE_UNSUPPORTED",
             IDS_VIDEO_PLAYER_VIDEO_FILE_UNSUPPORTED);
  SET_STRING("VIDEO_PLAYER_VIDEO_FILE_UNSUPPORTED_FOR_CAST",
             IDS_VIDEO_PLAYER_VIDEO_FILE_UNSUPPORTED_FOR_CAST);
}

void AddStringsForAudioPlayer(base::DictionaryValue* dict) {
  SET_STRING("AUDIO_ERROR", IDS_FILE_BROWSER_AUDIO_ERROR);
  SET_STRING("AUDIO_OFFLINE", IDS_FILE_BROWSER_AUDIO_OFFLINE);
  SET_STRING("AUDIO_PLAYER_DEFAULT_ARTIST",
             IDS_FILE_BROWSER_AUDIO_PLAYER_DEFAULT_ARTIST);
  SET_STRING("AUDIO_PLAYER_TITLE", IDS_FILE_BROWSER_AUDIO_PLAYER_TITLE);
}

void AddStringsForCloudImport(base::DictionaryValue* dict) {
  SET_STRING("CLOUD_IMPORT_TITLE", IDS_FILE_BROWSER_CLOUD_IMPORT_TITLE);
  SET_STRING("CLOUD_IMPORT_DESTINATION_FOLDER",
             IDS_FILE_BROWSER_CLOUD_DESTINATION_FOLDER);
  SET_STRING("CLOUD_IMPORT_DESCRIPTION",
             IDS_FILE_BROWSER_CLOUD_IMPORT_DESCRIPTION);
  SET_STRING("CLOUD_IMPORT_START", IDS_FILE_BROWSER_CLOUD_IMPORT_START);
  SET_STRING("CLOUD_IMPORT_SHOW_DETAILS",
             IDS_FILE_BROWSER_CLOUD_IMPORT_SHOW_DETAILS);
  SET_STRING("CLOUD_IMPORT_COMMAND", IDS_FILE_BROWSER_CLOUD_IMPORT_COMMAND);
  SET_STRING("CLOUD_IMPORT_CANCEL_COMMAND",
             IDS_FILE_BROWSER_CLOUD_IMPORT_CANCEL_COMMAND);

  SET_STRING("CLOUD_IMPORT_STATUS_READY",
             IDS_FILE_BROWSER_CLOUD_IMPORT_STATUS_DONE);
  SET_STRING("CLOUD_IMPORT_STATUS_IMPORTING",
             IDS_FILE_BROWSER_CLOUD_IMPORT_STATUS_IMPORTING);
  SET_STRING("CLOUD_IMPORT_STATUS_INSUFFICIENT_SPACE",
             IDS_FILE_BROWSER_CLOUD_IMPORT_STATUS_INSUFFICIENT_SPACE);
  SET_STRING("CLOUD_IMPORT_STATUS_NO_MEDIA",
             IDS_FILE_BROWSER_CLOUD_IMPORT_STATUS_NO_MEDIA);
  SET_STRING("CLOUD_IMPORT_STATUS_READY",
             IDS_FILE_BROWSER_CLOUD_IMPORT_STATUS_READY);
  SET_STRING("CLOUD_IMPORT_STATUS_SCANNING",
             IDS_FILE_BROWSER_CLOUD_IMPORT_STATUS_SCANNING);
  SET_STRING("CLOUD_IMPORT_ITEMS_REMAINING",
             IDS_FILE_BROWSER_CLOUD_IMPORT_ITEMS_REMAINING);

  SET_STRING("CLOUD_IMPORT_TOOLTIP_READY",
             IDS_FILE_BROWSER_CLOUD_IMPORT_TOOLTIP_DONE);
  SET_STRING("CLOUD_IMPORT_TOOLTIP_IMPORTING",
             IDS_FILE_BROWSER_CLOUD_IMPORT_TOOLTIP_IMPORTING);
  SET_STRING("CLOUD_IMPORT_TOOLTIP_INSUFFICIENT_SPACE",
             IDS_FILE_BROWSER_CLOUD_IMPORT_TOOLTIP_INSUFFICIENT_SPACE);
  SET_STRING("CLOUD_IMPORT_TOOLTIP_NO_MEDIA",
             IDS_FILE_BROWSER_CLOUD_IMPORT_TOOLTIP_NO_MEDIA);
  SET_STRING("CLOUD_IMPORT_TOOLTIP_READY",
             IDS_FILE_BROWSER_CLOUD_IMPORT_TOOLTIP_READY);
  SET_STRING("CLOUD_IMPORT_TOOLTIP_SCANNING",
             IDS_FILE_BROWSER_CLOUD_IMPORT_TOOLTIP_SCANNING);
}

void AddStringsForCrUiMenuItemShortcuts(base::DictionaryValue* dict) {
  // Shortcut key names: used from cr.ui.MenuItem.updateShortcut_.
  SET_STRING("SHORTCUT_ALT", IDS_FILE_BROWSER_SHORTCUT_ALT);
  SET_STRING("SHORTCUT_CTRL", IDS_FILE_BROWSER_SHORTCUT_CTRL);
  SET_STRING("SHORTCUT_ENTER", IDS_FILE_BROWSER_SHORTCUT_ENTER);
  SET_STRING("SHORTCUT_META", IDS_FILE_BROWSER_SHORTCUT_META);
  SET_STRING("SHORTCUT_SHIFT", IDS_FILE_BROWSER_SHORTCUT_SHIFT);
  SET_STRING("SHORTCUT_SPACE", IDS_FILE_BROWSER_SHORTCUT_SPACE);
}

void AddStringsForFileErrors(base::DictionaryValue* dict) {
  SET_STRING("FILE_ERROR_GENERIC", IDS_FILE_BROWSER_FILE_ERROR_GENERIC);
  SET_STRING("FILE_ERROR_INVALID_MODIFICATION",
             IDS_FILE_BROWSER_FILE_ERROR_INVALID_MODIFICATION);
  SET_STRING("FILE_ERROR_INVALID_STATE",
             IDS_FILE_BROWSER_FILE_ERROR_INVALID_STATE);
  SET_STRING("FILE_ERROR_NOT_FOUND", IDS_FILE_BROWSER_FILE_ERROR_NOT_FOUND);
  SET_STRING("FILE_ERROR_NOT_READABLE",
             IDS_FILE_BROWSER_FILE_ERROR_NOT_READABLE);
  SET_STRING("FILE_ERROR_NO_MODIFICATION_ALLOWED",
             IDS_FILE_BROWSER_FILE_ERROR_NO_MODIFICATION_ALLOWED);
  SET_STRING("FILE_ERROR_PATH_EXISTS", IDS_FILE_BROWSER_FILE_ERROR_PATH_EXISTS);
  SET_STRING("FILE_ERROR_QUOTA_EXCEEDED",
             IDS_FILE_BROWSER_FILE_ERROR_QUOTA_EXCEEDED);
  SET_STRING("FILE_ERROR_SECURITY", IDS_FILE_BROWSER_FILE_ERROR_SECURITY);
}

}  // namespace

namespace extensions {

FileManagerPrivateGetStringsFunction::FileManagerPrivateGetStringsFunction() {
}

FileManagerPrivateGetStringsFunction::~FileManagerPrivateGetStringsFunction() {
}

bool FileManagerPrivateGetStringsFunction::RunSync() {
  base::DictionaryValue* dict = new base::DictionaryValue();
  SetResult(dict);

  AddStringsForDrive(dict);
  AddStringsForFileTypes(dict);
  AddStringsForGallery(dict);
  AddStringsForVideoPlayer(dict);
  AddStringsForAudioPlayer(dict);
  AddStringsForCloudImport(dict);
  AddStringsForCrUiMenuItemShortcuts(dict);
  AddStringsForFileErrors(dict);

  SET_STRING("ACTION_LISTEN", IDS_FILE_BROWSER_ACTION_LISTEN);
  SET_STRING("ACTION_OPEN", IDS_FILE_BROWSER_ACTION_OPEN);
  SET_STRING("ACTION_OPEN_GDOC", IDS_FILE_BROWSER_ACTION_OPEN_GDOC);
  SET_STRING("ACTION_OPEN_GSHEET", IDS_FILE_BROWSER_ACTION_OPEN_GSHEET);
  SET_STRING("ACTION_OPEN_GSLIDES", IDS_FILE_BROWSER_ACTION_OPEN_GSLIDES);
  SET_STRING("ACTION_VIEW", IDS_FILE_BROWSER_ACTION_VIEW);
  SET_STRING("ADD_NEW_SERVICES_BUTTON_LABEL",
             IDS_FILE_BROWSER_ADD_NEW_SERVICES_BUTTON_LABEL);
  SET_STRING("ALL_FILES_FILTER", IDS_FILE_BROWSER_ALL_FILES_FILTER);
  SET_STRING("ARCHIVE_MOUNT_FAILED", IDS_FILE_BROWSER_ARCHIVE_MOUNT_FAILED);
  SET_STRING("CALCULATING_SIZE", IDS_FILE_BROWSER_CALCULATING_SIZE);
  SET_STRING("CANCEL_LABEL", IDS_FILE_BROWSER_CANCEL_LABEL);
  SET_STRING("CHANGE_DEFAULT_CAPTION", IDS_FILE_BROWSER_CHANGE_DEFAULT_CAPTION);
  SET_STRING("CHANGE_DEFAULT_MENU_ITEM",
             IDS_FILE_BROWSER_CHANGE_DEFAULT_MENU_ITEM);
  SET_STRING("CLOSE_VOLUME_BUTTON_LABEL",
             IDS_FILE_BROWSER_CLOSE_VOLUME_BUTTON_LABEL);
  SET_STRING("CONFIGURE_VOLUME_BUTTON_LABEL",
             IDS_FILE_BROWSER_CONFIGURE_VOLUME_BUTTON_LABEL);
  SET_STRING("CONFIRM_MOBILE_DATA_USE",
             IDS_FILE_BROWSER_CONFIRM_MOBILE_DATA_USE);
  SET_STRING("CONFIRM_MOBILE_DATA_USE_PLURAL",
             IDS_FILE_BROWSER_CONFIRM_MOBILE_DATA_USE_PLURAL);
  SET_STRING("CONFIRM_OVERWRITE_FILE", IDS_FILE_BROWSER_CONFIRM_OVERWRITE_FILE);
  SET_STRING("CONFLICT_DIALOG_APPLY_TO_ALL",
             IDS_FILE_BROWSER_CONFLICT_DIALOG_APPLY_TO_ALL);
  SET_STRING("CONFLICT_DIALOG_KEEP_BOTH",
             IDS_FILE_BROWSER_CONFLICT_DIALOG_KEEP_BOTH);
  SET_STRING("CONFLICT_DIALOG_MESSAGE",
             IDS_FILE_BROWSER_CONFLICT_DIALOG_MESSAGE);
  SET_STRING("CONFLICT_DIALOG_REPLACE",
             IDS_FILE_BROWSER_CONFLICT_DIALOG_REPLACE);
  SET_STRING("COPY_BUTTON_LABEL", IDS_FILE_BROWSER_COPY_BUTTON_LABEL);
  SET_STRING("COPY_FILESYSTEM_ERROR", IDS_FILE_BROWSER_COPY_FILESYSTEM_ERROR);
  SET_STRING("COPY_FILE_NAME", IDS_FILE_BROWSER_COPY_FILE_NAME);
  SET_STRING("COPY_ITEMS_REMAINING", IDS_FILE_BROWSER_COPY_ITEMS_REMAINING);
  SET_STRING("COPY_PROGRESS_SUMMARY", IDS_FILE_BROWSER_COPY_PROGRESS_SUMMARY);
  SET_STRING("COPY_SOURCE_NOT_FOUND_ERROR",
             IDS_FILE_BROWSER_COPY_SOURCE_NOT_FOUND_ERROR);
  SET_STRING("COPY_TARGET_EXISTS_ERROR",
             IDS_FILE_BROWSER_COPY_TARGET_EXISTS_ERROR);
  SET_STRING("COPY_UNEXPECTED_ERROR", IDS_FILE_BROWSER_COPY_UNEXPECTED_ERROR);
  SET_STRING("CREATE_FOLDER_SHORTCUT_BUTTON_LABEL",
             IDS_FILE_BROWSER_CREATE_FOLDER_SHORTCUT_BUTTON_LABEL);
  SET_STRING("CUT_BUTTON_LABEL", IDS_FILE_BROWSER_CUT_BUTTON_LABEL);
  SET_STRING("DATE_COLUMN_LABEL", IDS_FILE_BROWSER_DATE_COLUMN_LABEL);
  SET_STRING("DEFAULT_ACTION_LABEL", IDS_FILE_BROWSER_DEFAULT_ACTION_LABEL);
  SET_STRING("DEFAULT_NEW_FOLDER_NAME",
             IDS_FILE_BROWSER_DEFAULT_NEW_FOLDER_NAME);
  SET_STRING("DELETE_BUTTON_LABEL", IDS_FILE_BROWSER_DELETE_BUTTON_LABEL);
  SET_STRING("DELETE_ERROR", IDS_FILE_BROWSER_DELETE_ERROR);
  SET_STRING("DELETE_FILE_NAME", IDS_FILE_BROWSER_DELETE_FILE_NAME);
  SET_STRING("DELETE_ITEMS_REMAINING", IDS_FILE_BROWSER_DELETE_ITEMS_REMAINING);
  SET_STRING("DELETE_PROGRESS_SUMMARY",
             IDS_FILE_BROWSER_DELETE_PROGRESS_SUMMARY);
  SET_STRING("DEVICE_HARD_UNPLUGGED_MESSAGE",
             IDS_DEVICE_HARD_UNPLUGGED_MESSAGE);
  SET_STRING("DEVICE_HARD_UNPLUGGED_TITLE", IDS_DEVICE_HARD_UNPLUGGED_TITLE);
  SET_STRING("DEVICE_UNKNOWN_BUTTON_LABEL", IDS_DEVICE_UNKNOWN_BUTTON_LABEL);
  SET_STRING("DEVICE_UNKNOWN_DEFAULT_MESSAGE",
             IDS_DEVICE_UNKNOWN_DEFAULT_MESSAGE);
  SET_STRING("DEVICE_UNKNOWN_MESSAGE", IDS_DEVICE_UNKNOWN_MESSAGE);
  SET_STRING("DEVICE_UNSUPPORTED_DEFAULT_MESSAGE",
             IDS_DEVICE_UNSUPPORTED_DEFAULT_MESSAGE);
  SET_STRING("DEVICE_UNSUPPORTED_MESSAGE", IDS_DEVICE_UNSUPPORTED_MESSAGE);
  SET_STRING("DIRECTORY_ALREADY_EXISTS",
             IDS_FILE_BROWSER_DIRECTORY_ALREADY_EXISTS);
  SET_STRING("DISABLED_MOBILE_SYNC_NOTIFICATION_ENABLE_BUTTON",
             IDS_FILE_BROWSER_DISABLED_MOBILE_SYNC_NOTIFICATION_ENABLE_BUTTON);
  SET_STRING("DISABLED_MOBILE_SYNC_NOTIFICATION_MESSAGE",
             IDS_FILE_BROWSER_DISABLED_MOBILE_SYNC_NOTIFICATION_MESSAGE);
  SET_STRING("DOWNLOADS_DIRECTORY_LABEL",
             IDS_FILE_BROWSER_DOWNLOADS_DIRECTORY_LABEL);
  SET_STRING("DOWNLOADS_DIRECTORY_WARNING",
             IDS_FILE_BROWSER_DOWNLOADS_DIRECTORY_WARNING);
  SET_STRING("DRAGGING_MULTIPLE_ITEMS",
             IDS_FILE_BROWSER_DRAGGING_MULTIPLE_ITEMS);
  SET_STRING("ERROR_CREATING_FOLDER", IDS_FILE_BROWSER_ERROR_CREATING_FOLDER);
  SET_STRING("ERROR_HIDDEN_NAME", IDS_FILE_BROWSER_ERROR_HIDDEN_NAME);
  SET_STRING("ERROR_INVALID_CHARACTER",
             IDS_FILE_BROWSER_ERROR_INVALID_CHARACTER);
  SET_STRING("ERROR_LONG_NAME", IDS_FILE_BROWSER_ERROR_LONG_NAME);
  SET_STRING("ERROR_PROGRESS_SUMMARY", IDS_FILE_BROWSER_ERROR_PROGRESS_SUMMARY);
  SET_STRING("ERROR_PROGRESS_SUMMARY_PLURAL",
             IDS_FILE_BROWSER_ERROR_PROGRESS_SUMMARY_PLURAL);
  SET_STRING("ERROR_RENAMING", IDS_FILE_BROWSER_ERROR_RENAMING);
  SET_STRING("ERROR_RESERVED_NAME", IDS_FILE_BROWSER_ERROR_RESERVED_NAME);
  SET_STRING("ERROR_WHITESPACE_NAME", IDS_FILE_BROWSER_ERROR_WHITESPACE_NAME);
  SET_STRING("EXTERNAL_STORAGE_DISABLED_MESSAGE",
             IDS_EXTERNAL_STORAGE_DISABLED_MESSAGE);
  SET_STRING("FAILED_SPACE_INFO", IDS_FILE_BROWSER_FAILED_SPACE_INFO);
  SET_STRING("FILENAME_LABEL", IDS_FILE_BROWSER_FILENAME_LABEL);
  SET_STRING("FILE_ALREADY_EXISTS", IDS_FILE_BROWSER_FILE_ALREADY_EXISTS);
  SET_STRING("FORMATTING_FINISHED_FAILURE_MESSAGE",
             IDS_FORMATTING_FINISHED_FAILURE_MESSAGE);
  SET_STRING("FORMATTING_FINISHED_SUCCESS_MESSAGE",
             IDS_FORMATTING_FINISHED_SUCCESS_MESSAGE);
  SET_STRING("FORMATTING_OF_DEVICE_FAILED_TITLE",
             IDS_FORMATTING_OF_DEVICE_FAILED_TITLE);
  SET_STRING("FORMATTING_OF_DEVICE_FINISHED_TITLE",
             IDS_FORMATTING_OF_DEVICE_FINISHED_TITLE);
  SET_STRING("FORMATTING_OF_DEVICE_PENDING_MESSAGE",
             IDS_FORMATTING_OF_DEVICE_PENDING_MESSAGE);
  SET_STRING("FORMATTING_OF_DEVICE_PENDING_TITLE",
             IDS_FORMATTING_OF_DEVICE_PENDING_TITLE);
  SET_STRING("FORMATTING_WARNING", IDS_FILE_BROWSER_FORMATTING_WARNING);
  SET_STRING("FORMAT_DEVICE_BUTTON_LABEL",
             IDS_FILE_BROWSER_FORMAT_DEVICE_BUTTON_LABEL);
  SET_STRING("SORT_BUTTON_TOOLTIP", IDS_FILE_BROWSER_SORT_BUTTON_TOOLTIP);
  SET_STRING("GEAR_BUTTON_TOOLTIP", IDS_FILE_BROWSER_GEAR_BUTTON_TOOLTIP);
  SET_STRING("HOSTED_OFFLINE_MESSAGE", IDS_FILE_BROWSER_HOSTED_OFFLINE_MESSAGE);
  SET_STRING("HOSTED_OFFLINE_MESSAGE_PLURAL",
             IDS_FILE_BROWSER_HOSTED_OFFLINE_MESSAGE_PLURAL);
  SET_STRING("INSTALL_NEW_EXTENSION_LABEL",
             IDS_FILE_BROWSER_INSTALL_NEW_EXTENSION_LABEL);
  SET_STRING("MANY_DIRECTORIES_SELECTED",
             IDS_FILE_BROWSER_MANY_DIRECTORIES_SELECTED);
  SET_STRING("MANY_ENTRIES_SELECTED", IDS_FILE_BROWSER_MANY_ENTRIES_SELECTED);
  SET_STRING("MANY_FILES_SELECTED", IDS_FILE_BROWSER_MANY_FILES_SELECTED);
  SET_STRING("MORE_ACTIONS", IDS_FILE_BROWSER_MORE_ACTIONS);
  SET_STRING("MOUNT_ARCHIVE", IDS_FILE_BROWSER_MOUNT_ARCHIVE);
  SET_STRING("MOVE_FILESYSTEM_ERROR", IDS_FILE_BROWSER_MOVE_FILESYSTEM_ERROR);
  SET_STRING("MOVE_FILE_NAME", IDS_FILE_BROWSER_MOVE_FILE_NAME);
  SET_STRING("MOVE_ITEMS_REMAINING", IDS_FILE_BROWSER_MOVE_ITEMS_REMAINING);
  SET_STRING("MOVE_PROGRESS_SUMMARY", IDS_FILE_BROWSER_MOVE_PROGRESS_SUMMARY);
  SET_STRING("MOVE_SOURCE_NOT_FOUND_ERROR",
             IDS_FILE_BROWSER_MOVE_SOURCE_NOT_FOUND_ERROR);
  SET_STRING("MOVE_TARGET_EXISTS_ERROR",
             IDS_FILE_BROWSER_MOVE_TARGET_EXISTS_ERROR);
  SET_STRING("MOVE_UNEXPECTED_ERROR", IDS_FILE_BROWSER_MOVE_UNEXPECTED_ERROR);
  SET_STRING("MULTIPART_DEVICE_UNSUPPORTED_DEFAULT_MESSAGE",
             IDS_MULTIPART_DEVICE_UNSUPPORTED_DEFAULT_MESSAGE);
  SET_STRING("MULTIPART_DEVICE_UNSUPPORTED_MESSAGE",
             IDS_MULTIPART_DEVICE_UNSUPPORTED_MESSAGE);
  SET_STRING("MULTI_PROFILE_SHARE_DIALOG_MESSAGE",
             IDS_FILE_BROWSER_MULTI_PROFILE_SHARE_DIALOG_MESSAGE);
  SET_STRING("MULTI_PROFILE_SHARE_DIALOG_MESSAGE_PLURAL",
             IDS_FILE_BROWSER_MULTI_PROFILE_SHARE_DIALOG_MESSAGE_PLURAL);
  SET_STRING("MULTI_PROFILE_SHARE_DIALOG_TITLE",
             IDS_FILE_BROWSER_MULTI_PROFILE_SHARE_DIALOG_TITLE);
  SET_STRING("MULTI_PROFILE_SHARE_DIALOG_TITLE_PLURAL",
             IDS_FILE_BROWSER_MULTI_PROFILE_SHARE_DIALOG_TITLE_PLURAL);
  SET_STRING("NAME_COLUMN_LABEL", IDS_FILE_BROWSER_NAME_COLUMN_LABEL);
  SET_STRING("EMPTY_FOLDER", IDS_FILE_BROWSER_EMPTY_FOLDER);
  SET_STRING("NEW_FOLDER_BUTTON_LABEL",
             IDS_FILE_BROWSER_NEW_FOLDER_BUTTON_LABEL);
  SET_STRING("NEW_WINDOW_BUTTON_LABEL",
             IDS_FILE_BROWSER_NEW_WINDOW_BUTTON_LABEL);
  SET_STRING("NO_ACTION_FOR_CRX", IDS_FILE_BROWSER_NO_ACTION_FOR_CRX);
  SET_STRING("NO_ACTION_FOR_CRX_TITLE",
             IDS_FILE_BROWSER_NO_ACTION_FOR_CRX_TITLE);
  SET_STRING("NO_ACTION_FOR_DMG", IDS_FILE_BROWSER_NO_ACTION_FOR_DMG);
  SET_STRING("NO_ACTION_FOR_EXECUTABLE",
             IDS_FILE_BROWSER_NO_ACTION_FOR_EXECUTABLE);
  SET_STRING("NO_ACTION_FOR_FILE", IDS_FILE_BROWSER_NO_ACTION_FOR_FILE);
  SET_STRING("OFFLINE_COLUMN_LABEL", IDS_FILE_BROWSER_OFFLINE_COLUMN_LABEL);
  SET_STRING("OFFLINE_HEADER", IDS_FILE_BROWSER_OFFLINE_HEADER);
  SET_STRING("OFFLINE_MESSAGE", IDS_FILE_BROWSER_OFFLINE_MESSAGE);
  SET_STRING("OFFLINE_MESSAGE_PLURAL", IDS_FILE_BROWSER_OFFLINE_MESSAGE_PLURAL);
  SET_STRING("OK_LABEL", IDS_FILE_BROWSER_OK_LABEL);
  SET_STRING("OPEN_IN_OTHER_DESKTOP_MESSAGE",
             IDS_FILE_BROWSER_OPEN_IN_OTHER_DESKTOP_MESSAGE);
  SET_STRING("OPEN_IN_OTHER_DESKTOP_MESSAGE_PLURAL",
             IDS_FILE_BROWSER_OPEN_IN_OTHER_DESKTOP_MESSAGE_PLURAL);
  SET_STRING("OPEN_LABEL", IDS_FILE_BROWSER_OPEN_LABEL);
  SET_STRING("OPEN_WITH_BUTTON_LABEL", IDS_FILE_BROWSER_OPEN_WITH_BUTTON_LABEL);
  SET_STRING("PASTE_BUTTON_LABEL", IDS_FILE_BROWSER_PASTE_BUTTON_LABEL);
  SET_STRING("PASTE_INTO_FOLDER_BUTTON_LABEL",
             IDS_FILE_BROWSER_PASTE_INTO_FOLDER_BUTTON_LABEL);
  SET_STRING("PREPARING_LABEL", IDS_FILE_BROWSER_PREPARING_LABEL);
  SET_STRING("REFRESH_BUTTON_LABEL", IDS_FILE_BROWSER_REFRESH_BUTTON_LABEL);
  SET_STRING("REMOVABLE_DEVICE_DETECTION_TITLE",
             IDS_REMOVABLE_DEVICE_DETECTION_TITLE);
  SET_STRING("REMOVABLE_DEVICE_IMPORT_BUTTON_LABEL",
             IDS_REMOVABLE_DEVICE_IMPORT_BUTTON_LABEL);
  SET_STRING("REMOVABLE_DEVICE_IMPORT_MESSAGE",
             IDS_REMOVABLE_DEVICE_IMPORT_MESSAGE);
  SET_STRING("REMOVABLE_DEVICE_NAVIGATION_BUTTON_LABEL",
             IDS_REMOVABLE_DEVICE_NAVIGATION_BUTTON_LABEL);
  SET_STRING("REMOVABLE_DEVICE_NAVIGATION_MESSAGE",
             IDS_REMOVABLE_DEVICE_NAVIGATION_MESSAGE);
  SET_STRING("REMOVE_FOLDER_SHORTCUT_BUTTON_LABEL",
             IDS_FILE_BROWSER_REMOVE_FOLDER_SHORTCUT_BUTTON_LABEL);
  SET_STRING("RENAME_BUTTON_LABEL", IDS_FILE_BROWSER_RENAME_BUTTON_LABEL);
  SET_STRING("SAVE_LABEL", IDS_FILE_BROWSER_SAVE_LABEL);
  SET_STRING("SEARCH_DRIVE_HTML", IDS_FILE_BROWSER_SEARCH_DRIVE_HTML);
  SET_STRING("SEARCH_NO_MATCHING_FILES_HTML",
             IDS_FILE_BROWSER_SEARCH_NO_MATCHING_FILES_HTML);
  SET_STRING("SEARCH_TEXT_LABEL", IDS_FILE_BROWSER_SEARCH_TEXT_LABEL);
  SET_STRING("TASKS_BUTTON_LABEL", IDS_FILE_BROWSER_TASKS_BUTTON_LABEL);
  SET_STRING("TOGGLE_HIDDEN_FILES_COMMAND_LABEL",
             IDS_FILE_BROWSER_TOGGLE_HIDDEN_FILES_COMMAND_LABEL);
  SET_STRING("SHARE_BUTTON_LABEL", IDS_FILE_BROWSER_SHARE_BUTTON_LABEL);
  SET_STRING("CHANGE_TO_LISTVIEW_BUTTON_LABEL",
             IDS_FILE_BROWSER_CHANGE_TO_LISTVIEW_BUTTON_LABEL);
  SET_STRING("CHANGE_TO_THUMBNAILVIEW_BUTTON_LABEL",
             IDS_FILE_BROWSER_CHANGE_TO_THUMBNAILVIEW_BUTTON_LABEL);
  SET_STRING("CANCEL_SELECTION_BUTTON_LABEL",
             IDS_FILE_BROWSER_CANCEL_SELECTION_BUTTON_LABEL);
  SET_STRING("SHARE_ERROR", IDS_FILE_BROWSER_SHARE_ERROR);

  SET_STRING("SIZE_BYTES", IDS_FILE_BROWSER_SIZE_BYTES);
  SET_STRING("SIZE_COLUMN_LABEL", IDS_FILE_BROWSER_SIZE_COLUMN_LABEL);
  SET_STRING("SIZE_GB", IDS_FILE_BROWSER_SIZE_GB);
  SET_STRING("SIZE_KB", IDS_FILE_BROWSER_SIZE_KB);
  SET_STRING("SIZE_MB", IDS_FILE_BROWSER_SIZE_MB);
  SET_STRING("SIZE_PB", IDS_FILE_BROWSER_SIZE_PB);
  SET_STRING("SIZE_TB", IDS_FILE_BROWSER_SIZE_TB);
  SET_STRING("SPACE_AVAILABLE", IDS_FILE_BROWSER_SPACE_AVAILABLE);
  SET_STRING("STATUS_COLUMN_LABEL", IDS_FILE_BROWSER_STATUS_COLUMN_LABEL);
  SET_STRING("SUGGEST_DIALOG_INSTALLATION_FAILED",
             IDS_FILE_BROWSER_SUGGEST_DIALOG_INSTALLATION_FAILED);
  SET_STRING("SUGGEST_DIALOG_LINK_TO_WEBSTORE",
             IDS_FILE_BROWSER_SUGGEST_DIALOG_LINK_TO_WEBSTORE);
  SET_STRING("SUGGEST_DIALOG_TITLE", IDS_FILE_BROWSER_SUGGEST_DIALOG_TITLE);
  SET_STRING("SUGGEST_DIALOG_FOR_PROVIDERS_TITLE",
             IDS_FILE_BROWSER_SUGGEST_DIALOG_FOR_PROVIDERS_TITLE);
  SET_STRING("SUGGEST_DIALOG_LOADING_SPINNER_ALT",
             IDS_WEBSTORE_WIDGET_LOADING_SPINNER_ALT);
  SET_STRING("SUGGEST_DIALOG_INSTALLING_SPINNER_ALT",
             IDS_WEBSTORE_WIDGET_INSTALLING_SPINNER_ALT);
  SET_STRING("THUMBNAIL_VIEW_TOOLTIP", IDS_FILE_BROWSER_THUMBNAIL_VIEW_TOOLTIP);
  SET_STRING("TIME_TODAY", IDS_FILE_BROWSER_TIME_TODAY);
  SET_STRING("TIME_YESTERDAY", IDS_FILE_BROWSER_TIME_YESTERDAY);
  SET_STRING("TRANSFER_PROGRESS_SUMMARY",
             IDS_FILE_BROWSER_TRANSFER_PROGRESS_SUMMARY);
  SET_STRING("TYPE_COLUMN_LABEL", IDS_FILE_BROWSER_TYPE_COLUMN_LABEL);
  SET_STRING("UNKNOWN_FILESYSTEM_WARNING",
             IDS_FILE_BROWSER_UNKNOWN_FILESYSTEM_WARNING);
  SET_STRING("UNMOUNT_DEVICE_BUTTON_LABEL",
             IDS_FILE_BROWSER_UNMOUNT_DEVICE_BUTTON_LABEL);
  SET_STRING("UNMOUNT_FAILED", IDS_FILE_BROWSER_UNMOUNT_FAILED);
  SET_STRING("UNSUPPORTED_FILESYSTEM_WARNING",
             IDS_FILE_BROWSER_UNSUPPORTED_FILESYSTEM_WARNING);
  SET_STRING("UPLOAD_LABEL", IDS_FILE_BROWSER_UPLOAD_LABEL);
  SET_STRING("WAITING_FOR_SPACE_INFO", IDS_FILE_BROWSER_WAITING_FOR_SPACE_INFO);
  SET_STRING("ZIP_FILESYSTEM_ERROR", IDS_FILE_BROWSER_ZIP_FILESYSTEM_ERROR);
  SET_STRING("ZIP_FILE_NAME", IDS_FILE_BROWSER_ZIP_FILE_NAME);
  SET_STRING("ZIP_ITEMS_REMAINING", IDS_FILE_BROWSER_ZIP_ITEMS_REMAINING);
  SET_STRING("ZIP_PROGRESS_SUMMARY", IDS_FILE_BROWSER_ZIP_PROGRESS_SUMMARY);
  SET_STRING("ZIP_SELECTION_BUTTON_LABEL",
             IDS_FILE_BROWSER_ZIP_SELECTION_BUTTON_LABEL);
  SET_STRING("ZIP_TARGET_EXISTS_ERROR",
             IDS_FILE_BROWSER_ZIP_TARGET_EXISTS_ERROR);
  SET_STRING("ZIP_UNEXPECTED_ERROR", IDS_FILE_BROWSER_ZIP_UNEXPECTED_ERROR);
#undef SET_STRING

  dict->SetBoolean("PDF_VIEW_ENABLED",
                   file_manager::util::ShouldBeOpenedWithPlugin(
                       GetProfile(), FILE_PATH_LITERAL(".pdf")));
  dict->SetBoolean("SWF_VIEW_ENABLED",
                   file_manager::util::ShouldBeOpenedWithPlugin(
                       GetProfile(), FILE_PATH_LITERAL(".swf")));
  dict->SetString("CHROMEOS_RELEASE_BOARD",
                  base::SysInfo::GetLsbReleaseBoard());
  dict->SetString(
      "DOWNLOADS_LOW_SPACE_WARNING_HELP_URL",
      base::StringPrintf(kHelpURLFormat, kDownloadsLowSpaceWarningHelpNumber));
  dict->SetString("FILES_APP_HELP_URL",
                  base::StringPrintf(kHelpURLFormat, kFilesAppHelpNumber));

  dict->SetString("GOOGLE_DRIVE_BUY_STORAGE_URL", kGoogleDriveBuyStorageUrl);
  dict->SetString(
      "GOOGLE_DRIVE_ERROR_HELP_URL",
      base::StringPrintf(kHelpURLFormat, kGoogleDriveErrorHelpNumber));
  dict->SetString("GOOGLE_DRIVE_HELP_URL", kGoogleDriveHelpUrl);
  dict->SetString("GOOGLE_DRIVE_OVERVIEW_URL", kGoogleDriveOverviewUrl);
  dict->SetString("GOOGLE_DRIVE_REDEEM_URL", kGoogleDriveRedeemUrl);
  dict->SetString("GOOGLE_DRIVE_ROOT_URL", kGoogleDriveRootUrl);
  dict->SetString(
      "NO_ACTION_FOR_FILE_URL",
      base::StringPrintf(kHelpURLFormat, kNoActionForFileHelpNumber));
  dict->SetString("UI_LOCALE", extension_l10n_util::CurrentLocaleOrDefault());

  const std::string& app_locale = g_browser_process->GetApplicationLocale();
  webui::SetLoadTimeDataDefaults(app_locale, dict);

  return true;
}

}  // namespace extensions
