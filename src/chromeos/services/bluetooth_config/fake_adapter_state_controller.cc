// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/bluetooth_config/fake_adapter_state_controller.h"

#include "base/run_loop.h"

namespace chromeos {
namespace bluetooth_config {

FakeAdapterStateController::FakeAdapterStateController() = default;

FakeAdapterStateController::~FakeAdapterStateController() = default;

void FakeAdapterStateController::SetSystemState(
    mojom::BluetoothSystemState system_state) {
  if (system_state_ == system_state)
    return;

  system_state_ = system_state;
  NotifyAdapterStateChanged();
  base::RunLoop().RunUntilIdle();
}

mojom::BluetoothSystemState FakeAdapterStateController::GetAdapterState()
    const {
  return system_state_;
}

void FakeAdapterStateController::SetBluetoothEnabledState(bool enabled) {
  if (system_state_ == mojom::BluetoothSystemState::kUnavailable)
    return;

  SetSystemState(enabled ? mojom::BluetoothSystemState::kEnabling
                         : mojom::BluetoothSystemState::kDisabling);
}

}  // namespace bluetooth_config
}  // namespace chromeos
