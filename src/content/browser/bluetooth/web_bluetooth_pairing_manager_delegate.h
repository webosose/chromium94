// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BLUETOOTH_WEB_BLUETOOTH_PAIRING_MANAGER_DELEGATE_H_
#define CONTENT_BROWSER_BLUETOOTH_WEB_BLUETOOTH_PAIRING_MANAGER_DELEGATE_H_

#include <string>

#include "base/callback_forward.h"
#include "device/bluetooth/bluetooth_device.h"
#include "third_party/blink/public/common/bluetooth/web_bluetooth_device_id.h"
#include "third_party/blink/public/mojom/bluetooth/web_bluetooth.mojom.h"

namespace content {

// A set of functions needed by whatever object (usually
// WebBluetoothServiceImpl) that embeds the WebBluetoothPairingManager and is
// separated into a separate interface for readability and testing purposes.
class WebBluetoothPairingManagerDelegate {
 public:
  // Return the cached device ID for the given characteric instance ID.
  // The returned device may be invalid - check before use.
  virtual blink::WebBluetoothDeviceId GetCharacteristicDeviceID(
      const std::string& characteristic_instance_id) = 0;

  // Return the cached device ID for the given descriptor instance ID.
  // The returned device may be invalid - check before use.
  virtual blink::WebBluetoothDeviceId GetDescriptorDeviceId(
      const std::string& descriptor_instance_id) = 0;

  // Pair the device identified by |device_id|. If successful, |callback| will
  // be run. If unsuccessful |error_callback| wil be run with the corresponding
  // error code.
  virtual void PairDevice(
      const blink::WebBluetoothDeviceId& device_id,
      device::BluetoothDevice::PairingDelegate* pairing_delegate,
      device::BluetoothDevice::ConnectCallback callback) = 0;

  // Cancels a pairing attempt to a remote device, clearing its reference to
  // the pairing delegate.
  virtual void CancelPairing(const blink::WebBluetoothDeviceId& device_id) = 0;

  // Reads the value for the characteristic identified by
  // |characteristic_instance_id|. If the value is successfully read the
  // callback will be run with WebBluetoothResult::SUCCESS and the
  // characteristic's value. If the value is not successfully read the
  // callback will be run with the corresponding error and nullptr for value.
  virtual void RemoteCharacteristicReadValue(
      const std::string& characteristic_instance_id,
      blink::mojom::WebBluetoothService::RemoteCharacteristicReadValueCallback
          callback) = 0;

  // Writes the |value| for the characteristic identified by
  // |characteristic_instance_id|. If the value is successfully written
  // |callback| will be run with WebBluetoothResult::SUCCESS. If the value is
  // not successfully written |callback| will be run with the corresponding
  // error.
  virtual void RemoteCharacteristicWriteValue(
      const std::string& characteristic_instance_id,
      const std::vector<uint8_t>& value,
      blink::mojom::WebBluetoothWriteType write_type,
      blink::mojom::WebBluetoothService::RemoteCharacteristicWriteValueCallback
          callback) = 0;

  // Reads the value for the descriptor identified by |descriptor_instance_id|.
  // If successfully read |callback| will be run with
  // WebBluetoothResult::SUCCESS and the descriptor value. If the value is not
  // successfully read the callback will be run with the corresponding error
  // and nullptr for value.
  virtual void RemoteDescriptorReadValue(
      const std::string& descriptor_instance_id,
      blink::mojom::WebBluetoothService::RemoteDescriptorReadValueCallback
          callback) = 0;

  // Writes the |value| for the descriptor identified by
  // |descriptor_instance_id|. If the value is successfully written
  // |callback| will be run with WebBluetoothResult::SUCCESS. If the value is
  // not successfully written |callback| will be run with the corresponding
  // error.
  virtual void RemoteDescriptorWriteValue(
      const std::string& descriptor_instance_id,
      const std::vector<uint8_t>& value,
      blink::mojom::WebBluetoothService::RemoteDescriptorWriteValueCallback
          callback) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_BLUETOOTH_WEB_BLUETOOTH_PAIRING_MANAGER_DELEGATE_H_
