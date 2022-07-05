// Copyright 2019 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "ui/ozone/platform/wayland/host/wayland_seat.h"

#include "base/logging.h"
#include "ui/events/ozone/layout/keyboard_layout_engine_manager.h"
#include "ui/ozone/platform/wayland/host/wayland_connection.h"
#include "ui/ozone/platform/wayland/host/wayland_cursor.h"
#include "ui/ozone/platform/wayland/host/wayland_cursor_position.h"
#include "ui/ozone/platform/wayland/host/wayland_event_source.h"
#include "ui/ozone/platform/wayland/host/wayland_keyboard.h"
#include "ui/ozone/platform/wayland/host/wayland_pointer.h"
#include "ui/ozone/platform/wayland/host/wayland_touch.h"

#if defined(USE_NEVA_APPRUNTIME)
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "ui/events/event_switches.h"
#endif

namespace ui {

WaylandSeat::WaylandSeat(WaylandConnection* connection,
                         std::uint32_t seat_id,
                         wl_seat* seat)
    : connection_(connection), seat_id_(seat_id), seat_(seat) {
  DCHECK(connection_);

  static const wl_seat_listener seat_listener = {
      &WaylandSeat::Capabilities,
      &WaylandSeat::Name,
  };
  wl_seat_add_listener(seat_.get(), &seat_listener, this);
}

WaylandSeat::~WaylandSeat() = default;

bool WaylandSeat::CreateKeyboard() {
  wl_keyboard* keyboard = wl_seat_get_keyboard(seat_.get());
  if (!keyboard)
    return false;

  auto* layout_engine = KeyboardLayoutEngineManager::GetKeyboardLayoutEngine();
  // Make sure to destroy the old WaylandKeyboard (if it exists) before creating
  // the new one.
  keyboard_.reset();
  keyboard_.reset(new WaylandKeyboard(
      keyboard, connection_->keyboard_extension_v1(), connection_,
      layout_engine, connection_->event_source()));
  return true;
}

void WaylandSeat::UpdateInputDevices(wl_seat* seat,
                                     std::uint32_t capabilities) {
  DCHECK(seat);
  DCHECK(connection_->event_source());
  auto has_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
  auto has_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;
  auto has_touch = capabilities & WL_SEAT_CAPABILITY_TOUCH;

  if (!has_pointer) {
    pointer_.reset();
    cursor_.reset();
    cursor_position_.reset();
  } else if (wl_pointer* pointer = wl_seat_get_pointer(seat)) {
    pointer_ = std::make_unique<WaylandPointer>(pointer, connection_,
                                                connection_->event_source());
    cursor_ = std::make_unique<WaylandCursor>(pointer_.get(), connection_);
    cursor_position_ = std::make_unique<WaylandCursorPosition>();
  } else {
    LOG(ERROR) << "Failed to get wl_pointer from seat";
  }

  if (!has_keyboard) {
    keyboard_.reset();
  } else if (!CreateKeyboard()) {
    LOG(ERROR) << "Failed to create WaylandKeyboard";
  }

  if (!has_touch) {
    touch_.reset();

#if defined(USE_NEVA_APPRUNTIME)
    NotifyTouchscreenRemoved();
#endif
  } else if (wl_touch* touch = wl_seat_get_touch(seat)) {
    touch_ = std::make_unique<WaylandTouch>(touch, connection_,
                                            connection_->event_source());

#if defined(USE_NEVA_APPRUNTIME)
    NotifyTouchscreenAdded();
#endif
  } else {
    LOG(ERROR) << "Failed to get wl_touch from seat";
  }
}

#if defined(USE_NEVA_APPRUNTIME)
void WaylandSeat::NotifyTouchscreenAdded() {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kIgnoreTouchDevices))
    return;

  const int device_id = touch_device_id_seq_.GetNext();
  const std::string device_name = "touch-" + std::to_string(device_id);
  const int default_touch_points = 1;
  const int touch_points = OverrideMaxTouchPoints(default_touch_points);
  touch_device_.clear();
  touch_device_.emplace_back(
      TouchscreenDevice(device_id, InputDeviceType::INPUT_DEVICE_INTERNAL,
                        device_name, gfx::Size(), touch_points));

  DeviceHotplugEventObserver* device_data_manager =
      DeviceDataManager::GetInstance();
  device_data_manager->OnTouchscreenDevicesUpdated(touch_device_);
}

int WaylandSeat::OverrideMaxTouchPoints(int default_value) {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  std::string str =
      command_line->GetSwitchValueASCII(switches::kForceMaxTouchPoints);

  int max_touch_points;
  if (!str.empty() && base::StringToInt(str, &max_touch_points))
    return max_touch_points;

  return default_value;
}

void WaylandSeat::NotifyTouchscreenRemoved() {
  if (touch_device_.empty()) {
    return;
  }

  touch_device_.clear();
  DeviceHotplugEventObserver* device_data_manager =
      DeviceDataManager::GetInstance();
  device_data_manager->OnTouchscreenDevicesUpdated(touch_device_);
}
#endif

// static
void WaylandSeat::Capabilities(void* data,
                               wl_seat* seat,
                               std::uint32_t capabilities) {
  WaylandSeat* wayland_seat = static_cast<WaylandSeat*>(data);

  DCHECK(wayland_seat);
  DCHECK(wayland_seat->connection_);

  wayland_seat->UpdateInputDevices(seat, capabilities);
  wayland_seat->connection_->ScheduleFlush();
}

// static
void WaylandSeat::Name(void* data, wl_seat* seat, const char* name) {}

}  // namespace ui
