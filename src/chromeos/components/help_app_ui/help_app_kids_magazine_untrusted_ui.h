// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_HELP_APP_UI_HELP_APP_KIDS_MAGAZINE_UNTRUSTED_UI_H_
#define CHROMEOS_COMPONENTS_HELP_APP_UI_HELP_APP_KIDS_MAGAZINE_UNTRUSTED_UI_H_

#include "ui/webui/untrusted_web_ui_controller.h"
#include "ui/webui/webui_config.h"

namespace chromeos {

class HelpAppKidsMagazineUntrustedUIConfig : public ui::WebUIConfig {
 public:
  HelpAppKidsMagazineUntrustedUIConfig();
  ~HelpAppKidsMagazineUntrustedUIConfig() override;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui) override;
};

// The Web UI for chrome-untrusted://help-app-kids-magazine.
class HelpAppKidsMagazineUntrustedUI : public ui::UntrustedWebUIController {
 public:
  explicit HelpAppKidsMagazineUntrustedUI(content::WebUI* web_ui);
  HelpAppKidsMagazineUntrustedUI(const HelpAppKidsMagazineUntrustedUI&) =
      delete;
  HelpAppKidsMagazineUntrustedUI& operator=(
      const HelpAppKidsMagazineUntrustedUI&) = delete;
  ~HelpAppKidsMagazineUntrustedUI() override;
};

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_HELP_APP_UI_HELP_APP_KIDS_MAGAZINE_UNTRUSTED_UI_H_
