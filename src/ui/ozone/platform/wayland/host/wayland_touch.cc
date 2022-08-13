// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/host/wayland_touch.h"

#include "base/time/time.h"
#include "ui/events/types/event_type.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/ozone/platform/wayland/common/wayland_util.h"
#include "ui/ozone/platform/wayland/host/wayland_connection.h"
#include "ui/ozone/platform/wayland/host/wayland_window.h"

namespace ui {

WaylandTouch::WaylandTouch(wl_touch* touch,
                           WaylandConnection* connection,
                           Delegate* delegate)
    : obj_(touch), connection_(connection), delegate_(delegate) {
  static constexpr wl_touch_listener listener = {
      &Down, &Up, &Motion, &Frame, &Cancel,
  };

  wl_touch_add_listener(obj_.get(), &listener, this);
}

WaylandTouch::~WaylandTouch() {
#if defined(OS_WEBOS)
  int device_id = obj_.id();
#endif  // defined(OS_WEBOS)
  delegate_->OnTouchCancelEvent(
#if defined(OS_WEBOS)
      device_id
#endif  // defined(OS_WEBOS)
  );
#if defined(OS_WEBOS)
  if (auto* window_manager = connection_->wayland_window_manager())
    window_manager->UngrabTouchEvents(
        device_id, window_manager->touch_events_grabber(device_id));
#endif  // defined(OS_WEBOS)
}

void WaylandTouch::Down(void* data,
                        wl_touch* obj,
                        uint32_t serial,
                        uint32_t time,
                        struct wl_surface* surface,
                        int32_t id,
                        wl_fixed_t x,
                        wl_fixed_t y) {
  if (!surface)
    return;

  WaylandTouch* touch = static_cast<WaylandTouch*>(data);
  DCHECK(touch);
  touch->connection_->set_serial(serial, ET_TOUCH_PRESSED);

  WaylandWindow* window = wl::RootWindowFromWlSurface(surface);
#if defined(OS_WEBOS)
  DCHECK(window);
  int device_id = touch->obj_.id();

  window->set_touch_device_id(device_id);
  if (auto* window_manager = touch->connection_->wayland_window_manager())
    window_manager->GrabTouchEvents(device_id, window);
#endif  // defined(OS_WEBOS)
  gfx::PointF location(wl_fixed_to_double(x), wl_fixed_to_double(y));
  base::TimeTicks timestamp =
      base::TimeTicks() + base::TimeDelta::FromMilliseconds(time);
  touch->delegate_->OnTouchPressEvent(window, location, timestamp, id
#if defined(OS_WEBOS)
                                      ,
                                      device_id
#endif  // defined(OS_WEBOS)
  );
}

void WaylandTouch::Up(void* data,
                      wl_touch* obj,
                      uint32_t serial,
                      uint32_t time,
                      int32_t id) {
  WaylandTouch* touch = static_cast<WaylandTouch*>(data);
  DCHECK(touch);

  touch->connection_->set_serial(serial, ET_TOUCH_RELEASED);

  base::TimeTicks timestamp =
      base::TimeTicks() + base::TimeDelta::FromMilliseconds(time);
  touch->delegate_->OnTouchReleaseEvent(timestamp, id
#if defined(OS_WEBOS)
                                        ,
                                        touch->obj_.id()
#endif  // defined(OS_WEBOS)
  );

  // Do not store the |serial| on UP events. Otherwise, Ozone can't create popup
  // windows, which (according to the spec) can only be created on reaction to
  // button/touch down serials.
}

void WaylandTouch::Motion(void* data,
                          wl_touch* obj,
                          uint32_t time,
                          int32_t id,
                          wl_fixed_t x,
                          wl_fixed_t y) {
  WaylandTouch* touch = static_cast<WaylandTouch*>(data);
  DCHECK(touch);

  gfx::PointF location(wl_fixed_to_double(x), wl_fixed_to_double(y));
  base::TimeTicks timestamp =
      base::TimeTicks() + base::TimeDelta::FromMilliseconds(time);
  touch->delegate_->OnTouchMotionEvent(location, timestamp, id
#if defined(OS_WEBOS)
                                       ,
                                       touch->obj_.id()
#endif  // defined(OS_WEBOS)
  );
}

void WaylandTouch::Cancel(void* data, wl_touch* obj) {
  WaylandTouch* touch = static_cast<WaylandTouch*>(data);
  DCHECK(touch);
#if defined(OS_WEBOS)
  int device_id = touch->obj_.id();
#endif  // defined(OS_WEBOS)
  touch->delegate_->OnTouchCancelEvent(
#if defined(OS_WEBOS)
      device_id
#endif  // defined(OS_WEBOS)
  );
#if defined(OS_WEBOS)
  if (auto* window_manager = touch->connection_->wayland_window_manager())
    window_manager->UngrabTouchEvents(
        device_id, window_manager->touch_events_grabber(device_id));
#endif  // defined(OS_WEBOS)
}

void WaylandTouch::Frame(void* data, wl_touch* obj) {}

}  // namespace ui
