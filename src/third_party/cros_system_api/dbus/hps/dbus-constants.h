// Copyright 2021 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_HPS_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_HPS_DBUS_CONSTANTS_H_

namespace hps {

constexpr char kHpsServiceInterface[] = "org.chromium.Hps";
constexpr char kHpsServicePath[] = "/org/chromium/Hps";
constexpr char kHpsServiceName[] = "org.chromium.Hps";

// Methods exported by hpsd.
constexpr char kGetFeatureResult[] = "GetFeatureResult";

}  // namespace hps

#endif  // SYSTEM_API_DBUS_HPS_DBUS_CONSTANTS_H_
