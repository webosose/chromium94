// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/host/zwp_text_input_wrapper_v1.h"

#include <string>
#include <utility>

#include "ui/gfx/range/range.h"
#include "ui/ozone/platform/wayland/host/wayland_connection.h"
#include "ui/ozone/platform/wayland/host/wayland_window.h"

#if defined(USE_NEVA_APPRUNTIME)
namespace {

void OnInputPanelVisibilityChanged(ui::WaylandConnection* conn, bool state) {
  ui::WaylandWindow* window =
      conn->wayland_window_manager()->GetCurrentKeyboardFocusedWindow();
  if (!window)
    return;
  window->OnInputPanelVisibilityChanged(state);
}

}  // namespace
#endif  // defined(USE_NEVA_APPRUNTIME)

namespace ui {

ZWPTextInputWrapperV1::ZWPTextInputWrapperV1(
    WaylandConnection* connection,
    ZWPTextInputWrapperClient* client,
    zwp_text_input_manager_v1* text_input_manager)
    : connection_(connection), client_(client) {
  static constexpr zwp_text_input_v1_listener text_input_listener = {
      &OnEnter,                  // text_input_enter,
      &OnLeave,                  // text_input_leave,
      &OnModifiersMap,           // text_input_modifiers_map,
      &OnInputPanelState,        // text_input_input_panel_state,
      &OnPreeditString,          // text_input_preedit_string,
      &OnPreeditStyling,         // text_input_preedit_styling,
      &OnPreeditCursor,          // text_input_preedit_cursor,
      &OnCommitString,           // text_input_commit_string,
      &OnCursorPosition,         // text_input_cursor_position,
      &OnDeleteSurroundingText,  // text_input_delete_surrounding_text,
      &OnKeysym,                 // text_input_keysym,
      &OnLanguage,               // text_input_language,
      &OnTextDirection,          // text_input_text_direction
  };

  auto* text_input =
      zwp_text_input_manager_v1_create_text_input(text_input_manager);
  obj_ = wl::Object<zwp_text_input_v1>(text_input);

  zwp_text_input_v1_add_listener(text_input, &text_input_listener, this);
}

ZWPTextInputWrapperV1::~ZWPTextInputWrapperV1() = default;

void ZWPTextInputWrapperV1::Reset() {
  ResetInputEventState();
  zwp_text_input_v1_reset(obj_.get());
}

void ZWPTextInputWrapperV1::Activate(WaylandWindow* window) {
  zwp_text_input_v1_activate(obj_.get(), connection_->seat(),
                             window->root_surface()->surface());
}

void ZWPTextInputWrapperV1::Deactivate() {
  zwp_text_input_v1_deactivate(obj_.get(), connection_->seat());
}

void ZWPTextInputWrapperV1::ShowInputPanel() {
  zwp_text_input_v1_show_input_panel(obj_.get());
}

void ZWPTextInputWrapperV1::HideInputPanel() {
  zwp_text_input_v1_hide_input_panel(obj_.get());
}

void ZWPTextInputWrapperV1::SetCursorRect(const gfx::Rect& rect) {
  zwp_text_input_v1_set_cursor_rectangle(obj_.get(), rect.x(), rect.y(),
                                         rect.width(), rect.height());
}

void ZWPTextInputWrapperV1::SetSurroundingText(
    const std::string& text,
    const gfx::Range& selection_range) {
  zwp_text_input_v1_set_surrounding_text(
      obj_.get(), text.c_str(), selection_range.start(), selection_range.end());
}

void ZWPTextInputWrapperV1::ResetInputEventState() {
  spans_.clear();
  preedit_cursor_ = -1;
}

// static
void ZWPTextInputWrapperV1::OnEnter(void* data,
                                    struct zwp_text_input_v1* text_input,
                                    struct wl_surface* surface) {
#if defined(USE_NEVA_APPRUNTIME)
  ZWPTextInputWrapperV1* wti = static_cast<ZWPTextInputWrapperV1*>(data);
  OnInputPanelVisibilityChanged(wti->connection_, true);
#else   // defined(USE_NEVA_APPRUNTIME)
  NOTIMPLEMENTED_LOG_ONCE();
#endif  // defined(USE_NEVA_APPRUNTIME)
}

// static
void ZWPTextInputWrapperV1::OnLeave(void* data,
                                    struct zwp_text_input_v1* text_input) {
#if defined(USE_NEVA_APPRUNTIME)
  ZWPTextInputWrapperV1* wti = static_cast<ZWPTextInputWrapperV1*>(data);
  OnInputPanelVisibilityChanged(wti->connection_, false);
#else   // defined(USE_NEVA_APPRUNTIME)
  NOTIMPLEMENTED_LOG_ONCE();
#endif  // defined(USE_NEVA_APPRUNTIME)
}

// static
void ZWPTextInputWrapperV1::OnModifiersMap(void* data,
                                           struct zwp_text_input_v1* text_input,
                                           struct wl_array* map) {
  NOTIMPLEMENTED_LOG_ONCE();
}

// static
void ZWPTextInputWrapperV1::OnInputPanelState(
    void* data,
    struct zwp_text_input_v1* text_input,
    uint32_t state) {
  NOTIMPLEMENTED_LOG_ONCE();
}

// static
void ZWPTextInputWrapperV1::OnPreeditString(
    void* data,
    struct zwp_text_input_v1* text_input,
    uint32_t serial,
    const char* text,
    const char* commit) {
  auto* self = static_cast<ZWPTextInputWrapperV1*>(data);
  auto spans = std::move(self->spans_);
  int32_t preedit_cursor = self->preedit_cursor_;
  self->ResetInputEventState();
  self->client_->OnPreeditString(text, spans, preedit_cursor);
}

// static
void ZWPTextInputWrapperV1::OnPreeditStyling(
    void* data,
    struct zwp_text_input_v1* text_input,
    uint32_t index,
    uint32_t length,
    uint32_t style) {
  auto* self = static_cast<ZWPTextInputWrapperV1*>(data);
  self->spans_.push_back(
      ZWPTextInputWrapperClient::SpanStyle{index, length, style});
}

// static
void ZWPTextInputWrapperV1::OnPreeditCursor(
    void* data,
    struct zwp_text_input_v1* text_input,
    int32_t index) {
  auto* self = static_cast<ZWPTextInputWrapperV1*>(data);
  self->preedit_cursor_ = index;
}

// static
void ZWPTextInputWrapperV1::OnCommitString(void* data,
                                           struct zwp_text_input_v1* text_input,
                                           uint32_t serial,
                                           const char* text) {
  auto* self = static_cast<ZWPTextInputWrapperV1*>(data);
  self->ResetInputEventState();
  self->client_->OnCommitString(text);
}

// static
void ZWPTextInputWrapperV1::OnCursorPosition(
    void* data,
    struct zwp_text_input_v1* text_input,
    int32_t index,
    int32_t anchor) {
  NOTIMPLEMENTED_LOG_ONCE();
}

// static
void ZWPTextInputWrapperV1::OnDeleteSurroundingText(
    void* data,
    struct zwp_text_input_v1* text_input,
    int32_t index,
    uint32_t length) {
  auto* self = static_cast<ZWPTextInputWrapperV1*>(data);
  self->client_->OnDeleteSurroundingText(index, length);
}

// static
void ZWPTextInputWrapperV1::OnKeysym(void* data,
                                     struct zwp_text_input_v1* text_input,
                                     uint32_t serial,
                                     uint32_t time,
                                     uint32_t key,
                                     uint32_t state,
                                     uint32_t modifiers) {
  auto* self = static_cast<ZWPTextInputWrapperV1*>(data);
  self->client_->OnKeysym(key, state, modifiers);
}

// static
void ZWPTextInputWrapperV1::OnLanguage(void* data,
                                       struct zwp_text_input_v1* text_input,
                                       uint32_t serial,
                                       const char* language) {
  NOTIMPLEMENTED_LOG_ONCE();
}

// static
void ZWPTextInputWrapperV1::OnTextDirection(
    void* data,
    struct zwp_text_input_v1* text_input,
    uint32_t serial,
    uint32_t direction) {
  NOTIMPLEMENTED_LOG_ONCE();
}

}  // namespace ui
