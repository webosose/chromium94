// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/constants/ash_features.h"
#include "ash/shell.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ash/web_applications/system_web_app_integration_test.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/web_applications/system_web_apps/system_web_app_manager.h"
#include "chromeos/components/eche_app_ui/url_constants.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/display/screen.h"
#include "ui/display/test/display_manager_test_api.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

class EcheAppIntegrationTest : public SystemWebAppIntegrationTest {
 public:
  EcheAppIntegrationTest() {
    scoped_feature_list_.InitWithFeatures(
        /*enabled_features=*/{chromeos::features::kEcheSWA,
                              chromeos::features::kPhoneHubRecentApps},
        /*disabled_features=*/{});
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Test that the Eche App installs and launches correctly. Runs some spot checks
// on the manifest.
IN_PROC_BROWSER_TEST_P(EcheAppIntegrationTest, EcheApp) {
  const GURL url(chromeos::eche_app::kChromeUIEcheAppURL);
  EXPECT_NO_FATAL_FAILURE(
      ExpectSystemWebAppValid(web_app::SystemAppType::ECHE, url, "Eche App"));
}

// Test that the Eche App has a default bounds is always 16:9 portrait aspect
// ratio and is in the center of the screen.
IN_PROC_BROWSER_TEST_P(EcheAppIntegrationTest,
                       DefaultWindowBoundsWithLandscapeDisplay) {
  const float aspect_ratio = 16.0f / 9.0f;
  display::test::DisplayManagerTestApi display_manager_test(
      ash::Shell::Get()->display_manager());
  display_manager_test.UpdateDisplay("2000x1000");
  WaitForTestSystemAppInstall();

  Browser* browser;
  LaunchApp(web_app::SystemAppType::ECHE, &browser);

  gfx::Rect work_area =
      display::Screen::GetScreen()->GetDisplayForNewWindows().work_area();
  int expected_width = work_area.height() / 2;
  int expected_hight = work_area.height() / 2 * aspect_ratio;
  int x = (work_area.width() - expected_width) / 2;
  int y = (work_area.height() - expected_hight) / 2;
  EXPECT_EQ(browser->window()->GetBounds(),
            gfx::Rect(x, y, expected_width, expected_hight));
}

IN_PROC_BROWSER_TEST_P(EcheAppIntegrationTest,
                       DefaultWindowBoundsWithPortraitDisplay) {
  const float aspect_ratio = 16.0f / 9.0f;
  display::test::DisplayManagerTestApi display_manager_test(
      ash::Shell::Get()->display_manager());
  display_manager_test.UpdateDisplay("1000x2000");

  WaitForTestSystemAppInstall();
  Browser* browser;
  LaunchApp(web_app::SystemAppType::ECHE, &browser);

  gfx::Rect work_area =
      display::Screen::GetScreen()->GetDisplayForNewWindows().work_area();
  int expected_width = work_area.width() / 2;
  int expected_hight = work_area.width() / 2 * aspect_ratio;
  int x = (work_area.width() - expected_width) / 2;
  int y = (work_area.height() - expected_hight) / 2;
  EXPECT_EQ(browser->window()->GetBounds(),
            gfx::Rect(x, y, expected_width, expected_hight));
}

IN_PROC_BROWSER_TEST_P(EcheAppIntegrationTest,
                       DefaultWindowBoundsWithSmallDisplay) {
  gfx::Size min_size(240, 240);
  display::test::DisplayManagerTestApi display_manager_test(
      ash::Shell::Get()->display_manager());
  display_manager_test.UpdateDisplay("400x400");

  WaitForTestSystemAppInstall();
  Browser* browser;
  LaunchApp(web_app::SystemAppType::ECHE, &browser);

  EXPECT_GE(browser->window()->GetBounds().width(), min_size.width());
  EXPECT_GE(browser->window()->GetBounds().height(), min_size.height());
}

IN_PROC_BROWSER_TEST_P(EcheAppIntegrationTest, HiddenInLauncherAndSearch) {
  WaitForTestSystemAppInstall();

  // Check system_web_app_manager has the correct attributes for Eche App.
  EXPECT_FALSE(GetManager().ShouldShowInLauncher(web_app::SystemAppType::ECHE));
  EXPECT_FALSE(GetManager().ShouldShowInSearch(web_app::SystemAppType::ECHE));
}

IN_PROC_BROWSER_TEST_P(EcheAppIntegrationTest,
                       WindowNonResizeableAndNonMaximizable) {
  WaitForTestSystemAppInstall();
  Browser* browser;
  LaunchApp(web_app::SystemAppType::ECHE, &browser);
  BrowserView* const browser_view =
      BrowserView::GetBrowserViewForBrowser(browser);
  EXPECT_FALSE(browser_view->CanResize());
  EXPECT_FALSE(browser_view->CanMaximize());
}

IN_PROC_BROWSER_TEST_P(EcheAppIntegrationTest, MinimalUiWithoutReloadButton) {
  WaitForTestSystemAppInstall();
  Browser* browser;
  LaunchApp(web_app::SystemAppType::ECHE, &browser);
  BrowserView* const browser_view =
      BrowserView::GetBrowserViewForBrowser(browser);
  ToolbarButtonProvider* provider = browser_view->toolbar_button_provider();
  EXPECT_TRUE(provider->GetBackButton());
  EXPECT_FALSE(provider->GetReloadButton());
}

IN_PROC_BROWSER_TEST_P(EcheAppIntegrationTest, ShouldAllowCloseWindow) {
  WaitForTestSystemAppInstall();
  Browser* browser;
  LaunchApp(web_app::SystemAppType::ECHE, &browser);
  EXPECT_TRUE(web_app::WebAppProvider::Get(browser->profile())
                  ->system_web_app_manager()
                  .AllowScriptsToCloseWindows(web_app::SystemAppType::ECHE));
}

INSTANTIATE_SYSTEM_WEB_APP_MANAGER_TEST_SUITE_REGULAR_PROFILE_P(
    EcheAppIntegrationTest);

class EcheAppEnableResizingTest : public SystemWebAppIntegrationTest {
 public:
  EcheAppEnableResizingTest() {
    scoped_feature_list_.InitWithFeatures(
        /*enabled_features=*/{chromeos::features::kEcheSWA,
                              chromeos::features::kEcheSWAResizing,
                              chromeos::features::kPhoneHubRecentApps},
        /*disabled_features=*/{});
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_P(EcheAppEnableResizingTest,
                       WindowResizeableAfterEnableEcheSWAResizingFlag) {
  WaitForTestSystemAppInstall();
  Browser* browser;
  LaunchApp(web_app::SystemAppType::ECHE, &browser);
  BrowserView* const browser_view =
      BrowserView::GetBrowserViewForBrowser(browser);
  EXPECT_TRUE(browser_view->CanResize());
}

INSTANTIATE_SYSTEM_WEB_APP_MANAGER_TEST_SUITE_REGULAR_PROFILE_P(
    EcheAppEnableResizingTest);
