// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_HOST_GTK_SHELL1_H_
#define UI_OZONE_PLATFORM_WAYLAND_HOST_GTK_SHELL1_H_

#include <memory>

#include "ui/ozone/platform/wayland/common/wayland_object.h"

namespace ui {

class GtkSurface1;

class GtkShell1 : public wl::GlobalObjectRegistrar<GtkShell1> {
 public:
  static void Register(WaylandConnection* connection);
  static void Instantiate(WaylandConnection* connection,
                          wl_registry* registry,
                          uint32_t name,
                          uint32_t version);

  explicit GtkShell1(gtk_shell1* shell1);
  GtkShell1(const GtkShell1&) = delete;
  GtkShell1& operator=(const GtkShell1&) = delete;
  ~GtkShell1();

  std::unique_ptr<GtkSurface1> GetGtkSurface1(
      wl_surface* top_level_window_surface);

 private:
  wl::Object<gtk_shell1> shell1_;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_HOST_GTK_SHELL1_H_
