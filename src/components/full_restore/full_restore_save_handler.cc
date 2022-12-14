// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/full_restore/full_restore_save_handler.h"

#include "ash/constants/app_types.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/files/file_path.h"
#include "base/no_destructor.h"
#include "base/sequenced_task_runner.h"
#include "base/time/time.h"
#include "components/full_restore/app_launch_info.h"
#include "components/full_restore/full_restore_file_handler.h"
#include "components/full_restore/full_restore_info.h"
#include "components/full_restore/full_restore_read_handler.h"
#include "components/full_restore/full_restore_utils.h"
#include "components/full_restore/restore_data.h"
#include "components/full_restore/window_info.h"
#include "components/services/app_service/public/cpp/app_registry_cache.h"
#include "components/sessions/core/session_id.h"
#include "extensions/common/constants.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/env.h"

namespace full_restore {

namespace {

// Delay between when an update is received, and when we save it to the
// full restore file.
constexpr base::TimeDelta kSaveDelay = base::TimeDelta::FromMilliseconds(2500);

// Delay starting `save_timer_` during the system startup phase.
constexpr base::TimeDelta kWaitDelay = base::TimeDelta::FromSeconds(120);

const char kCrxAppPrefix[] = "_crx_";

std::string GetAppIdFromAppName(const std::string& app_name) {
  std::string prefix(kCrxAppPrefix);
  if (app_name.substr(0, prefix.length()) != prefix)
    return std::string();
  return app_name.substr(prefix.length());
}

}  // namespace

FullRestoreSaveHandler* FullRestoreSaveHandler::GetInstance() {
  static base::NoDestructor<FullRestoreSaveHandler> full_restore_save_handler;
  return full_restore_save_handler.get();
}

FullRestoreSaveHandler::FullRestoreSaveHandler() {
  if (aura::Env::HasInstance())
    env_observer_.Observe(aura::Env::GetInstance());
}

FullRestoreSaveHandler::~FullRestoreSaveHandler() = default;

void FullRestoreSaveHandler::SetPrimaryProfilePath(
    const base::FilePath& profile_path) {
  primary_profile_path_ = profile_path;
  arc_save_handler_ = std::make_unique<ArcSaveHandler>(primary_profile_path_);
}

void FullRestoreSaveHandler::SetActiveProfilePath(
    const base::FilePath& profile_path) {
  active_profile_path_ = profile_path;
}

void FullRestoreSaveHandler::SetAppRegistryCache(
    const base::FilePath& profile_path,
    apps::AppRegistryCache* app_registry_cache) {
  if (app_registry_cache)
    profile_path_to_app_registry_cache_[profile_path] = app_registry_cache;
  else
    profile_path_to_app_registry_cache_.erase(profile_path);
}

void FullRestoreSaveHandler::AllowSave() {
  if (allow_save_)
    return;

  allow_save_ = true;

  if (wait_timer_.IsRunning())
    wait_timer_.Stop();

  if (!is_shut_down_ &&
      base::Contains(pending_save_profile_paths_, active_profile_path_)) {
    MaybeStartSaveTimer(active_profile_path_);
  }
}

void FullRestoreSaveHandler::SetShutDown() {
  is_shut_down_ = true;
}

void FullRestoreSaveHandler::OnWindowInitialized(aura::Window* window) {
  if (window->GetProperty(aura::client::kAppType) ==
      static_cast<int>(ash::AppType::ARC_APP)) {
    observed_windows_.AddObservation(window);

    if (arc_save_handler_)
      arc_save_handler_->OnWindowInitialized(window);

    ++window_count_;
    return;
  }

  int32_t window_id = window->GetProperty(::full_restore::kWindowIdKey);
  if (!SessionID::IsValidValue(window_id))
    return;

  ++window_count_;
  observed_windows_.AddObservation(window);

  std::string* app_id_str = window->GetProperty(::full_restore::kAppIdKey);
  AppLaunchInfoPtr app_launch_info;

  if (app_id_str) {
    // For Chrome apps, launched via event, get the app id from the window's
    // property, then find the app launch info from
    // |app_id_to_app_launch_infos_| to save the app launch info for |app_id|
    // and |window_id|.
    auto it = app_id_to_app_launch_infos_.find(*app_id_str);
    if (it == app_id_to_app_launch_infos_.end())
      return;

    auto launch_it = it->second.find(active_profile_path_);
    if (launch_it == it->second.end())
      return;

    DCHECK(!launch_it->second.empty());
    app_launch_info = std::move(*launch_it->second.begin());
    app_launch_info->window_id = window_id;
    it->second.erase(active_profile_path_);
    if (it->second.empty())
      app_id_to_app_launch_infos_.erase(it);
  } else {
    app_launch_info = std::make_unique<AppLaunchInfo>(
        extension_misc::kChromeAppId, window_id);

    // If the window is an app type browser window, set `app_type_browser` as
    // true, to call the browser session restore to restore apps for the next
    // system startup.
    if (window->GetProperty(full_restore::kAppTypeBrowser)) {
      app_launch_info->app_type_browser = true;

      std::string* browser_app_name =
          window->GetProperty(full_restore::kBrowserAppNameKey);
      if (browser_app_name) {
        std::string app_id = GetAppIdFromAppName(*browser_app_name);
        auto it =
            profile_path_to_app_registry_cache_.find(active_profile_path_);
        if (it != profile_path_to_app_registry_cache_.end() && it->second &&
            it->second->GetAppType(app_id) == apps::mojom::AppType::kUnknown) {
          // If the app doesn't exist in AppRegistryCache, this window is an
          // extension window, and we don't need to save the launch info for the
          // extension.
          return;
        }
      }
    }
  }

  AddAppLaunchInfo(active_profile_path_, std::move(app_launch_info));

  FullRestoreInfo::GetInstance()->OnAppLaunched(window);
}

void FullRestoreSaveHandler::OnWindowDestroyed(aura::Window* window) {
  DCHECK(observed_windows_.IsObservingSource(window));
  observed_windows_.RemoveObservation(window);

  int32_t window_id = window->GetProperty(::full_restore::kWindowIdKey);

  if (window->GetProperty(aura::client::kAppType) ==
      static_cast<int>(ash::AppType::ARC_APP)) {
    if (arc_save_handler_)
      arc_save_handler_->OnWindowDestroyed(window);
    return;
  }

  DCHECK(SessionID::IsValidValue(window_id));

  RemoveAppRestoreData(window_id);
}

void FullRestoreSaveHandler::SaveAppLaunchInfo(
    const base::FilePath& profile_path,
    AppLaunchInfoPtr app_launch_info) {
  if (profile_path.empty() || !app_launch_info)
    return;

  const std::string app_id = app_launch_info->app_id;

  if (app_launch_info->arc_session_id.has_value()) {
    if (arc_save_handler_)
      arc_save_handler_->SaveAppLaunchInfo(std::move(app_launch_info));
    return;
  }

  if (!app_launch_info->window_id.has_value()) {
    // For Chrome apps, save |app_launch_info| to |app_id_to_app_launch_infos_|,
    // and wait for the window to be initialized to get the window id.
    app_id_to_app_launch_infos_[app_id][profile_path].push_back(
        std::move(app_launch_info));
    return;
  }

  const int window_id = app_launch_info->window_id.value();
  std::unique_ptr<WindowInfo> window_info;

  if (app_id != extension_misc::kChromeAppId) {
    // For browser windows, it could have been saved as
    // extension_misc::kChromeAppId in OnWindowInitialized. However, for the
    // system web apps, we need to save as the system web app app id and the
    // launch parameter, because system web apps can't be restored by the
    // browser session restore. So remove the record in
    // extension_misc::kChromeAppId, save the launch info as the system web
    // app id, and move window info to the record of the system web app id.
    auto it = window_id_to_app_restore_info_.find(window_id);
    if (it != window_id_to_app_restore_info_.end()) {
      window_info =
          profile_path_to_restore_data_[it->second.first].GetWindowInfo(
              it->second.second, window_id);
      RemoveAppRestoreData(window_id);
    }
  }

  AddAppLaunchInfo(profile_path, std::move(app_launch_info));

  if (window_info) {
    profile_path_to_restore_data_[profile_path].ModifyWindowInfo(
        app_id, window_id, *window_info);
  }
}

void FullRestoreSaveHandler::SaveWindowInfo(const WindowInfo& window_info) {
  if (!window_info.window)
    return;

  if (window_info.window->GetProperty(aura::client::kAppType) ==
      static_cast<int>(ash::AppType::ARC_APP)) {
    if (arc_save_handler_)
      arc_save_handler_->ModifyWindowInfo(window_info);
    return;
  }

  int32_t window_id =
      window_info.window->GetProperty(::full_restore::kWindowIdKey);

  if (!SessionID::IsValidValue(window_id))
    return;

  ModifyWindowInfo(window_id, window_info);
}

void FullRestoreSaveHandler::OnTaskCreated(const std::string& app_id,
                                           int32_t task_id,
                                           int32_t session_id) {
  if (arc_save_handler_)
    arc_save_handler_->OnTaskCreated(app_id, task_id, session_id);
}

void FullRestoreSaveHandler::OnTaskDestroyed(int32_t task_id) {
  if (arc_save_handler_)
    arc_save_handler_->OnTaskDestroyed(task_id);
}

void FullRestoreSaveHandler::OnTaskThemeColorUpdated(
    int32_t task_id,
    uint32_t primary_color,
    uint32_t status_bar_color) {
  if (arc_save_handler_) {
    arc_save_handler_->OnTaskThemeColorUpdated(task_id, primary_color,
                                               status_bar_color);
  }
}

void FullRestoreSaveHandler::Flush(const base::FilePath& profile_path) {
  if (save_running_.find(profile_path) != save_running_.end())
    return;

  save_running_.insert(profile_path);

  BackendTaskRunner(profile_path)
      ->PostTaskAndReply(
          FROM_HERE,
          base::BindOnce(&FullRestoreFileHandler::WriteToFile,
                         GetFileHandler(profile_path),
                         profile_path_to_restore_data_[profile_path].Clone()),
          base::BindOnce(&FullRestoreSaveHandler::OnSaveFinished,
                         weak_factory_.GetWeakPtr(), profile_path));
}

bool FullRestoreSaveHandler::HasAppRestoreData(
    const base::FilePath& profile_path,
    const std::string& app_id,
    int32_t window_id) {
  auto it = profile_path_to_restore_data_.find(profile_path);
  if (it == profile_path_to_restore_data_.end())
    return false;

  return it->second.HasAppRestoreData(app_id, window_id);
}

void FullRestoreSaveHandler::AddAppLaunchInfo(
    const base::FilePath& profile_path,
    AppLaunchInfoPtr app_launch_info) {
  if (profile_path.empty() || !app_launch_info ||
      !app_launch_info->window_id.has_value()) {
    return;
  }

  if (!app_launch_info->arc_session_id.has_value()) {
    // If |app_launch_info| is not an ARC app launch info, add to
    // |window_id_to_app_restore_info_|.
    window_id_to_app_restore_info_[app_launch_info->window_id.value()] =
        std::make_pair(profile_path, app_launch_info->app_id);
  }

  // Each user should have one full restore file saving the restore data in the
  // profile directory |profile_path|. So |app_launch_info| is saved to the
  // restore data for the user with |profile_path|.
  profile_path_to_restore_data_[profile_path].AddAppLaunchInfo(
      std::move(app_launch_info));

  pending_save_profile_paths_.insert(profile_path);

  MaybeStartSaveTimer(profile_path);
}

void FullRestoreSaveHandler::ModifyWindowId(const base::FilePath& profile_path,
                                            const std::string& app_id,
                                            int32_t old_window_id,
                                            int32_t new_window_id) {
  auto it = profile_path_to_restore_data_.find(profile_path);
  if (it == profile_path_to_restore_data_.end())
    return;

  profile_path_to_restore_data_[profile_path].ModifyWindowId(
      app_id, old_window_id, new_window_id);

  pending_save_profile_paths_.insert(profile_path);

  MaybeStartSaveTimer(profile_path);
}

void FullRestoreSaveHandler::ModifyWindowInfo(
    const base::FilePath& profile_path,
    const std::string& app_id,
    int32_t window_id,
    const WindowInfo& window_info) {
  auto it = profile_path_to_restore_data_.find(profile_path);
  if (it == profile_path_to_restore_data_.end())
    return;

  profile_path_to_restore_data_[profile_path].ModifyWindowInfo(
      app_id, window_id, window_info);

  pending_save_profile_paths_.insert(profile_path);

  MaybeStartSaveTimer(profile_path);
}

void FullRestoreSaveHandler::ModifyThemeColor(
    const base::FilePath& profile_path,
    const std::string& app_id,
    int32_t window_id,
    uint32_t primary_color,
    uint32_t status_bar_color) {
  auto it = profile_path_to_restore_data_.find(profile_path);
  if (it == profile_path_to_restore_data_.end())
    return;

  profile_path_to_restore_data_[profile_path].ModifyThemeColor(
      app_id, window_id, primary_color, status_bar_color);

  pending_save_profile_paths_.insert(profile_path);

  MaybeStartSaveTimer(profile_path);
}

void FullRestoreSaveHandler::RemoveApp(const base::FilePath& profile_path,
                                       const std::string& app_id) {
  auto it = profile_path_to_restore_data_.find(profile_path);
  if (it == profile_path_to_restore_data_.end())
    return;

  it->second.RemoveApp(app_id);

  pending_save_profile_paths_.insert(profile_path);

  MaybeStartSaveTimer(profile_path);
}

void FullRestoreSaveHandler::RemoveAppRestoreData(
    const base::FilePath& profile_path,
    const std::string& app_id,
    int window_id) {
  auto it = profile_path_to_restore_data_.find(profile_path);
  if (it == profile_path_to_restore_data_.end())
    return;

  it->second.RemoveAppRestoreData(app_id, window_id);

  pending_save_profile_paths_.insert(profile_path);

  MaybeStartSaveTimer(profile_path);
}

void FullRestoreSaveHandler::RemoveWindowInfo(
    const base::FilePath& profile_path,
    const std::string& app_id,
    int window_id) {
  auto it = profile_path_to_restore_data_.find(profile_path);
  if (it == profile_path_to_restore_data_.end())
    return;

  it->second.RemoveWindowInfo(app_id, window_id);

  pending_save_profile_paths_.insert(profile_path);

  MaybeStartSaveTimer(profile_path);
}

void FullRestoreSaveHandler::ClearRestoreData(
    const base::FilePath& profile_path) {
  pending_save_profile_paths_.insert(profile_path);

  MaybeStartSaveTimer(profile_path);
}

int32_t FullRestoreSaveHandler::GetArcSessionId() {
  if (!arc_save_handler_)
    return -1;
  return arc_save_handler_->GetArcSessionId();
}

const RestoreData* FullRestoreSaveHandler::GetRestoreData(
    const base::FilePath& profile_path) {
  auto it = profile_path_to_restore_data_.find(profile_path);
  if (it == profile_path_to_restore_data_.end())
    return nullptr;
  return &(profile_path_to_restore_data_[profile_path]);
}

std::string FullRestoreSaveHandler::GetAppId(aura::Window* window) {
  DCHECK(window);
  if (window->GetProperty(aura::client::kAppType) ==
      static_cast<int>(ash::AppType::ARC_APP)) {
    return arc_save_handler_ ? arc_save_handler_->GetAppId(window)
                             : std::string();
  } else {
    // For other window types (browser, PWAs, SWAs, Chrome apps), get its
    // corresponding app id from |window_id_to_app_restore_info_|.
    const int32_t window_id = window->GetProperty(kWindowIdKey);
    auto iter = window_id_to_app_restore_info_.find(window_id);
    return iter != window_id_to_app_restore_info_.end() ? iter->second.second
                                                        : std::string();
  }
}

void FullRestoreSaveHandler::ClearForTesting() {
  profile_path_to_file_handler_.clear();
  profile_path_to_restore_data_.clear();
  app_id_to_app_launch_infos_.clear();
  primary_profile_path_.clear();
  save_running_.clear();
  pending_save_profile_paths_.clear();
  window_id_to_app_restore_info_.clear();
  app_id_to_app_launch_infos_.clear();
  is_shut_down_ = false;
  allow_save_ = false;
  save_timer_.Stop();
  wait_timer_.Stop();
}

void FullRestoreSaveHandler::MaybeStartSaveTimer(
    const base::FilePath& profile_path) {
  if (!base::Contains(been_read_profile_paths_, profile_path)) {
    // FullRestoreSaveHandler might be called to save the help app before
    // FullRestoreAppLaunchHandler reads the full restore data from the full
    // restore file during the system startup phase, e.g. when a new user login.
    // So call FullRestoreReadHandler to read the file before saving the new
    // data.
    FullRestoreReadHandler::GetInstance()->ReadFromFile(profile_path,
                                                        base::DoNothing());
    been_read_profile_paths_.insert(profile_path);
  }

  if (!allow_save_) {
    if (!wait_timer_.IsRunning()) {
      wait_timer_.Start(FROM_HERE, kWaitDelay,
                        base::BindOnce(&FullRestoreSaveHandler::AllowSave,
                                       weak_factory_.GetWeakPtr()));
    }
    return;
  }

  if (!save_timer_.IsRunning() && save_running_.empty()) {
    save_timer_.Start(FROM_HERE, kSaveDelay,
                      base::BindOnce(&FullRestoreSaveHandler::Save,
                                     weak_factory_.GetWeakPtr()));
  }
}

void FullRestoreSaveHandler::Save() {
  if (is_shut_down_ ||
      !base::Contains(pending_save_profile_paths_, active_profile_path_)) {
    return;
  }

  Flush(active_profile_path_);

  pending_save_profile_paths_.erase(active_profile_path_);
}

void FullRestoreSaveHandler::OnSaveFinished(
    const base::FilePath& profile_path) {
  save_running_.erase(profile_path);
}

FullRestoreFileHandler* FullRestoreSaveHandler::GetFileHandler(
    const base::FilePath& profile_path) {
  if (profile_path_to_file_handler_.find(profile_path) ==
      profile_path_to_file_handler_.end()) {
    profile_path_to_file_handler_[profile_path] =
        base::MakeRefCounted<FullRestoreFileHandler>(profile_path);
  }
  return profile_path_to_file_handler_[profile_path].get();
}

base::SequencedTaskRunner* FullRestoreSaveHandler::BackendTaskRunner(
    const base::FilePath& profile_path) {
  return GetFileHandler(profile_path)->owning_task_runner();
}

void FullRestoreSaveHandler::ModifyWindowInfo(int window_id,
                                              const WindowInfo& window_info) {
  auto it = window_id_to_app_restore_info_.find(window_id);
  if (it == window_id_to_app_restore_info_.end())
    return;

  const base::FilePath& profile_path = it->second.first;
  const std::string& app_id = it->second.second;
  ModifyWindowInfo(profile_path, app_id, window_id, window_info);
}

void FullRestoreSaveHandler::RemoveAppRestoreData(int window_id) {
  auto it = window_id_to_app_restore_info_.find(window_id);
  if (it == window_id_to_app_restore_info_.end())
    return;

  const base::FilePath& profile_path = it->second.first;
  const std::string& app_id = it->second.second;

  RemoveAppRestoreData(profile_path, app_id, window_id);

  window_id_to_app_restore_info_.erase(it);
}

}  // namespace full_restore
