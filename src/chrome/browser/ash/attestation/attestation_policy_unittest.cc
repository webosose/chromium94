// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/attestation/platform_verification_flow.h"
#include "chrome/browser/ash/settings/scoped_testing_cros_settings.h"
#include "chrome/browser/ash/settings/stub_cros_settings_provider.h"
#include "chromeos/settings/cros_settings_names.h"
#include "testing/gtest/include/gtest/gtest.h"

using ash::attestation::PlatformVerificationFlow;

namespace {

TEST(AttestationDevicePolicyTest, ContentProtectionTest) {
  ash::ScopedTestingCrosSettings settings;

  settings.device_settings()->SetBoolean(
      chromeos::kAttestationForContentProtectionEnabled, true);
  EXPECT_TRUE(PlatformVerificationFlow::IsAttestationAllowedByPolicy());

  settings.device_settings()->SetBoolean(
      chromeos::kAttestationForContentProtectionEnabled, false);
  EXPECT_FALSE(PlatformVerificationFlow::IsAttestationAllowedByPolicy());
}

}  // namespace
