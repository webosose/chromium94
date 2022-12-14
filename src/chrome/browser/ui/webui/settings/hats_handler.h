// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_HATS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_HATS_HANDLER_H_

#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

namespace settings {

// Settings page UI handler that shows HaTS surveys.
class HatsHandler : public SettingsPageUIHandler {
 public:
  HatsHandler();

  // Not copyable or movable
  HatsHandler(const HatsHandler&) = delete;
  HatsHandler& operator=(const HatsHandler&) = delete;

  ~HatsHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

  void HandleTrustSafetyInteractionOccurred(const base::ListValue* args);

 private:
  friend class HatsHandlerTest;
  FRIEND_TEST_ALL_PREFIXES(HatsHandlerTest, PrivacySettingsHats);
  FRIEND_TEST_ALL_PREFIXES(HatsHandlerTest, PrivacySandboxHats);
  FRIEND_TEST_ALL_PREFIXES(HatsHandlerTest, TrustSafetySentimentInteractions);

  // All Trust & Safety based interactions which may result in a HaTS survey.
  // Must be kept in sync with the enum of the same name in
  // hats_browser_proxy.js
  enum class TrustSafetyInteraction {
    RAN_SAFETY_CHECK = 0,
    USED_PRIVACY_CARD = 1,
    OPENED_PRIVACY_SANDBOX = 2,
    OPENED_PASSWORD_MANAGER = 3,
  };

  // Requests the appropriate HaTS survey, which may be none, for |interaction|.
  void RequestHatsSurvey(TrustSafetyInteraction interaction);

  // Informs the sentiment service, if appropriate, that |interaction| occurred.
  void InformSentimentService(TrustSafetyInteraction interaction);

  // SettingsPageUIHandler implementation.
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}
};

}  // namespace settings

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_HATS_HANDLER_H_
