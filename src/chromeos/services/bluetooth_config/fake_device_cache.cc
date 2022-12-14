// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/bluetooth_config/fake_device_cache.h"

namespace chromeos {
namespace bluetooth_config {

FakeDeviceCache::FakeDeviceCache(
    AdapterStateController* adapter_state_controller)
    : DeviceCache(adapter_state_controller) {}

FakeDeviceCache::~FakeDeviceCache() = default;

void FakeDeviceCache::SetPairedDevices(
    std::vector<mojom::PairedBluetoothDevicePropertiesPtr> paired_devices) {
  paired_devices_ = std::move(paired_devices);
  NotifyPairedDevicesListChanged();
}

std::vector<mojom::PairedBluetoothDevicePropertiesPtr>
FakeDeviceCache::PerformGetPairedDevices() const {
  std::vector<mojom::PairedBluetoothDevicePropertiesPtr> paired_devices;
  for (const auto& paired_device : paired_devices_)
    paired_devices.push_back(paired_device.Clone());
  return paired_devices;
}

}  // namespace bluetooth_config
}  // namespace chromeos
