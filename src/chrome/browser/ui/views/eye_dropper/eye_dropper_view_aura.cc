// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/eye_dropper/eye_dropper_view.h"

#include "build/build_config.h"
#include "ui/aura/client/cursor_client.h"
#include "ui/aura/window.h"

class EyeDropperView::PreEventDispatchHandler::KeyboardHandler
    : public ui::EventHandler {
 public:
  KeyboardHandler(EyeDropperView* view, gfx::NativeView parent);
  KeyboardHandler(const KeyboardHandler&) = delete;
  KeyboardHandler& operator=(const KeyboardHandler&) = delete;
  ~KeyboardHandler() override;

 private:
  void OnKeyEvent(ui::KeyEvent* event) override;

  EyeDropperView* view_;
  gfx::NativeView parent_;
};

EyeDropperView::PreEventDispatchHandler::KeyboardHandler::KeyboardHandler(
    EyeDropperView* view,
    gfx::NativeView parent)
    : view_(view), parent_(parent) {
  // Because the eye dropper is not focused in order to not dismiss the color
  // popup, we need to listen for key events on the parent window that has
  // focus.
  parent_->AddPreTargetHandler(this, ui::EventTarget::Priority::kSystem);
}

EyeDropperView::PreEventDispatchHandler::KeyboardHandler::~KeyboardHandler() {
  parent_->RemovePreTargetHandler(this);
}

void EyeDropperView::PreEventDispatchHandler::KeyboardHandler::OnKeyEvent(
    ui::KeyEvent* event) {
  if (event->type() == ui::ET_KEY_PRESSED &&
      event->key_code() == ui::VKEY_ESCAPE) {
    // Ensure that the color selection is canceled when ESC key is pressed.
    view_->OnColorSelectionCanceled();
    event->StopPropagation();
  }
}

EyeDropperView::PreEventDispatchHandler::PreEventDispatchHandler(
    EyeDropperView* view,
    gfx::NativeView parent)
    : view_(view),
      keyboard_handler_(std::make_unique<KeyboardHandler>(view, parent)) {
  // Ensure that this handler is called before color popup handler by using
  // a higher priority.
  view->GetWidget()->GetNativeWindow()->AddPreTargetHandler(
      this, ui::EventTarget::Priority::kSystem);
}

EyeDropperView::PreEventDispatchHandler::~PreEventDispatchHandler() {
  view_->GetWidget()->GetNativeWindow()->RemovePreTargetHandler(this);
}

void EyeDropperView::PreEventDispatchHandler::OnMouseEvent(
    ui::MouseEvent* event) {
  if (event->type() == ui::ET_MOUSE_PRESSED) {
    // Avoid closing the color popup when the eye dropper is clicked.
    event->StopPropagation();
  }
  if (event->type() == ui::ET_MOUSE_RELEASED) {
    view_->OnColorSelected();
    event->StopPropagation();
  }
}

void EyeDropperView::MoveViewToFront() {
  // The view is already topmost when Aura is used.
}

void EyeDropperView::CaptureInputIfNeeded() {
#if defined(OS_LINUX)
  // The eye dropper needs to capture input since it is not activated
  // in order to avoid dismissing the color picker.
  GetWidget()->GetNativeWindow()->SetCapture();
#endif
}

void EyeDropperView::HideCursor() {
  auto* cursor_client = aura::client::GetCursorClient(
      GetWidget()->GetNativeWindow()->GetRootWindow());
  cursor_client->HideCursor();
  cursor_client->LockCursor();
}

void EyeDropperView::ShowCursor() {
  aura::client::GetCursorClient(GetWidget()->GetNativeWindow()->GetRootWindow())
      ->UnlockCursor();
}

gfx::Size EyeDropperView::GetSize() const {
  return gfx::Size(100, 100);
}

float EyeDropperView::GetDiameter() const {
  return 90;
}

std::unique_ptr<content::EyeDropper> ShowEyeDropper(
    content::RenderFrameHost* frame,
    content::EyeDropperListener* listener) {
  return std::make_unique<EyeDropperView>(frame, listener);
}
