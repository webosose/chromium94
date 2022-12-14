// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_ZWP_RELATIVE_POINTER_MANAGER_H_
#define UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_ZWP_RELATIVE_POINTER_MANAGER_H_

#include "ui/ozone/platform/wayland/common/wayland_object.h"

namespace gfx {
class Vector2dF;
}  // namespace gfx

namespace ui {

class WaylandConnection;

// Wraps the zwp_relative_pointer_manager_v1 object.
class WaylandZwpRelativePointerManager
    : public wl::GlobalObjectRegistrar<WaylandZwpRelativePointerManager> {
 public:
  static void Register(WaylandConnection* connection);
  static void Instantiate(WaylandConnection* connection,
                          wl_registry* registry,
                          uint32_t name,
                          uint32_t version);

  class Delegate;

  WaylandZwpRelativePointerManager(
      zwp_relative_pointer_manager_v1* relative_pointer_manager,
      WaylandConnection* connection);

  WaylandZwpRelativePointerManager(const WaylandZwpRelativePointerManager&) =
      delete;
  WaylandZwpRelativePointerManager& operator=(
      const WaylandZwpRelativePointerManager&) = delete;
  ~WaylandZwpRelativePointerManager();

  void EnableRelativePointer();
  void DisableRelativePointer();

 private:
  // zwp_relative_pointer_v1_listener
  static void OnHandleMotion(void* data,
                             struct zwp_relative_pointer_v1* pointer,
                             uint32_t utime_hi,
                             uint32_t utime_lo,
                             wl_fixed_t dx,
                             wl_fixed_t dy,
                             wl_fixed_t dx_unaccel,
                             wl_fixed_t dy_unaccel);

  wl::Object<zwp_relative_pointer_manager_v1> obj_;
  wl::Object<zwp_relative_pointer_v1> relative_pointer_;
  WaylandConnection* const connection_;
  Delegate* const delegate_;
};

class WaylandZwpRelativePointerManager::Delegate {
 public:
  virtual void SetRelativePointerMotionEnabled(bool enabled) = 0;
  virtual void OnRelativePointerMotion(const gfx::Vector2dF& delta) = 0;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_ZWP_RELATIVE_POINTER_MANAGER_H_
