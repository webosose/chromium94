// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_POLICY_HANDLERS_FAKE_DEVICE_NAME_POLICY_HANDLER_H_
#define CHROME_BROWSER_ASH_POLICY_HANDLERS_FAKE_DEVICE_NAME_POLICY_HANDLER_H_

#include "chrome/browser/ash/policy/handlers/device_name_policy_handler.h"

namespace policy {

// Fake DeviceNamePolicyHandler implementation
class FakeDeviceNamePolicyHandler : public DeviceNamePolicyHandler {
 public:
  FakeDeviceNamePolicyHandler();
  ~FakeDeviceNamePolicyHandler() override;

  // DeviceNamePolicyHandler:
  DeviceNamePolicy GetDeviceNamePolicy() const override;
  absl::optional<std::string> GetHostnameChosenByAdministrator() const override;

  // Sets new device name and policy if different from the current device name
  // and/or policy.
  void SetPolicyState(
      DeviceNamePolicy policy,
      const absl::optional<std::string>& hostname_from_template);

 private:
  absl::optional<std::string> hostname_ = absl::nullopt;
  DeviceNamePolicy device_name_policy_ = DeviceNamePolicy::kNoPolicy;
};

}  // namespace policy

#endif  // CHROME_BROWSER_ASH_POLICY_HANDLERS_FAKE_DEVICE_NAME_POLICY_HANDLER_H_
