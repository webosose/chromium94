// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_LOGIN_UI_KIOSK_APP_MENU_CONTROLLER_H_
#define CHROME_BROWSER_ASH_LOGIN_UI_KIOSK_APP_MENU_CONTROLLER_H_

#include "ash/public/cpp/kiosk_app_menu.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "chrome/browser/ash/app_mode/kiosk_app_manager_base.h"
#include "chrome/browser/ash/app_mode/kiosk_app_manager_observer.h"

namespace ash {

// Observer class to update the Kiosk app menu when Kiosk app data is changed.
class KioskAppMenuController : public KioskAppManagerObserver {
 public:
  KioskAppMenuController();
  ~KioskAppMenuController() override;

  // Manually dispatch kiosk app data to Ash.
  void SendKioskApps();

  // KioskAppManagerObserver:
  void OnKioskAppDataChanged(const std::string& app_id) override;
  void OnKioskAppDataLoadFailure(const std::string& app_id) override;
  void OnKioskAppsSettingsChanged() override;

 private:
  void LaunchApp(const KioskAppMenuEntry& app);
  void OnMenuWillShow();

  base::ScopedMultiSourceObservation<KioskAppManagerBase,
                                     KioskAppManagerObserver>
      kiosk_observations_{this};

  base::WeakPtrFactory<KioskAppMenuController> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(KioskAppMenuController);
};

}  // namespace ash

#endif  // CHROME_BROWSER_ASH_LOGIN_UI_KIOSK_APP_MENU_CONTROLLER_H_
