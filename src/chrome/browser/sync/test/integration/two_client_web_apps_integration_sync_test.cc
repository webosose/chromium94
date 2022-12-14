// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/barrier_closure.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "build/chromeos_buildflags.h"
#include "chrome/browser/sync/test/integration/apps_helper.h"
#include "chrome/browser/sync/test/integration/sync_service_impl_harness.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "chrome/browser/ui/views/web_apps/web_app_integration_browsertest_base.h"
#include "chrome/browser/web_applications/web_app_registrar.h"
#include "chrome/common/chrome_features.h"
#include "content/public/test/browser_test.h"
#include "services/network/public/cpp/network_switches.h"

namespace web_app {

class TwoClientWebAppsIntegrationSyncTest
    : public SyncTest,
      public WebAppIntegrationBrowserTestBase::TestDelegate,
      public testing::WithParamInterface<std::string> {
 public:
  TwoClientWebAppsIntegrationSyncTest() : SyncTest(TWO_CLIENT), helper_(this) {
#if BUILDFLAG(IS_CHROMEOS_ASH)
    // Disable WebAppsCrosapi, so that Web Apps get synced in the Ash browser.
    scoped_feature_list_.InitAndDisableFeature(features::kWebAppsCrosapi);
#endif
  }

  // WebAppIntegrationBrowserTestBase::TestDelegate
  Browser* CreateBrowser(Profile* profile) override {
    return InProcessBrowserTest::CreateBrowser(profile);
  }

  void AddBlankTabAndShow(Browser* browser) override {
    InProcessBrowserTest::AddBlankTabAndShow(browser);
  }

  net::EmbeddedTestServer* EmbeddedTestServer() override {
    return embedded_test_server();
  }

  std::vector<Profile*> GetAllProfiles() override {
    return SyncTest::GetAllProfiles();
  }

  bool IsSyncTest() override { return true; }

  void SyncTurnOff() override {
    for (auto* client : GetSyncClients()) {
      client->StopSyncServiceAndClearData();
    }
  }

  void SyncTurnOn() override {
    for (auto* client : GetSyncClients()) {
      ASSERT_TRUE(client->StartSyncService());
    }
    ASSERT_TRUE(AwaitQuiescence());
    apps_helper::AwaitWebAppQuiescence(GetAllProfiles());
  }

  WebAppIntegrationBrowserTestBase helper_;

 private:
  // InProcessBrowserTest
  void SetUp() override {
    helper_.SetUp(GetChromeTestDataDir());
    SyncTest::SetUp();
  }

  // BrowserTestBase
  void SetUpOnMainThread() override {
    SyncTest::SetUpOnMainThread();
    ASSERT_TRUE(SetupClients());
    helper_.SetUpOnMainThread();
  }

  void TearDownOnMainThread() override { helper_.TearDownOnMainThread(); }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    SyncTest::SetUpCommandLine(command_line);
    ASSERT_TRUE(embedded_test_server()->Start());
    command_line->AppendSwitchASCII(
        network::switches::kUnsafelyTreatInsecureOriginAsSecure,
        helper_.GetInstallableAppURL("SiteA").GetOrigin().spec());
    command_line->AppendSwitch("disable-fake-server-failure-output");
  }

  bool SetupClients() override {
    if (!SyncTest::SetupClients()) {
      return false;
    }

    for (Profile* profile : GetAllProfiles()) {
      auto* web_app_provider = WebAppProvider::Get(profile);
      base::RunLoop loop;
      web_app_provider->on_registry_ready().Post(FROM_HERE, loop.QuitClosure());
      loop.Run();
    }
    return true;
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

// This test is a part of the web app integration test suite, which is
// documented in //chrome/browser/ui/views/web_apps/README.md. For information
// about diagnosing, debugging and/or disabling tests, please look to the
// README file.

namespace {
// TODO(jarrydg@chromium.org): Remove the macro disabling the following tests
// when they can compile. https://crbug.com/1215791
#if false
IN_PROC_BROWSER_TEST_F(
    TwoClientWebAppsIntegrationSyncTest,
    WebAppIntegration_InstOmniboxSiteA_WindowCreated_SwitchProfileClientUserAClient2_InListNotLclyInstSiteA_NavSiteA_InstIconShown_LaunchIconShown) {
  // Test contents are generated by script. Please do not modify!
  // See `chrome/test/webapps/README.md` for more info.
  // Sheriffs: Disabling this test is supported.
  helper_.BeforeStateChangeAction();
  helper_.InstallOmniboxIcon("SiteA");
  helper_.AfterStateChangeAction();

  helper_.BeforeStateCheckAction();
  helper_.CheckWindowCreated();
  helper_.AfterStateCheckAction();

  helper_.BeforeStateChangeAction();
  helper_.SwitchProfileClients();
  helper_.AfterStateChangeAction();

  helper_.BeforeStateCheckAction();
  helper_.CheckAppInListNotLocallyInstalled("SiteA");
  helper_.AfterStateCheckAction();

  helper_.BeforeStateChangeAction();
  helper_.NavigateBrowser("SiteA");
  helper_.AfterStateChangeAction();

  helper_.BeforeStateCheckAction();
  helper_.CheckInstallIconShown();
  helper_.AfterStateCheckAction();

  helper_.BeforeStateCheckAction();
  helper_.CheckLaunchIconNotShown();
  helper_.AfterStateCheckAction();
}

IN_PROC_BROWSER_TEST_F(
    TwoClientWebAppsIntegrationSyncTest,
    WebAppIntegration_InstOmniboxSiteA_WindowCreated_SwitchProfileClientUserAClient2_InListNotLclyInstSiteA_TurnSyncOff) {
  // Test contents are generated by script. Please do not modify!
  // See `chrome/test/webapps/README.md` for more info.
  // Sheriffs: Disabling this test is supported.
  helper_.BeforeStateChangeAction();
  helper_.InstallOmniboxIcon("SiteA");
  helper_.AfterStateChangeAction();

  helper_.BeforeStateCheckAction();
  helper_.CheckWindowCreated();
  helper_.AfterStateCheckAction();

  helper_.BeforeStateChangeAction();
  helper_.SwitchProfileClients();
  helper_.AfterStateChangeAction();

  helper_.BeforeStateCheckAction();
  helper_.CheckAppInListNotLocallyInstalled("SiteA");
  helper_.AfterStateCheckAction();

  helper_.BeforeStateChangeAction();
  helper_.SyncTurnOff();
  helper_.AfterStateChangeAction();
}

IN_PROC_BROWSER_TEST_F(
    TwoClientWebAppsIntegrationSyncTest,
    WebAppIntegration_InstMenuOptionSiteA_WindowCreated_SwitchProfileClientUserAClient2_InListNotLclyInstSiteA_NavSiteA_InstIconShown_LaunchIconShown) {
  // Test contents are generated by script. Please do not modify!
  // See `chrome/test/webapps/README.md` for more info.
  // Sheriffs: Disabling this test is supported.
  helper_.BeforeStateChangeAction();
  helper_.InstallMenuOption("SiteA");
  helper_.AfterStateChangeAction();

  helper_.BeforeStateCheckAction();
  helper_.CheckWindowCreated();
  helper_.AfterStateCheckAction();

  helper_.BeforeStateChangeAction();
  helper_.SwitchProfileClients();
  helper_.AfterStateChangeAction();

  helper_.BeforeStateCheckAction();
  helper_.CheckAppInListNotLocallyInstalled("SiteA");
  helper_.AfterStateCheckAction();

  helper_.BeforeStateChangeAction();
  helper_.NavigateBrowser("SiteA");
  helper_.AfterStateChangeAction();

  helper_.BeforeStateCheckAction();
  helper_.CheckInstallIconShown();
  helper_.AfterStateCheckAction();

  helper_.BeforeStateCheckAction();
  helper_.CheckLaunchIconNotShown();
  helper_.AfterStateCheckAction();
}

IN_PROC_BROWSER_TEST_F(
    TwoClientWebAppsIntegrationSyncTest,
    WebAppIntegration_InstMenuOptionSiteA_WindowCreated_SwitchProfileClientUserAClient2_InListNotLclyInstSiteA_TurnSyncOff) {
  // Test contents are generated by script. Please do not modify!
  // See `chrome/test/webapps/README.md` for more info.
  // Sheriffs: Disabling this test is supported.
  helper_.BeforeStateChangeAction();
  helper_.InstallMenuOption("SiteA");
  helper_.AfterStateChangeAction();

  helper_.BeforeStateCheckAction();
  helper_.CheckWindowCreated();
  helper_.AfterStateCheckAction();

  helper_.BeforeStateChangeAction();
  helper_.SwitchProfileClients();
  helper_.AfterStateChangeAction();

  helper_.BeforeStateCheckAction();
  helper_.CheckAppInListNotLocallyInstalled("SiteA");
  helper_.AfterStateCheckAction();

  helper_.BeforeStateChangeAction();
  helper_.SyncTurnOff();
  helper_.AfterStateChangeAction();
}
#endif

}  // namespace

}  // namespace web_app
