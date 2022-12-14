// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_BLUETOOTH_CONFIG_DEVICE_CACHE_IMPL_H_
#define CHROMEOS_SERVICES_BLUETOOTH_CONFIG_DEVICE_CACHE_IMPL_H_

#include "base/memory/ref_counted.h"
#include "base/scoped_observation.h"
#include "chromeos/services/bluetooth_config/adapter_state_controller.h"
#include "chromeos/services/bluetooth_config/device_cache.h"
#include "chromeos/services/bluetooth_config/public/mojom/cros_bluetooth_config.mojom.h"
#include "device/bluetooth/bluetooth_adapter.h"

namespace chromeos {
namespace bluetooth_config {

// Concrete DeviceCache implementation. When this class is created, it uses
// BluetoothAdapter to fetch an initial list of devices; then, it observes
// BluetoothAdapter so that it can update the cache when devices are added,
// removed, or changed.
//
// Additionally, it uses AdapterStateController to ensure that no devices are
// returned unless Bluetooth is enabled or enabling.
class DeviceCacheImpl : public DeviceCache,
                        public AdapterStateController::Observer,
                        public device::BluetoothAdapter::Observer {
 public:
  DeviceCacheImpl(AdapterStateController* adapter_state_controller,
                  scoped_refptr<device::BluetoothAdapter> bluetooth_adapter);
  ~DeviceCacheImpl() override;

 private:
  friend class DeviceCacheImplTest;

  // DeviceCache:
  std::vector<mojom::PairedBluetoothDevicePropertiesPtr>
  PerformGetPairedDevices() const override;

  // AdapterStateController::Observer:
  void OnAdapterStateChanged() override;

  // device::BluetoothAdapter::Observer:
  void DeviceAdded(device::BluetoothAdapter* adapter,
                   device::BluetoothDevice* device) override;
  void DeviceRemoved(device::BluetoothAdapter* adapter,
                     device::BluetoothDevice* device) override;
  void DeviceChanged(device::BluetoothAdapter* adapter,
                     device::BluetoothDevice* device) override;
  void DevicePairedChanged(device::BluetoothAdapter* adapter,
                           device::BluetoothDevice* device,
                           bool new_paired_status) override;
  void DeviceConnectedStateChanged(device::BluetoothAdapter* adapter,
                                   device::BluetoothDevice* device,
                                   bool is_now_connected) override;
  void DeviceBatteryChanged(
      device::BluetoothAdapter* adapter,
      device::BluetoothDevice* device,
      absl::optional<uint8_t> new_battery_percentage) override;

  // Fetches all known devices from BluetoothAdapter.
  void FetchInitialPairedDeviceList();

  // Sorts |paired_devices_| based on connection state. This function is called
  // each time a device is added to the list. This is not particularly
  // efficient, but the list is expected to be small and is only sorted when its
  // contents change.
  void SortPairedDeviceList();

  // Adds |device| to |paired_devices_|, but only if |device| is paired. If the
  // device was already present in the list, this function updates its metadata
  // to reflect up-to-date values. This function sorts the list after a new
  // element is inserted.
  void AttemptSetDeviceInPairedDeviceList(device::BluetoothDevice* device);

  // Removes |device| from |paired_devices_| if it exists in the list.
  void RemoveFromPairedDeviceList(device::BluetoothDevice* device);

  // Attempts to add updated metadata about |device| to |paired_devices_|.
  void AttemptUpdatePairedDeviceMetadata(device::BluetoothDevice* device);

  scoped_refptr<device::BluetoothAdapter> bluetooth_adapter_;

  // Sorted by connection status.
  std::vector<mojom::PairedBluetoothDevicePropertiesPtr> paired_devices_;

  base::ScopedObservation<AdapterStateController,
                          AdapterStateController::Observer>
      adapter_state_controller_observation_{this};
  base::ScopedObservation<device::BluetoothAdapter,
                          device::BluetoothAdapter::Observer>
      adapter_observation_{this};
};

}  // namespace bluetooth_config
}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_BLUETOOTH_CONFIG_DEVICE_CACHE_IMPL_H_
