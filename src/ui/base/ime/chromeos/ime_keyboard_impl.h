// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_CHROMEOS_IME_KEYBOARD_IMPL_H_
#define UI_BASE_IME_CHROMEOS_IME_KEYBOARD_IMPL_H_

#include "base/component_export.h"
#include "base/macros.h"
#include "ui/base/ime/chromeos/ime_keyboard.h"

namespace ui {
class InputController;
}  // namespace ui

namespace chromeos {
namespace input_method {

// Version of ImeKeyboard used when chrome is run on device.
class COMPONENT_EXPORT(UI_BASE_IME_CHROMEOS) ImeKeyboardImpl
    : public ImeKeyboard {
 public:
  ImeKeyboardImpl(ui::InputController* input_controller);
  ~ImeKeyboardImpl() override;

  // ImeKeyboard:
  bool SetCurrentKeyboardLayoutByName(const std::string& layout_name) override;
  bool SetAutoRepeatRate(const AutoRepeatRate& rate) override;
  bool SetAutoRepeatEnabled(bool enabled) override;
  bool GetAutoRepeatEnabled() override;
  bool ReapplyCurrentKeyboardLayout() override;
  void ReapplyCurrentModifierLockStatus() override;
  void DisableNumLock() override;
  void SetCapsLockEnabled(bool enable_caps_lock) override;
  bool CapsLockIsEnabled() override;

 private:
  ui::InputController* const input_controller_;

  DISALLOW_COPY_AND_ASSIGN(ImeKeyboardImpl);
};

}  // namespace input_method
}  // namespace chromeos

// TODO(https://crbug.com/1164001): remove when moved to ash.
namespace ash {
namespace input_method {
using ::chromeos::input_method::ImeKeyboardImpl;
}  // namespace input_method
}  // namespace ash

#endif  // UI_BASE_IME_CHROMEOS_IME_KEYBOARD_IMPL_H_
