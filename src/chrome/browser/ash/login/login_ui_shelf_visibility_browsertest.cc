// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/constants/ash_switches.h"
#include "ash/public/cpp/login_screen_test_api.h"
#include "chrome/browser/ash/login/login_manager_test.h"
#include "chrome/browser/ash/login/test/device_state_mixin.h"
#include "chrome/browser/ash/login/test/embedded_test_server_mixin.h"
#include "chrome/browser/ash/login/test/fake_gaia_mixin.h"
#include "chrome/browser/ash/login/test/kiosk_apps_mixin.h"
#include "chrome/browser/ash/login/test/login_manager_mixin.h"
#include "chrome/browser/ash/login/test/oobe_auth_page_waiter.h"
#include "chrome/browser/ash/login/test/oobe_screen_waiter.h"
#include "chrome/browser/ash/login/ui/login_display_host.h"
#include "chrome/browser/ash/login/wizard_controller.h"
#include "chrome/browser/ui/webui/chromeos/login/gaia_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/oobe_ui.h"
#include "chrome/browser/ui/webui/chromeos/login/os_install_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/sync_consent_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/user_creation_screen_handler.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"

namespace chromeos {
namespace {

constexpr char kExistingUserEmail[] = "existing@gmail.com";
constexpr char kExistingUserGaiaId[] = "9876543210";

constexpr char kNewUserEmail[] = "new@gmail.com";
constexpr char kNewUserGaiaId[] = "0123456789";

class LoginUIShelfVisibilityTest : public MixinBasedInProcessBrowserTest {
 public:
  LoginUIShelfVisibilityTest() = default;
  ~LoginUIShelfVisibilityTest() override = default;

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    MixinBasedInProcessBrowserTest::SetUpOnMainThread();
  }

 protected:
  void StartOnboardingFlow() {
    auto autoreset = WizardController::ForceBrandedBuildForTesting(true);
    EXPECT_TRUE(ash::LoginScreenTestApi::ClickAddUserButton());
    OobeScreenWaiter(UserCreationView::kScreenId).Wait();
    LoginDisplayHost::default_host()
        ->GetOobeUI()
        ->GetView<GaiaScreenHandler>()
        ->ShowSigninScreenForTest(kNewUserEmail, kNewUserGaiaId,
                                  FakeGaiaMixin::kEmptyUserServices);

    // Sync consent is the first post-login screen shown when a new user signs
    // in.
    OobeScreenWaiter(SyncConsentScreenView::kScreenId).Wait();
  }

 private:
  LoginManagerMixin::TestUserInfo test_user_{
      AccountId::FromUserEmailGaiaId(kExistingUserEmail, kExistingUserGaiaId)};
  LoginManagerMixin login_manager_mixin_{&mixin_host_, {test_user_}};
  EmbeddedTestServerSetupMixin test_server_mixin_{&mixin_host_,
                                                  embedded_test_server()};
  FakeGaiaMixin fake_gaia_mixin_{&mixin_host_, embedded_test_server()};

  DISALLOW_COPY_AND_ASSIGN(LoginUIShelfVisibilityTest);
};

class OsInstallVisibilityTest : public LoginUIShelfVisibilityTest {
 public:
  OsInstallVisibilityTest() = default;
  ~OsInstallVisibilityTest() override = default;
  OsInstallVisibilityTest(const OsInstallVisibilityTest&) = delete;
  void operator=(const OsInstallVisibilityTest&) = delete;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    LoginUIShelfVisibilityTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kAllowOsInstall);
  }
};
}  // namespace

// Verifies that shelf buttons are shown by default on login screen.
IN_PROC_BROWSER_TEST_F(LoginUIShelfVisibilityTest, DefaultVisibility) {
  EXPECT_TRUE(ash::LoginScreenTestApi::IsGuestButtonShown());
  EXPECT_TRUE(ash::LoginScreenTestApi::IsAddUserButtonShown());
}

// Verifies that guest button, add user button and enterprise enrollment button
// are hidden when Gaia dialog is shown.
IN_PROC_BROWSER_TEST_F(LoginUIShelfVisibilityTest, GaiaDialogOpen) {
  EXPECT_TRUE(ash::LoginScreenTestApi::ClickAddUserButton());
  OobeScreenWaiter(UserCreationView::kScreenId).Wait();
  EXPECT_FALSE(ash::LoginScreenTestApi::IsGuestButtonShown());
  EXPECT_FALSE(ash::LoginScreenTestApi::IsAddUserButtonShown());
  EXPECT_FALSE(ash::LoginScreenTestApi::IsEnterpriseEnrollmentButtonShown());
}

// Verifies that guest button and add user button are hidden on post-login
// screens, after a user session is started.
IN_PROC_BROWSER_TEST_F(LoginUIShelfVisibilityTest, PostLoginScreen) {
  StartOnboardingFlow();

  EXPECT_FALSE(ash::LoginScreenTestApi::IsGuestButtonShown());
  EXPECT_FALSE(ash::LoginScreenTestApi::IsAddUserButtonShown());
}

// Verifies that OS install button is shown by default on login screen.
IN_PROC_BROWSER_TEST_F(OsInstallVisibilityTest, DefaultVisibility) {
  EXPECT_TRUE(ash::LoginScreenTestApi::IsOsInstallButtonShown());
}

// Verifies that OS install button is hidden when Gaia dialog is shown.
IN_PROC_BROWSER_TEST_F(OsInstallVisibilityTest, GaiaDialogOpen) {
  EXPECT_TRUE(ash::LoginScreenTestApi::ClickAddUserButton());
  OobeScreenWaiter(UserCreationView::kScreenId).Wait();
  EXPECT_FALSE(ash::LoginScreenTestApi::IsOsInstallButtonShown());
}

// Verifies that guest button, add user button, enterprise enrollment button and
// OS install button are hidden when os-install dialog is shown.
IN_PROC_BROWSER_TEST_F(OsInstallVisibilityTest, OsInstallDialogOpen) {
  EXPECT_TRUE(ash::LoginScreenTestApi::ClickOsInstallButton());
  OobeScreenWaiter(OsInstallScreenView::kScreenId).Wait();
  EXPECT_FALSE(ash::LoginScreenTestApi::IsGuestButtonShown());
  EXPECT_FALSE(ash::LoginScreenTestApi::IsAddUserButtonShown());
  EXPECT_FALSE(ash::LoginScreenTestApi::IsEnterpriseEnrollmentButtonShown());
  EXPECT_FALSE(ash::LoginScreenTestApi::IsOsInstallButtonShown());
}

// Verifies that OS install is hidden on post-login screens.
IN_PROC_BROWSER_TEST_F(OsInstallVisibilityTest, PostLoginScreen) {
  StartOnboardingFlow();
  EXPECT_FALSE(ash::LoginScreenTestApi::IsOsInstallButtonShown());
}

class SamlInterstitialTest : public LoginManagerTest {
 public:
  // LoginManagerTest:
  void SetUpInProcessBrowserTestFixture() override {
    auto device_policy_update = device_state_.RequestDevicePolicyUpdate();

    device_policy_update->policy_payload()
        ->mutable_login_authentication_behavior()
        ->set_login_authentication_behavior(
            enterprise_management::
                LoginAuthenticationBehaviorProto_LoginBehavior_SAML_INTERSTITIAL);

    KioskAppsMixin::AppendKioskAccount(device_policy_update->policy_payload());

    device_policy_update.reset();

    device_state_.RequestDeviceLocalAccountPolicyUpdate(
        KioskAppsMixin::kEnterpriseKioskAccountId);
    LoginManagerTest::SetUpInProcessBrowserTestFixture();
  }

 private:
  DeviceStateMixin device_state_{
      &mixin_host_, DeviceStateMixin::State::OOBE_COMPLETED_CLOUD_ENROLLED};
  KioskAppsMixin kiosk_apps_{&mixin_host_, embedded_test_server()};
};

// Verifies that Apps and Guest buttons are visible when login flow starts from
// the SAML interstitial step.
IN_PROC_BROWSER_TEST_F(SamlInterstitialTest, AppsGuestButton) {
  KioskAppsMixin::WaitForAppsButton();
  EXPECT_TRUE(ash::LoginScreenTestApi::IsAppsButtonShown());
  EXPECT_TRUE(ash::LoginScreenTestApi::IsGuestButtonShown());
}

}  // namespace chromeos
