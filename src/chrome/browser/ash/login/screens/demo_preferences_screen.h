// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_LOGIN_SCREENS_DEMO_PREFERENCES_SCREEN_H_
#define CHROME_BROWSER_ASH_LOGIN_SCREENS_DEMO_PREFERENCES_SCREEN_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/scoped_observation.h"
#include "chrome/browser/ash/login/screens/base_screen.h"
// TODO(https://crbug.com/1164001): move to forward declaration.
#include "chrome/browser/ui/webui/chromeos/login/demo_preferences_screen_handler.h"
#include "ui/base/ime/chromeos/input_method_manager.h"

namespace ash {

// Controls demo mode preferences. The screen can be shown during OOBE. It
// allows user to choose preferences for retail demo mode.
class DemoPreferencesScreen
    : public BaseScreen,
      public input_method::InputMethodManager::Observer {
 public:
  enum class Result { COMPLETED, CANCELED };

  static std::string GetResultString(Result result);

  using ScreenExitCallback = base::RepeatingCallback<void(Result result)>;
  DemoPreferencesScreen(DemoPreferencesScreenView* view,
                        const ScreenExitCallback& exit_callback);
  ~DemoPreferencesScreen() override;

  void SetLocale(const std::string& locale);
  void SetInputMethod(const std::string& input_method);
  void SetDemoModeCountry(const std::string& country_id);

  // Called when view is being destroyed. If Screen is destroyed earlier
  // then it has to call Bind(nullptr).
  void OnViewDestroyed(DemoPreferencesScreenView* view);

 protected:
  // BaseScreen:
  void ShowImpl() override;
  void HideImpl() override;
  void OnUserAction(const std::string& action_id) override;

  ScreenExitCallback* exit_callback() { return &exit_callback_; }

 private:
  // InputMethodManager::Observer:
  void InputMethodChanged(input_method::InputMethodManager* manager,
                          Profile* profile,
                          bool show_message) override;

  // Passes current input method to the context, so it can be shown in the UI.
  void UpdateInputMethod(input_method::InputMethodManager* input_manager);

  // Initial locale that was set when the screen was shown. It will be restored
  // if user presses back button.
  std::string initial_locale_;

  // Initial input method that was set when the screen was shown. It will be
  // restored if user presses back button.
  std::string initial_input_method_;

  base::ScopedObservation<input_method::InputMethodManager,
                          input_method::InputMethodManager::Observer>
      input_manager_observation_{this};

  DemoPreferencesScreenView* view_;
  ScreenExitCallback exit_callback_;

  DISALLOW_COPY_AND_ASSIGN(DemoPreferencesScreen);
};

}  // namespace ash

// TODO(https://crbug.com/1164001): remove after the //chrome/browser/chromeos
// source migration is finished.
namespace chromeos {
using ::ash::DemoPreferencesScreen;
}

// TODO(https://crbug.com/1164001): remove after the //chrome/browser/chromeos
// source migration is finished.
namespace ash {
using ::chromeos::DemoPreferencesScreen;
}

#endif  // CHROME_BROWSER_ASH_LOGIN_SCREENS_DEMO_PREFERENCES_SCREEN_H_
