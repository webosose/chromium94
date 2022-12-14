// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_DEVICE_NAME_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_DEVICE_NAME_HANDLER_H_

#include "base/macros.h"
#include "base/scoped_observation.h"
#include "chrome/browser/chromeos/device_name_store.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

namespace base {
class ListValue;
}

namespace chromeos {
namespace settings {

// DeviceNameHandler handles calls from WebUI JS related to getting and setting
// the device name.
class DeviceNameHandler : public ::settings::SettingsPageUIHandler,
                          public chromeos::DeviceNameStore::Observer {
 public:
  DeviceNameHandler();
  ~DeviceNameHandler() override;

  // SettingsPageUIHandler implementation.
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

 protected:
  void HandleAttemptSetDeviceName(const base::ListValue* args);
  void HandleNotifyReadyForDeviceName(const base::ListValue* args);

 private:
  friend class TestDeviceNameHandler;

  // DeviceNameStore::Observer:
  void OnDeviceNameMetadataChanged() override;

  explicit DeviceNameHandler(DeviceNameStore* device_name_store);

  base::Value GetDeviceNameMetadata() const;

  DeviceNameStore* device_name_store_;

  base::ScopedObservation<chromeos::DeviceNameStore,
                          chromeos::DeviceNameStore::Observer>
      observation_{this};

  DISALLOW_COPY_AND_ASSIGN(DeviceNameHandler);
};

}  // namespace settings
}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_DEVICE_NAME_HANDLER_H_
