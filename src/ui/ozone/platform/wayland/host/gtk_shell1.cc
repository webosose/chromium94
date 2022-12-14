// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/host/gtk_shell1.h"

#include <gtk-shell-client-protocol.h>

#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "ui/ozone/platform/wayland/host/gtk_surface1.h"
#include "ui/ozone/platform/wayland/host/wayland_connection.h"

namespace ui {

namespace {
// gtk_shell1 exposes request_focus() since version 3.  Below that, it is not
// interesting for us, although it provides some shell integration that might be
// useful.
constexpr uint32_t kMinGtkShell1Version = 3;
constexpr uint32_t kMaxGtkShell1Version = 4;
}  // namespace

// static
void GtkShell1::Register(WaylandConnection* connection) {
  connection->RegisterGlobalObjectFactory("gtk_shell1",
                                          &GtkShell1::Instantiate);
}

// static
void GtkShell1::Instantiate(WaylandConnection* connection,
                            wl_registry* registry,
                            uint32_t name,
                            uint32_t version) {
  if (connection->gtk_shell1_ || version < kMinGtkShell1Version)
    return;

  auto gtk_shell1 = wl::Bind<::gtk_shell1>(
      registry, name, std::min(version, kMaxGtkShell1Version));
  if (!gtk_shell1) {
    LOG(ERROR) << "Failed to bind gtk_shell1";
    return;
  }
  connection->gtk_shell1_ = std::make_unique<GtkShell1>(gtk_shell1.release());
  ReportShellUMA(UMALinuxWaylandShell::kGtkShell1);
}

GtkShell1::GtkShell1(gtk_shell1* shell1) : shell1_(shell1) {}

GtkShell1::~GtkShell1() = default;

std::unique_ptr<GtkSurface1> GtkShell1::GetGtkSurface1(
    wl_surface* top_level_window_surface) {
  return std::make_unique<GtkSurface1>(
      gtk_shell1_get_gtk_surface(shell1_.get(), top_level_window_surface));
}

}  // namespace ui
