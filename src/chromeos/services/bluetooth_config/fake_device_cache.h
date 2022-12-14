// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_BLUETOOTH_CONFIG_FAKE_DEVICE_CACHE_H_
#define CHROMEOS_SERVICES_BLUETOOTH_CONFIG_FAKE_DEVICE_CACHE_H_

#include "chromeos/services/bluetooth_config/device_cache.h"

namespace chromeos {
namespace bluetooth_config {

class FakeDeviceCache : public DeviceCache {
 public:
  FakeDeviceCache(AdapterStateController* adapter_state_controller);
  ~FakeDeviceCache() override;

  void SetPairedDevices(
      std::vector<mojom::PairedBluetoothDevicePropertiesPtr> paired_devices);

 private:
  // DeviceCache:
  std::vector<mojom::PairedBluetoothDevicePropertiesPtr>
  PerformGetPairedDevices() const override;

  std::vector<mojom::PairedBluetoothDevicePropertiesPtr> paired_devices_;
};

}  // namespace bluetooth_config
}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_BLUETOOTH_CONFIG_FAKE_DEVICE_CACHE_H_
