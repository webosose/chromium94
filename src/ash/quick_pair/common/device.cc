// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/quick_pair/common/device.h"

#include <ostream>

#include "ash/quick_pair/common/protocol.h"
#include "base/memory/scoped_refptr.h"

namespace {

std::ostream& OutputToStream(std::ostream& stream,
                             const std::string& metadata_id,
                             const std::string& address,
                             const ash::quick_pair::Protocol& protocol) {
  stream << "[Device: metadata_id=" << metadata_id << ", address=" << address
         << ", protocol=" << protocol << "]";
  return stream;
}

}  // namespace

namespace ash {
namespace quick_pair {

Device::Device(std::string metadata_id, std::string address, Protocol protocol)
    : metadata_id(std::move(metadata_id)),
      address(std::move(address)),
      protocol(protocol) {}

std::ostream& operator<<(std::ostream& stream, const Device& device) {
  return OutputToStream(stream, device.metadata_id, device.address,
                        device.protocol);
}

std::ostream& operator<<(std::ostream& stream, scoped_refptr<Device> device) {
  return OutputToStream(stream, device->metadata_id, device->address,
                        device->protocol);
}

}  // namespace quick_pair
}  // namespace ash
