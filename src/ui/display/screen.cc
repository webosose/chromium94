// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/screen.h"

#include <utility>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/notreached.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "ui/display/display.h"
#include "ui/display/types/display_constants.h"
#include "ui/gfx/geometry/rect.h"

namespace display {

namespace {

Screen* g_screen;

}  // namespace

Screen::Screen() : display_id_for_new_windows_(kInvalidDisplayId) {}

Screen::~Screen() = default;

// static
Screen* Screen::GetScreen() {
#if defined(OS_APPLE)
  // TODO(scottmg): https://crbug.com/558054
  if (!g_screen)
    g_screen = CreateNativeScreen();
#endif
  return g_screen;
}

// static
Screen* Screen::SetScreenInstance(Screen* instance) {
  return std::exchange(g_screen, instance);
}

void Screen::SetCursorScreenPointForTesting(const gfx::Point& point) {
  NOTIMPLEMENTED_LOG_ONCE();
}

Display Screen::GetDisplayNearestView(gfx::NativeView view) const {
  return GetDisplayNearestWindow(GetWindowForView(view));
}

display::Display Screen::GetDisplayForNewWindows() const {
  display::Display display;
  // Scoped value can override if it is set.
  if (scoped_display_id_for_new_windows_ != kInvalidDisplayId &&
      GetDisplayWithDisplayId(scoped_display_id_for_new_windows_, &display)) {
    return display;
  }

  if (GetDisplayWithDisplayId(display_id_for_new_windows_, &display))
    return display;

  // Fallback to primary display.
  return GetPrimaryDisplay();
}

void Screen::SetDisplayForNewWindows(int64_t display_id) {
  // GetDisplayForNewWindows() handles invalid display ids.
  display_id_for_new_windows_ = display_id;
}

DisplayList Screen::GetDisplayListNearestViewWithFallbacks(
    gfx::NativeView view) const {
  return GetDisplayListNearestDisplayWithFallbacks(GetDisplayNearestView(view));
}

DisplayList Screen::GetDisplayListNearestWindowWithFallbacks(
    gfx::NativeWindow window) const {
  return GetDisplayListNearestDisplayWithFallbacks(
      GetDisplayNearestWindow(window));
}

void Screen::SetScreenSaverSuspended(bool suspend) {
  NOTIMPLEMENTED_LOG_ONCE();
}

bool Screen::IsScreenSaverActive() const {
  NOTIMPLEMENTED_LOG_ONCE();
  return false;
}

base::TimeDelta Screen::CalculateIdleTime() const {
  NOTIMPLEMENTED_LOG_ONCE();
  return base::TimeDelta::FromSeconds(0);
}

gfx::Rect Screen::ScreenToDIPRectInWindow(gfx::NativeWindow window,
                                          const gfx::Rect& screen_rect) const {
  float scale = GetDisplayNearestWindow(window).device_scale_factor();
  return ScaleToEnclosingRect(screen_rect, 1.0f / scale);
}

gfx::Rect Screen::DIPToScreenRectInWindow(gfx::NativeWindow window,
                                          const gfx::Rect& dip_rect) const {
  float scale = GetDisplayNearestWindow(window).device_scale_factor();
  return ScaleToEnclosingRect(dip_rect, scale);
}

bool Screen::GetDisplayWithDisplayId(int64_t display_id,
                                     Display* display) const {
  for (const Display& display_in_list : GetAllDisplays()) {
    if (display_in_list.id() == display_id) {
      *display = display_in_list;
      return true;
    }
  }
  return false;
}

void Screen::SetPanelRotationForTesting(int64_t display_id,
                                        Display::Rotation rotation) {
  // Not implemented.
  DCHECK(false);
}

std::string Screen::GetCurrentWorkspace() {
  NOTIMPLEMENTED_LOG_ONCE();
  return {};
}

std::vector<base::Value> Screen::GetGpuExtraInfo(
    const gfx::GpuExtraInfo& gpu_extra_info) {
  return std::vector<base::Value>();
}

void Screen::SetScopedDisplayForNewWindows(int64_t display_id) {
  if (display_id == scoped_display_id_for_new_windows_)
    return;
  // Only allow set and clear, not switch.
  DCHECK(display_id == kInvalidDisplayId ^
         scoped_display_id_for_new_windows_ == kInvalidDisplayId)
      << "display_id=" << display_id << ", scoped_display_id_for_new_windows_="
      << scoped_display_id_for_new_windows_;
  scoped_display_id_for_new_windows_ = display_id;
}

DisplayList Screen::GetDisplayListNearestDisplayWithFallbacks(
    Display nearest) const {
  DisplayList display_list;
  const std::vector<Display>& displays = GetAllDisplays();
  Display primary = GetPrimaryDisplay();
  if (displays.empty()) {
    // The nearest display's metrics are of greater value to clients of this
    // function than those of the primary display, so prefer to use that Display
    // object as the fallback, if GetAllDisplays() returned an empty array.
    if (nearest.id() == kInvalidDisplayId && primary.id() != kInvalidDisplayId)
      display_list = DisplayList({primary}, primary.id(), primary.id());
    else
      display_list = DisplayList({nearest}, nearest.id(), nearest.id());
  } else {
    // Use the primary and nearest displays as fallbacks for each other, if the
    // counterpart exists in `displays`. Otherwise, use `display[0]` for both.
    int64_t primary_id = primary.id();
    int64_t nearest_id = nearest.id();
    const bool has_primary = base::Contains(displays, primary_id, &Display::id);
    const bool has_nearest = base::Contains(displays, nearest_id, &Display::id);
    if (!has_primary)
      primary_id = has_nearest ? nearest_id : displays[0].id();
    if (!has_nearest)
      nearest_id = primary_id;
    display_list = DisplayList(displays, primary_id, nearest_id);
  }
  CHECK(display_list.IsValidAndHasPrimaryAndCurrentDisplays());
  return display_list;
}

}  // namespace display
