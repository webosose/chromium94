// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_WILCO_DTC_SUPPORTD_WILCO_DTC_SUPPORTD_CLIENT_H_
#define CHROME_BROWSER_ASH_WILCO_DTC_SUPPORTD_WILCO_DTC_SUPPORTD_CLIENT_H_

#include "base/component_export.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "chromeos/dbus/dbus_client.h"
#include "chromeos/dbus/dbus_method_call_status.h"
#include "dbus/object_proxy.h"

namespace ash {

class WilcoDtcSupportdClient : public DBusClient {
 public:
  // Creates and initializes the global instance. |bus| must not be null.
  static void Initialize(dbus::Bus* bus);
  // Creates and initializes a fake global instance if not already created.
  static void InitializeFake();
  // Destroys the global instance which must have been initialized.
  static void Shutdown();
  // Checks if initialization was performed
  static bool IsInitialized();
  // Returns the global instance if initialized.
  static WilcoDtcSupportdClient* Get();

  // Registers |callback| to run when the wilco_dtc_supportd service becomes
  // available.
  virtual void WaitForServiceToBeAvailable(
      WaitForServiceToBeAvailableCallback callback) = 0;

  // Bootstrap the Mojo connection between Chrome and the wilco_dtc_supportd
  // daemon. |fd| is the file descriptor with the child end of the Mojo pipe.
  virtual void BootstrapMojoConnection(base::ScopedFD fd,
                                       VoidDBusMethodCallback callback) = 0;

 protected:
  // Create() should be used instead.
  WilcoDtcSupportdClient();
  ~WilcoDtcSupportdClient() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(WilcoDtcSupportdClient);
};

}  // namespace ash

// TODO(https://crbug.com/1164001): remove when Chrome OS code migration is
// done.
namespace chromeos {
using ::ash::WilcoDtcSupportdClient;
}  // namespace chromeos

#endif  // CHROME_BROWSER_ASH_WILCO_DTC_SUPPORTD_WILCO_DTC_SUPPORTD_CLIENT_H_
