// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/developer_private/extension_info_generator.h"

#include "base/base64.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/thread_task_runner_handle.h"
#include "chrome/browser/extensions/api/commands/command_service.h"
#include "chrome/browser/extensions/api/developer_private/inspectable_views_finder.h"
#include "chrome/browser/extensions/api/extension_action/extension_action_api.h"
#include "chrome/browser/extensions/error_console/error_console.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_ui_util.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/path_util.h"
#include "chrome/browser/extensions/shared_module_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/extensions/extension_icon_source.h"
#include "chrome/common/extensions/command.h"
#include "chrome/common/extensions/manifest_handlers/app_launch_info.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/render_frame_host.h"
#include "extensions/browser/extension_error.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/image_loader.h"
#include "extensions/browser/warning_service.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/feature_switch.h"
#include "extensions/common/install_warning.h"
#include "extensions/common/manifest.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "extensions/common/manifest_handlers/icons_handler.h"
#include "extensions/common/manifest_handlers/offline_enabled_info.h"
#include "extensions/common/manifest_handlers/options_page_info.h"
#include "extensions/common/manifest_url_handlers.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/grit/extensions_browser_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/skbitmap_operations.h"

namespace extensions {

namespace developer = api::developer_private;

namespace {

// Given a Manifest::Type, converts it into its developer_private
// counterpart.
developer::ExtensionType GetExtensionType(Manifest::Type manifest_type) {
  developer::ExtensionType type = developer::EXTENSION_TYPE_EXTENSION;
  switch (manifest_type) {
    case Manifest::TYPE_EXTENSION:
      type = developer::EXTENSION_TYPE_EXTENSION;
      break;
    case Manifest::TYPE_THEME:
      type = developer::EXTENSION_TYPE_THEME;
      break;
    case Manifest::TYPE_HOSTED_APP:
      type = developer::EXTENSION_TYPE_HOSTED_APP;
      break;
    case Manifest::TYPE_LEGACY_PACKAGED_APP:
      type = developer::EXTENSION_TYPE_LEGACY_PACKAGED_APP;
      break;
    case Manifest::TYPE_PLATFORM_APP:
      type = developer::EXTENSION_TYPE_PLATFORM_APP;
      break;
    case Manifest::TYPE_SHARED_MODULE:
      type = developer::EXTENSION_TYPE_SHARED_MODULE;
      break;
    default:
      NOTREACHED();
  }
  return type;
}

// Populates the common fields of an extension error.
template <typename ErrorType>
void PopulateErrorBase(const ExtensionError& error, ErrorType* out) {
  CHECK(out);
  out->type = error.type() == ExtensionError::MANIFEST_ERROR ?
      developer::ERROR_TYPE_MANIFEST : developer::ERROR_TYPE_RUNTIME;
  out->extension_id = error.extension_id();
  out->from_incognito = error.from_incognito();
  out->source = base::UTF16ToUTF8(error.source());
  out->message = base::UTF16ToUTF8(error.message());
  out->id = error.id();
}

// Given a ManifestError object, converts it into its developer_private
// counterpart.
linked_ptr<developer::ManifestError> ConstructManifestError(
    const ManifestError& error) {
  linked_ptr<developer::ManifestError> result(new developer::ManifestError());
  PopulateErrorBase(error, result.get());
  result->manifest_key = base::UTF16ToUTF8(error.manifest_key());
  if (!error.manifest_specific().empty()) {
    result->manifest_specific.reset(
        new std::string(base::UTF16ToUTF8(error.manifest_specific())));
  }
  return result;
}

// Given a RuntimeError object, converts it into its developer_private
// counterpart.
linked_ptr<developer::RuntimeError> ConstructRuntimeError(
    const RuntimeError& error) {
  linked_ptr<developer::RuntimeError> result(new developer::RuntimeError());
  PopulateErrorBase(error, result.get());
  switch (error.level()) {
    case logging::LOG_VERBOSE:
    case logging::LOG_INFO:
      result->severity = developer::ERROR_LEVEL_LOG;
      break;
    case logging::LOG_WARNING:
      result->severity = developer::ERROR_LEVEL_WARN;
      break;
    case logging::LOG_FATAL:
    case logging::LOG_ERROR:
      result->severity = developer::ERROR_LEVEL_ERROR;
      break;
    default:
      NOTREACHED();
  }
  result->occurrences = error.occurrences();
  // NOTE(devlin): This is called "render_view_id" in the api for legacy
  // reasons, but it's not a high priority to change.
  result->render_view_id = error.render_frame_id();
  result->render_process_id = error.render_process_id();
  result->can_inspect =
      content::RenderFrameHost::FromID(error.render_process_id(),
                                       error.render_frame_id()) != nullptr;
  for (const StackFrame& f : error.stack_trace()) {
    linked_ptr<developer::StackFrame> frame(new developer::StackFrame());
    frame->line_number = f.line_number;
    frame->column_number = f.column_number;
    frame->url = base::UTF16ToUTF8(f.source);
    frame->function_name = base::UTF16ToUTF8(f.function);
    result->stack_trace.push_back(frame);
  }
  return result;
}

// Constructs any commands for the extension with the given |id|, and adds them
// to the list of |commands|.
void ConstructCommands(CommandService* command_service,
                       const std::string& extension_id,
                       std::vector<linked_ptr<developer::Command>>* commands) {
  auto construct_command = [](const Command& command,
                              bool active,
                              bool is_extension_action) {
    developer::Command* command_value = new developer::Command();
    command_value->description = is_extension_action ?
        l10n_util::GetStringUTF8(IDS_EXTENSION_COMMANDS_GENERIC_ACTIVATE) :
        base::UTF16ToUTF8(command.description());
    command_value->keybinding =
        base::UTF16ToUTF8(command.accelerator().GetShortcutText());
    command_value->name = command.command_name();
    command_value->is_active = active;
    command_value->scope = command.global() ? developer::COMMAND_SCOPE_GLOBAL :
        developer::COMMAND_SCOPE_CHROME;
    command_value->is_extension_action = is_extension_action;
    return command_value;
  };
  bool active = false;
  Command browser_action;
  if (command_service->GetBrowserActionCommand(extension_id,
                                               CommandService::ALL,
                                               &browser_action,
                                               &active)) {
    commands->push_back(
        make_linked_ptr(construct_command(browser_action, active, true)));
  }

  Command page_action;
  if (command_service->GetPageActionCommand(extension_id,
                                            CommandService::ALL,
                                            &page_action,
                                            &active)) {
    commands->push_back(
        make_linked_ptr(construct_command(page_action, active, true)));
  }

  CommandMap named_commands;
  if (command_service->GetNamedCommands(extension_id,
                                        CommandService::ALL,
                                        CommandService::ANY_SCOPE,
                                        &named_commands)) {
    for (auto& pair : named_commands) {
      Command& command_to_use = pair.second;
      // TODO(devlin): For some reason beyond my knowledge, FindCommandByName
      // returns different data than GetNamedCommands, including the
      // accelerators, but not the descriptions - and even then, only if the
      // command is active.
      // Unfortunately, some systems may be relying on the other data (which
      // more closely matches manifest data).
      // Until we can sort all this out, we merge the two command structures.
      Command active_command = command_service->FindCommandByName(
          extension_id, command_to_use.command_name());
      command_to_use.set_accelerator(active_command.accelerator());
      command_to_use.set_global(active_command.global());
      bool active = command_to_use.accelerator().key_code() != ui::VKEY_UNKNOWN;
      commands->push_back(
          make_linked_ptr(construct_command(command_to_use, active, false)));
    }
  }
}

}  // namespace

ExtensionInfoGenerator::ExtensionInfoGenerator(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context),
      command_service_(CommandService::Get(browser_context)),
      extension_system_(ExtensionSystem::Get(browser_context)),
      extension_prefs_(ExtensionPrefs::Get(browser_context)),
      extension_action_api_(ExtensionActionAPI::Get(browser_context)),
      warning_service_(WarningService::Get(browser_context)),
      error_console_(ErrorConsole::Get(browser_context)),
      image_loader_(ImageLoader::Get(browser_context)),
      pending_image_loads_(0u),
      weak_factory_(this) {
}

ExtensionInfoGenerator::~ExtensionInfoGenerator() {
}

void ExtensionInfoGenerator::CreateExtensionInfo(
    const std::string& id,
    const ExtensionInfosCallback& callback) {
  DCHECK(callback_.is_null() && list_.empty()) <<
      "Only a single generation can be running at a time!";
  ExtensionRegistry* registry = ExtensionRegistry::Get(browser_context_);

  developer::ExtensionState state = developer::EXTENSION_STATE_NONE;
  const Extension* ext = nullptr;
  if ((ext = registry->enabled_extensions().GetByID(id)) != nullptr)
    state = developer::EXTENSION_STATE_ENABLED;
  else if ((ext = registry->disabled_extensions().GetByID(id)) != nullptr)
    state = developer::EXTENSION_STATE_DISABLED;
  else if ((ext = registry->terminated_extensions().GetByID(id)) != nullptr)
    state = developer::EXTENSION_STATE_TERMINATED;

  if (ext && ui_util::ShouldDisplayInExtensionSettings(ext, browser_context_))
    CreateExtensionInfoHelper(*ext, state);

  if (pending_image_loads_ == 0) {
    // Don't call the callback re-entrantly.
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                  base::Bind(callback, list_));
    list_.clear();
  } else {
    callback_ = callback;
  }
}

void ExtensionInfoGenerator::CreateExtensionsInfo(
    bool include_disabled,
    bool include_terminated,
    const ExtensionInfosCallback& callback) {
  auto add_to_list = [this](const ExtensionSet& extensions,
                            developer::ExtensionState state) {
    for (const scoped_refptr<const Extension>& extension : extensions) {
      if (ui_util::ShouldDisplayInExtensionSettings(extension.get(),
                                                    browser_context_)) {
        CreateExtensionInfoHelper(*extension, state);
      }
    }
  };

  ExtensionRegistry* registry = ExtensionRegistry::Get(browser_context_);
  add_to_list(registry->enabled_extensions(),
              developer::EXTENSION_STATE_ENABLED);
  if (include_disabled) {
    add_to_list(registry->disabled_extensions(),
                developer::EXTENSION_STATE_DISABLED);
  }
  if (include_terminated) {
    add_to_list(registry->terminated_extensions(),
                developer::EXTENSION_STATE_TERMINATED);
  }

  if (pending_image_loads_ == 0) {
    // Don't call the callback re-entrantly.
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                  base::Bind(callback, list_));
    list_.clear();
  } else {
    callback_ = callback;
  }
}

void ExtensionInfoGenerator::CreateExtensionInfoHelper(
    const Extension& extension,
    developer::ExtensionState state) {
  scoped_ptr<developer::ExtensionInfo> info(new developer::ExtensionInfo());

  // Don't consider the button hidden with the redesign, because "hidden"
  // buttons are now just hidden in the wrench menu.
  info->action_button_hidden =
      !extension_action_api_->GetBrowserActionVisibility(extension.id()) &&
      !FeatureSwitch::extension_action_redesign()->IsEnabled();

  // Blacklist text.
  int blacklist_text = -1;
  switch (extension_prefs_->GetExtensionBlacklistState(extension.id())) {
    case BLACKLISTED_SECURITY_VULNERABILITY:
      blacklist_text = IDS_OPTIONS_BLACKLISTED_SECURITY_VULNERABILITY;
      break;
    case BLACKLISTED_CWS_POLICY_VIOLATION:
      blacklist_text = IDS_OPTIONS_BLACKLISTED_CWS_POLICY_VIOLATION;
      break;
    case BLACKLISTED_POTENTIALLY_UNWANTED:
      blacklist_text = IDS_OPTIONS_BLACKLISTED_POTENTIALLY_UNWANTED;
      break;
    default:
      break;
  }
  if (blacklist_text != -1) {
    info->blacklist_text.reset(
        new std::string(l10n_util::GetStringUTF8(blacklist_text)));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context_);

  // ControlledInfo.
  bool is_policy_location = Manifest::IsPolicyLocation(extension.location());
  if (is_policy_location || util::IsExtensionSupervised(&extension, profile)) {
    info->controlled_info.reset(new developer::ControlledInfo());
    if (is_policy_location) {
      info->controlled_info->type = developer::CONTROLLER_TYPE_POLICY;
      info->controlled_info->text =
          l10n_util::GetStringUTF8(IDS_OPTIONS_INSTALL_LOCATION_ENTERPRISE);
    } else if (profile->IsChild()) {
      info->controlled_info->type = developer::CONTROLLER_TYPE_CHILD_CUSTODIAN;
      info->controlled_info->text = l10n_util::GetStringUTF8(
          IDS_EXTENSIONS_INSTALLED_BY_CHILD_CUSTODIAN);
    } else {
      info->controlled_info->type =
          developer::CONTROLLER_TYPE_SUPERVISED_USER_CUSTODIAN;
      info->controlled_info->text = l10n_util::GetStringUTF8(
          IDS_EXTENSIONS_INSTALLED_BY_SUPERVISED_USER_CUSTODIAN);
    }
  }

  bool is_enabled = state == developer::EXTENSION_STATE_ENABLED;

  // Commands.
  if (is_enabled)
    ConstructCommands(command_service_, extension.id(), &info->commands);

  // Dependent extensions.
  if (extension.is_shared_module()) {
    scoped_ptr<ExtensionSet> dependent_extensions =
        extension_system_->extension_service()->
            shared_module_service()->GetDependentExtensions(&extension);
    for (const scoped_refptr<const Extension>& dependent :
             *dependent_extensions)
      info->dependent_extensions.push_back(dependent->id());
  }

  info->description = extension.description();

  // Disable reasons.
  int disable_reasons = extension_prefs_->GetDisableReasons(extension.id());
  info->disable_reasons.suspicious_install =
      (disable_reasons & Extension::DISABLE_NOT_VERIFIED) != 0;
  info->disable_reasons.corrupt_install =
      (disable_reasons & Extension::DISABLE_CORRUPTED) != 0;
  info->disable_reasons.update_required =
      (disable_reasons & Extension::DISABLE_UPDATE_REQUIRED_BY_POLICY) != 0;

  // Error collection.
  bool error_console_enabled =
      error_console_->IsEnabledForChromeExtensionsPage();
  info->error_collection.is_enabled = error_console_enabled;
  info->error_collection.is_active =
      error_console_enabled &&
      error_console_->IsReportingEnabledForExtension(extension.id());

  // File access.
  info->file_access.is_enabled = extension.wants_file_access();
  info->file_access.is_active =
      util::AllowFileAccess(extension.id(), browser_context_);

  // Home page.
  info->home_page.url = ManifestURL::GetHomepageURL(&extension).spec();
  info->home_page.specified = ManifestURL::SpecifiedHomepageURL(&extension);

  info->id = extension.id();

  // Incognito access.
  info->incognito_access.is_enabled = extension.can_be_incognito_enabled();
  info->incognito_access.is_active =
      util::IsIncognitoEnabled(extension.id(), browser_context_);

  // Install warnings, but only if unpacked, the error console isn't enabled
  // (otherwise it shows these), and we're in developer mode (normal users don't
  // need to see these).
  if (!error_console_enabled &&
      Manifest::IsUnpackedLocation(extension.location()) &&
      profile->GetPrefs()->GetBoolean(prefs::kExtensionsUIDeveloperMode)) {
    const std::vector<InstallWarning>& install_warnings =
        extension.install_warnings();
    for (const InstallWarning& warning : install_warnings)
      info->install_warnings.push_back(warning.message);
  }

  // Launch url.
  if (extension.is_app()) {
    info->launch_url.reset(
        new std::string(AppLaunchInfo::GetFullLaunchURL(&extension).spec()));
  }

  // Location.
  if (extension.location() == Manifest::INTERNAL &&
      ManifestURL::UpdatesFromGallery(&extension)) {
    info->location = developer::LOCATION_FROM_STORE;
  } else if (Manifest::IsUnpackedLocation(extension.location())) {
    info->location = developer::LOCATION_UNPACKED;
  } else if (Manifest::IsExternalLocation(extension.location()) &&
             ManifestURL::UpdatesFromGallery(&extension)) {
    info->location = developer::LOCATION_THIRD_PARTY;
  } else {
    info->location = developer::LOCATION_UNKNOWN;
  }

  // Location text.
  int location_text = -1;
  if (info->location == developer::LOCATION_UNKNOWN)
    location_text = IDS_OPTIONS_INSTALL_LOCATION_UNKNOWN;
  else if (extension.location() == Manifest::EXTERNAL_REGISTRY)
    location_text = IDS_OPTIONS_INSTALL_LOCATION_3RD_PARTY;
  else if (extension.is_shared_module())
    location_text = IDS_OPTIONS_INSTALL_LOCATION_SHARED_MODULE;
  if (location_text != -1) {
    info->location_text.reset(
        new std::string(l10n_util::GetStringUTF8(location_text)));
  }

  // Runtime/Manifest errors.
  if (error_console_enabled) {
    const ErrorList& errors =
        error_console_->GetErrorsForExtension(extension.id());
    for (const ExtensionError* error : errors) {
      switch (error->type()) {
        case ExtensionError::MANIFEST_ERROR:
          info->manifest_errors.push_back(ConstructManifestError(
              static_cast<const ManifestError&>(*error)));
          break;
        case ExtensionError::RUNTIME_ERROR:
          info->runtime_errors.push_back(ConstructRuntimeError(
              static_cast<const RuntimeError&>(*error)));
          break;
        case ExtensionError::INTERNAL_ERROR:
          // TODO(wittman): Support InternalError in developer tools:
          // https://crbug.com/503427.
          break;
        case ExtensionError::NUM_ERROR_TYPES:
          NOTREACHED();
          break;
      }
    }
  }

  ManagementPolicy* management_policy = extension_system_->management_policy();
  info->must_remain_installed =
      management_policy->MustRemainInstalled(&extension, nullptr);

  info->name = extension.name();
  info->offline_enabled = OfflineEnabledInfo::IsOfflineEnabled(&extension);

  // Options page.
  if (OptionsPageInfo::HasOptionsPage(&extension)) {
    info->options_page.reset(new developer::OptionsPage());
    info->options_page->open_in_tab =
        OptionsPageInfo::ShouldOpenInTab(&extension);
    info->options_page->url =
        OptionsPageInfo::GetOptionsPage(&extension).spec();
  }

  // Path.
  if (Manifest::IsUnpackedLocation(extension.location())) {
    info->path.reset(new std::string(extension.path().AsUTF8Unsafe()));
    info->prettified_path.reset(new std::string(
      extensions::path_util::PrettifyPath(extension.path()).AsUTF8Unsafe()));
  }

  // Runs on all urls.
  info->run_on_all_urls.is_enabled =
      (FeatureSwitch::scripts_require_action()->IsEnabled() &&
       PermissionsData::ScriptsMayRequireActionForExtension(
           &extension,
           extension.permissions_data()->active_permissions().get())) ||
      extension.permissions_data()->HasWithheldImpliedAllHosts() ||
      util::HasSetAllowedScriptingOnAllUrls(extension.id(), browser_context_);
  info->run_on_all_urls.is_active =
      util::AllowedScriptingOnAllUrls(extension.id(), browser_context_);

  // Runtime warnings.
  std::vector<std::string> warnings =
      warning_service_->GetWarningMessagesForExtension(extension.id());
  for (const std::string& warning : warnings)
    info->runtime_warnings.push_back(warning);

  info->state = state;

  info->type = GetExtensionType(extension.manifest()->type());

  info->update_url = ManifestURL::GetUpdateURL(&extension).spec();

  info->user_may_modify =
      management_policy->UserMayModifySettings(&extension, nullptr);

  info->version = extension.GetVersionForDisplay();

  if (state != developer::EXTENSION_STATE_TERMINATED) {
    info->views = InspectableViewsFinder(profile).
                      GetViewsForExtension(extension, is_enabled);
  }

  // The icon.
  ExtensionResource icon =
      IconsInfo::GetIconResource(&extension,
                                 extension_misc::EXTENSION_ICON_MEDIUM,
                                 ExtensionIconSet::MATCH_BIGGER);
  if (icon.empty()) {
    info->icon_url = GetDefaultIconUrl(extension.is_app(), !is_enabled);
    list_.push_back(make_linked_ptr(info.release()));
  } else {
    ++pending_image_loads_;
    // Max size of 128x128 is a random guess at a nice balance between being
    // overly eager to resize and sending across gigantic data urls. (The icon
    // used by the url is 48x48).
    gfx::Size max_size(128, 128);
    image_loader_->LoadImageAsync(
        &extension,
        icon,
        max_size,
        base::Bind(&ExtensionInfoGenerator::OnImageLoaded,
                   weak_factory_.GetWeakPtr(),
                   base::Passed(info.Pass())));
  }
}

const std::string& ExtensionInfoGenerator::GetDefaultIconUrl(
    bool is_app,
    bool is_greyscale) {
  std::string* str;
  if (is_app) {
    str = is_greyscale ? &default_disabled_app_icon_url_ :
        &default_app_icon_url_;
  } else {
    str = is_greyscale ? &default_disabled_extension_icon_url_ :
        &default_extension_icon_url_;
  }

  if (str->empty()) {
    *str = GetIconUrlFromImage(
        ui::ResourceBundle::GetSharedInstance().GetImageNamed(
            is_app ? IDR_APP_DEFAULT_ICON : IDR_EXTENSION_DEFAULT_ICON),
        is_greyscale);
  }

  return *str;
}

std::string ExtensionInfoGenerator::GetIconUrlFromImage(
    const gfx::Image& image,
    bool should_greyscale) {
  scoped_refptr<base::RefCountedMemory> data;
  if (should_greyscale) {
    color_utils::HSL shift = {-1, 0, 0.6};
    const SkBitmap* bitmap = image.ToSkBitmap();
    DCHECK(bitmap);
    SkBitmap grey = SkBitmapOperations::CreateHSLShiftedBitmap(*bitmap, shift);
    scoped_refptr<base::RefCountedBytes> image_bytes(
        new base::RefCountedBytes());
    gfx::PNGCodec::EncodeBGRASkBitmap(grey, false, &image_bytes->data());
    data = image_bytes;
  } else {
    data = image.As1xPNGBytes();
  }

  std::string base_64;
  base::Base64Encode(std::string(data->front_as<char>(), data->size()),
                     &base_64);
  const char kDataUrlPrefix[] = "data:image/png;base64,";
  return GURL(kDataUrlPrefix + base_64).spec();
}

void ExtensionInfoGenerator::OnImageLoaded(
    scoped_ptr<developer::ExtensionInfo> info,
    const gfx::Image& icon) {
  if (!icon.IsEmpty()) {
    info->icon_url = GetIconUrlFromImage(
        icon, info->state != developer::EXTENSION_STATE_ENABLED);
  } else {
    bool is_app =
        info->type == developer::EXTENSION_TYPE_HOSTED_APP ||
        info->type == developer::EXTENSION_TYPE_LEGACY_PACKAGED_APP ||
        info->type == developer::EXTENSION_TYPE_PLATFORM_APP;
    info->icon_url = GetDefaultIconUrl(
        is_app, info->state != developer::EXTENSION_STATE_ENABLED);
  }

  list_.push_back(make_linked_ptr(info.release()));

  --pending_image_loads_;

  if (pending_image_loads_ == 0) {  // All done!
    // We assign to a temporary callback and list and reset the stored values so
    // that at the end of the method, any stored refs are destroyed.
    ExtensionInfoList list;
    list.swap(list_);
    ExtensionInfosCallback callback = callback_;
    callback_.Reset();
    callback.Run(list);  // WARNING: |this| is possibly deleted after this line!
  }
}

}  // namespace extensions
