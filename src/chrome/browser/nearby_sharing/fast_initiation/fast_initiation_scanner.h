// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NEARBY_SHARING_FAST_INITIATION_FAST_INITIATION_SCANNER_H_
#define CHROME_BROWSER_NEARBY_SHARING_FAST_INITIATION_FAST_INITIATION_SCANNER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/containers/flat_set.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_low_energy_scan_session.h"

// FastInitiationScanner will scan for BLE advertisements using a
// BluetoothLowEnergyScanSession. It will filter for advertisements containing
// the Fast Initiation UUID 0xFE2C and service data 0xFC128E. Remote devices
// will broacast these advertisements when they are attempting to share (see
// FastInitiationAdvertiser). We use this as a signal to prompt the user to
// enable high visibility mode.
class FastInitiationScanner
    : public device::BluetoothLowEnergyScanSession::Delegate {
 public:
  class Factory {
   public:
    Factory() = default;
    Factory(const Factory&) = delete;
    Factory& operator=(const Factory&) = delete;

    static std::unique_ptr<FastInitiationScanner> Create(
        scoped_refptr<device::BluetoothAdapter> adapter);

    static void SetFactoryForTesting(Factory* factory);

   protected:
    virtual ~Factory() = default;
    virtual std::unique_ptr<FastInitiationScanner> CreateInstance(
        scoped_refptr<device::BluetoothAdapter> adapter) = 0;

   private:
    static Factory* factory_instance_;
  };

  explicit FastInitiationScanner(
      scoped_refptr<device::BluetoothAdapter> adapter);
  ~FastInitiationScanner() override;
  FastInitiationScanner(const FastInitiationScanner&) = delete;
  FastInitiationScanner& operator=(const FastInitiationScanner&) = delete;

  virtual void StartScanning(base::RepeatingClosure device_found_callback,
                             base::RepeatingClosure device_lost_callback,
                             base::OnceClosure scanner_invalidated_callback);

  virtual bool AreFastInitiationDevicesDetected() const;

 private:
  // device::BluetoothLowEnergyScanSession::Delegate:
  void OnSessionStarted(
      device::BluetoothLowEnergyScanSession* scan_session,
      absl::optional<device::BluetoothLowEnergyScanSession::ErrorCode>
          error_code) override;
  void OnDeviceFound(device::BluetoothLowEnergyScanSession* scan_session,
                     device::BluetoothDevice* device) override;
  void OnDeviceLost(device::BluetoothLowEnergyScanSession* scan_session,
                    device::BluetoothDevice* device) override;
  void OnSessionInvalidated(
      device::BluetoothLowEnergyScanSession* scan_session) override;

  scoped_refptr<device::BluetoothAdapter> adapter_;
  base::RepeatingClosure device_found_callback_;
  base::RepeatingClosure device_lost_callback_;
  base::OnceClosure scanner_invalidated_callback_;
  std::unique_ptr<device::BluetoothLowEnergyScanSession>
      background_scan_session_;
  // Set of remote devices that we detect are currently emitting fast initiation
  // advertisements.
  base::flat_set<std::string> devices_attempting_to_share_;

  base::WeakPtrFactory<FastInitiationScanner> weak_ptr_factory_{this};
};

#endif  // CHROME_BROWSER_NEARBY_SHARING_FAST_INITIATION_FAST_INITIATION_SCANNER_H_
