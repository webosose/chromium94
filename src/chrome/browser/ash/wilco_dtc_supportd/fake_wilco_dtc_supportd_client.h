// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_WILCO_DTC_SUPPORTD_FAKE_WILCO_DTC_SUPPORTD_CLIENT_H_
#define CHROME_BROWSER_ASH_WILCO_DTC_SUPPORTD_FAKE_WILCO_DTC_SUPPORTD_CLIENT_H_

#include <vector>

#include "base/component_export.h"
#include "base/macros.h"
#include "chrome/browser/ash/wilco_dtc_supportd/wilco_dtc_supportd_client.h"
#include "chromeos/dbus/dbus_method_call_status.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ash {

class FakeWilcoDtcSupportdClient final : public WilcoDtcSupportdClient {
 public:
  FakeWilcoDtcSupportdClient();
  ~FakeWilcoDtcSupportdClient() override;

  // DBusClient overrides:
  void Init(dbus::Bus* bus) override;

  // WilcoDtcSupportdClient overrides:
  void WaitForServiceToBeAvailable(
      WaitForServiceToBeAvailableCallback callback) override;
  void BootstrapMojoConnection(base::ScopedFD fd,
                               VoidDBusMethodCallback callback) override;

  // Whether there's a pending WaitForServiceToBeAvailable call.
  int wait_for_service_to_be_available_in_flight_call_count() const;
  // If the passed optional is non-empty, then it determines the result for
  // pending and future WaitForServiceToBeAvailable calls. Otherwise, the
  // requests will stay pending.
  void SetWaitForServiceToBeAvailableResult(
      absl::optional<bool> wait_for_service_to_be_available_result);

  // Whether there's a pending BootstrapMojoConnection call.
  int bootstrap_mojo_connection_in_flight_call_count() const;
  // If the passed optional is non-empty, then it determines the result for
  // pending and future BootstrapMojoConnection calls. Otherwise, the requests
  // will stay pending.
  void SetBootstrapMojoConnectionResult(
      absl::optional<bool> bootstrap_mojo_connection_result);

 private:
  absl::optional<bool> wait_for_service_to_be_available_result_;
  std::vector<WaitForServiceToBeAvailableCallback>
      pending_wait_for_service_to_be_available_callbacks_;

  absl::optional<bool> bootstrap_mojo_connection_result_;
  std::vector<VoidDBusMethodCallback>
      pending_bootstrap_mojo_connection_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(FakeWilcoDtcSupportdClient);
};

}  // namespace ash

#endif  // CHROME_BROWSER_ASH_WILCO_DTC_SUPPORTD_FAKE_WILCO_DTC_SUPPORTD_CLIENT_H_
