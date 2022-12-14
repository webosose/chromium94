// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_BLUETOOTH_CONFIG_IN_PROCESS_INSTANCE_H_
#define CHROMEOS_SERVICES_BLUETOOTH_CONFIG_IN_PROCESS_INSTANCE_H_

#include "base/component_export.h"
#include "chromeos/services/bluetooth_config/public/mojom/cros_bluetooth_config.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace chromeos {
namespace bluetooth_config {

class Initializer;

// Binds to an instance of CrosBluetoothConfig from within the browser process.
COMPONENT_EXPORT(IN_PROCESS_BLUETOOTH_CONFIG)
void BindToInProcessInstance(
    mojo::PendingReceiver<mojom::CrosBluetoothConfig> pending_receiver);

// Overrides the in-process instance for testing purposes. To reverse this
// override, call this function, passing null for |initializer|.
COMPONENT_EXPORT(IN_PROCESS_BLUETOOTH_CONFIG)
void OverrideInProcessInstanceForTesting(Initializer* initializer);

}  // namespace bluetooth_config
}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_BLUETOOTH_CONFIG_IN_PROCESS_INSTANCE_H_
