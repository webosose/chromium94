// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_CHROMEOS_IME_ASSISTIVE_WINDOW_HANDLER_INTERFACE_H_
#define UI_BASE_IME_CHROMEOS_IME_ASSISTIVE_WINDOW_HANDLER_INTERFACE_H_

#include <stdint.h>

#include <string>

#include "base/component_export.h"
#include "ui/gfx/geometry/rect.h"

namespace ash {
namespace input_method {
struct AssistiveWindowProperties;
}  // namespace input_method
}  // namespace ash

namespace ui {
namespace ime {
struct AssistiveWindowButton;
struct SuggestionDetails;
}  // namespace ime
}  // namespace ui

namespace chromeos {

// Contains bounds for windows controlled by handler.
struct Bounds {
  // Position of the cursor.
  gfx::Rect caret;
  // Position of the autocorrect span, empty if not present.
  gfx::Rect autocorrect;
  // Position of the current composition text in screen
  gfx::Rect composition_text;
};

// A interface to handle the assistive windows related method call.
class COMPONENT_EXPORT(UI_BASE_IME_CHROMEOS)
    IMEAssistiveWindowHandlerInterface {
 public:
  virtual ~IMEAssistiveWindowHandlerInterface() = default;

  // Called when showing/hiding assistive window.
  virtual void SetAssistiveWindowProperties(
      const ash::input_method::AssistiveWindowProperties& window) {}

  virtual void ShowSuggestion(const ui::ime::SuggestionDetails& details) {}

  // Highlights or unhighlights a given assistive button based on the given
  // parameters.
  virtual void SetButtonHighlighted(
      const ui::ime::AssistiveWindowButton& button,
      bool highlighted) {}

  virtual void AcceptSuggestion(const std::u16string& suggestion) {}

  virtual void HideSuggestion() {}

  // Called to get the current suggestion text.
  virtual std::u16string GetSuggestionText() const = 0;

  // Called to get length of the confirmed part of suggestion text.
  virtual size_t GetConfirmedLength() const = 0;

  // Called when the application changes its caret bounds.
  virtual void SetBounds(const Bounds& bounds) = 0;

  // Called when the text field's focus state is changed.
  virtual void FocusStateChanged() {}

  // Announces the given message using the system's text-to-speech features
  virtual void Announce(const std::u16string& message) = 0;

 protected:
  IMEAssistiveWindowHandlerInterface() = default;
};

}  // namespace chromeos

// TODO(https://crbug.com/1164001): remove when moved to ash.
namespace ash {
using ::chromeos::Bounds;
using ::chromeos::IMEAssistiveWindowHandlerInterface;
}  // namespace ash

#endif  // UI_BASE_IME_CHROMEOS_IME_ASSISTIVE_WINDOW_HANDLER_INTERFACE_H_
