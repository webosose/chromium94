// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_HELP_APP_UI_HELP_APP_UNTRUSTED_UI_H_
#define CHROMEOS_COMPONENTS_HELP_APP_UI_HELP_APP_UNTRUSTED_UI_H_

#include "base/callback.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace content {
class WebUIDataSource;
}

namespace chromeos {

// The Web UI for chrome-untrusted://help-app.
class HelpAppUntrustedUI : public ui::UntrustedWebUIController {
 public:
  explicit HelpAppUntrustedUI(
      content::WebUI* web_ui,
      base::RepeatingCallback<void(content::WebUIDataSource*)>
          populate_load_time_data_callback);
  HelpAppUntrustedUI(const HelpAppUntrustedUI&) = delete;
  HelpAppUntrustedUI& operator=(const HelpAppUntrustedUI&) = delete;
  ~HelpAppUntrustedUI() override;
};

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_HELP_APP_UI_HELP_APP_UNTRUSTED_UI_H_
