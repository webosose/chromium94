// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_FULL_RESTORE_FULL_RESTORE_ARC_TASK_HANDLER_H_
#define CHROME_BROWSER_ASH_FULL_RESTORE_FULL_RESTORE_ARC_TASK_HANDLER_H_

#include <utility>

#include "base/scoped_observation.h"
#include "chrome/browser/ash/arc/session/arc_session_manager_observer.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/common/buildflags.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

namespace ash {
namespace full_restore {

class ArcAppLaunchHandler;
class ArcWindowHandler;

// The FullRestoreArcTaskHandler class observes ArcAppListPrefs, and calls
// FullRestoreSaveHandler to update the ARC app launch info when a task is
// created or destroyed.
//
// FullRestoreArcTaskHandler is an independent KeyedService so that it could be
// created along with ARC system rather than with FullRestoreService.
class FullRestoreArcTaskHandler : public KeyedService,
                                  public ArcAppListPrefs::Observer,
                                  public arc::ArcSessionManagerObserver {
 public:
  static FullRestoreArcTaskHandler* GetForProfile(Profile* profile);

  explicit FullRestoreArcTaskHandler(Profile* profile);
  FullRestoreArcTaskHandler(const FullRestoreArcTaskHandler&) = delete;
  FullRestoreArcTaskHandler& operator=(const FullRestoreArcTaskHandler&) =
      delete;

  ~FullRestoreArcTaskHandler() override;

#if BUILDFLAG(ENABLE_WAYLAND_SERVER)
  ArcWindowHandler* window_handler() { return window_handler_.get(); }
#endif

  ArcAppLaunchHandler* arc_app_launch_handler() {
    return arc_app_launch_handler_.get();
  }

  // ArcAppListPrefs::Observer.
  void OnTaskCreated(int32_t task_id,
                     const std::string& package_name,
                     const std::string& activity,
                     const std::string& intent,
                     int32_t session_id) override;
  void OnTaskDestroyed(int task_id) override;
  void OnTaskDescriptionChanged(int32_t task_id,
                                const std::string& label,
                                const arc::mojom::RawIconPngData& icon,
                                uint32_t primary_color,
                                uint32_t status_bar_color) override;
  void OnAppConnectionReady() override;
  void OnArcAppListPrefsDestroyed() override;

  // arc::ArcSessionManagerObserver:
  void OnArcPlayStoreEnabledChanged(bool enabled) override;

  // Invoked when ChromeShelfController is created.
  void OnShelfReady();

 private:
  // KeyedService:
  void Shutdown() override;

  base::ScopedObservation<ArcAppListPrefs, ArcAppListPrefs::Observer>
      arc_prefs_observer_{this};

#if BUILDFLAG(ENABLE_WAYLAND_SERVER)
  std::unique_ptr<ArcWindowHandler> window_handler_;
#endif

  std::unique_ptr<ArcAppLaunchHandler> arc_app_launch_handler_;
};

}  // namespace full_restore
}  // namespace ash

#endif  // CHROME_BROWSER_ASH_FULL_RESTORE_FULL_RESTORE_ARC_TASK_HANDLER_H_
