// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fake_device_name_applier.h"

namespace chromeos {

FakeDeviceNameApplier::FakeDeviceNameApplier() = default;

FakeDeviceNameApplier::~FakeDeviceNameApplier() = default;

void FakeDeviceNameApplier::SetDeviceName(const std::string& new_device_name) {
  hostname_ = new_device_name;
}

}  // namespace chromeos
