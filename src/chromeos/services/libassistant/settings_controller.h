// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_LIBASSISTANT_SETTINGS_CONTROLLER_H_
#define CHROMEOS_SERVICES_LIBASSISTANT_SETTINGS_CONTROLLER_H_

#include <string>

#include "third_party/abseil-cpp/absl/types/optional.h"

#include "chromeos/services/libassistant/abortable_task_list.h"
#include "chromeos/services/libassistant/assistant_client_observer.h"
#include "chromeos/services/libassistant/public/mojom/settings_controller.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace assistant_client {
class AssistantManager;
class AssistantManagerInternal;
}  // namespace assistant_client

namespace chromeos {
namespace libassistant {

class SettingsController : public AssistantClientObserver,
                           public mojom::SettingsController {
 public:
  SettingsController();
  SettingsController(const SettingsController&) = delete;
  SettingsController& operator=(const SettingsController&) = delete;
  ~SettingsController() override;

  void Bind(mojo::PendingReceiver<mojom::SettingsController> receiver);

  // mojom::SettingsController implementation:
  void SetAuthenticationTokens(
      std::vector<mojom::AuthenticationTokenPtr> tokens) override;
  void SetListeningEnabled(bool value) override;
  void SetLocale(const std::string& value) override;
  void SetSpokenFeedbackEnabled(bool value) override;
  void SetHotwordEnabled(bool value) override;
  void GetSettings(const std::string& selector,
                   bool include_header,
                   GetSettingsCallback callback) override;
  void UpdateSettings(const std::string& settings,
                      UpdateSettingsCallback callback) override;

  // AssistantClientObserver:
  void OnAssistantClientCreated(AssistantClient* assistant_client) override;
  void OnAssistantClientStarted(AssistantClient* assistant_client) override;
  void OnDestroyingAssistantClient(AssistantClient* assistant_client) override;

 private:
  class DeviceSettingsUpdater;

  // The settings are being passed in to clearly document when Libassistant
  // must be updated.
  void UpdateListeningEnabled(absl::optional<bool> listening_enabled);
  void UpdateAuthenticationTokens(
      const absl::optional<std::vector<mojom::AuthenticationTokenPtr>>& tokens);
  void UpdateInternalOptions(const absl::optional<std::string>& locale,
                             absl::optional<bool> spoken_feedback_enabled);
  void UpdateDeviceSettings(const absl::optional<std::string>& locale,
                            absl::optional<bool> hotword_enabled);

  // Instantiated when Libassistant is started and destroyed when Libassistant
  // is stopped.
  // Used to update the device settings.
  std::unique_ptr<DeviceSettingsUpdater> device_settings_updater_;
  // Contains all pending callbacks for get/update setting requests.
  AbortableTaskList pending_response_waiters_;

  // Set in |OnAssistantClientCreated| and unset in
  // |OnDestroyingAssistantClient|.
  assistant_client::AssistantManagerInternal* assistant_manager_internal_ =
      nullptr;
  assistant_client::AssistantManager* assistant_manager_ = nullptr;

  absl::optional<bool> hotword_enabled_;
  absl::optional<bool> spoken_feedback_enabled_;
  absl::optional<bool> listening_enabled_;
  absl::optional<std::string> locale_;
  absl::optional<std::vector<mojom::AuthenticationTokenPtr>>
      authentication_tokens_;

  mojo::Receiver<mojom::SettingsController> receiver_{this};
};

}  // namespace libassistant
}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_LIBASSISTANT_SETTINGS_CONTROLLER_H_
