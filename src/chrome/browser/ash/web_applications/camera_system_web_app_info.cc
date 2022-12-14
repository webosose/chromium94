// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/web_applications/camera_system_web_app_info.h"

#include "ash/constants/ash_pref_names.h"
#include "chrome/browser/ash/web_applications/chrome_camera_app_ui_constants.h"
#include "chrome/browser/ash/web_applications/system_web_app_install_utils.h"
#include "chrome/browser/web_applications/components/web_app_constants.h"
#include "chrome/browser/web_applications/components/web_application_info.h"
#include "chromeos/components/camera_app_ui/resources/strings/grit/chromeos_camera_app_strings.h"
#include "chromeos/components/camera_app_ui/url_constants.h"
#include "chromeos/grit/chromeos_camera_app_resources.h"
#include "components/prefs/pref_service.h"
#include "extensions/common/constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect.h"

namespace {
constexpr gfx::Size CAMERA_WINDOW_DEFAULT_SIZE(kChromeCameraAppDefaultWidth,
                                               kChromeCameraAppDefaultHeight +
                                                   32);
}

std::unique_ptr<WebApplicationInfo> CreateWebAppInfoForCameraSystemWebApp() {
  auto info = std::make_unique<WebApplicationInfo>();
  info->start_url = GURL(chromeos::kChromeUICameraAppMainURL);
  info->scope = GURL(chromeos::kChromeUICameraAppScopeURL);

  info->title = l10n_util::GetStringUTF16(IDS_NAME);
  web_app::CreateIconInfoForSystemWebApp(
      info->start_url,
      {
          {"camera_app_icons_48.png", 48,
           IDR_CHROMEOS_CAMERA_APP_IMAGES_CAMERA_APP_ICONS_48_PNG},
          {"camera_app_icons_128.png", 128,
           IDR_CHROMEOS_CAMERA_APP_IMAGES_CAMERA_APP_ICONS_128_PNG},
          {"camera_app_icons_192.png", 192,
           IDR_CHROMEOS_CAMERA_APP_IMAGES_CAMERA_APP_ICONS_192_PNG},
      },
      *info);
  info->theme_color = 0xff000000;
  info->display_mode = blink::mojom::DisplayMode::kStandalone;
  info->open_as_window = true;
  return info;
}

gfx::Rect GetDefaultBoundsForCameraApp(Browser*) {
  gfx::Rect bounds =
      display::Screen::GetScreen()->GetDisplayForNewWindows().work_area();
  bounds.ClampToCenteredSize(CAMERA_WINDOW_DEFAULT_SIZE);
  return bounds;
}

CameraSystemAppDelegate::CameraSystemAppDelegate(Profile* profile)
    : web_app::SystemWebAppDelegate(
          web_app::SystemAppType::CAMERA,
          "Camera",
          GURL("chrome://camera-app/views/main.html"),
          profile,
          web_app::OriginTrialsMap({{web_app::GetOrigin("chrome://camera-app"),
                                     {"FileHandling"}}})) {}

std::unique_ptr<WebApplicationInfo> CameraSystemAppDelegate::GetWebAppInfo()
    const {
  return CreateWebAppInfoForCameraSystemWebApp();
}

std::vector<web_app::AppId>
CameraSystemAppDelegate::GetAppIdsToUninstallAndReplace() const {
  if (!profile_->GetPrefs()->GetBoolean(
          chromeos::prefs::kHasCameraAppMigratedToSWA)) {
    return {extension_misc::kCameraAppId};
  }
  return {};
}

bool CameraSystemAppDelegate::ShouldCaptureNavigations() const {
  return true;
}

gfx::Size CameraSystemAppDelegate::GetMinimumWindowSize() const {
  return {kChromeCameraAppMinimumWidth, kChromeCameraAppMinimumHeight + 32};
}

gfx::Rect CameraSystemAppDelegate::GetDefaultBounds(Browser* browser) const {
  return GetDefaultBoundsForCameraApp(browser);
}
