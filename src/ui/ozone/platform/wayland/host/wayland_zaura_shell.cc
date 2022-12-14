// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/host/wayland_zaura_shell.h"

#include <cstring>

#include <components/exo/wayland/protocol/aura-shell-client-protocol.h>

#include "base/check.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/ozone/platform/wayland/host/wayland_connection.h"
#include "ui/ozone/platform/wayland/host/wayland_output_manager.h"
#include "ui/ozone/platform/wayland/host/wayland_screen.h"

namespace ui {

namespace {
constexpr uint32_t kMaxAuraShellVersion = 22;
}

// static
void WaylandZAuraShell::Register(WaylandConnection* connection) {
  connection->RegisterGlobalObjectFactory("zaura_shell",
                                          &WaylandZAuraShell::Instantiate);
}

// static
void WaylandZAuraShell::Instantiate(WaylandConnection* connection,
                                    wl_registry* registry,
                                    uint32_t name,
                                    uint32_t version) {
  if (connection->zaura_shell_)
    return;

  auto zaura_shell = wl::Bind<struct zaura_shell>(
      registry, name, std::min(version, kMaxAuraShellVersion));
  if (!zaura_shell) {
    LOG(ERROR) << "Failed to bind zaura_shell";
    return;
  }
  connection->zaura_shell_ =
      std::make_unique<WaylandZAuraShell>(zaura_shell.release(), connection);
  ReportShellUMA(UMALinuxWaylandShell::kZauraShell);
}

WaylandZAuraShell::WaylandZAuraShell(zaura_shell* aura_shell,
                                     WaylandConnection* connection)
    : obj_(aura_shell), connection_(connection) {
  DCHECK(obj_);
  DCHECK(connection_);

  static constexpr zaura_shell_listener zaura_shell_listener = {
      &OnLayoutMode,
      &OnBugFix,
      &OnDesksChanged,
      &OnDeskActivationChanged,
  };
  zaura_shell_add_listener(obj_.get(), &zaura_shell_listener, this);
}

WaylandZAuraShell::~WaylandZAuraShell() = default;

bool WaylandZAuraShell::HasBugFix(uint32_t id) {
  return bug_fix_ids_.find(id) != bug_fix_ids_.end();
}

std::string WaylandZAuraShell::GetDeskName(int index) const {
  if (static_cast<size_t>(index) >= desks_.size())
    return std::string();
  return desks_[index];
}

int WaylandZAuraShell::GetNumberOfDesks() {
  return desks_.size();
}

int WaylandZAuraShell::GetActiveDeskIndex() const {
  return active_desk_index_;
}

// static
void WaylandZAuraShell::OnLayoutMode(void* data,
                                     struct zaura_shell* zaura_shell,
                                     uint32_t layout_mode) {
  auto* self = static_cast<WaylandZAuraShell*>(data);
  auto* screen = self->connection_->wayland_output_manager()->wayland_screen();
  // |screen| is null in some unit test suites.
  if (!screen)
    return;

  switch (layout_mode) {
    case ZAURA_SHELL_LAYOUT_MODE_WINDOWED:
      screen->OnTabletStateChanged(display::TabletState::kInClamshellMode);
      return;
    case ZAURA_SHELL_LAYOUT_MODE_TABLET:
      screen->OnTabletStateChanged(display::TabletState::kInTabletMode);
      return;
  }
}

// static
void WaylandZAuraShell::OnBugFix(void* data,
                                 struct zaura_shell* zaura_shell,
                                 uint32_t id) {
  auto* self = static_cast<WaylandZAuraShell*>(data);
  self->bug_fix_ids_.insert(id);
}

// static
void WaylandZAuraShell::OnDesksChanged(void* data,
                                       struct zaura_shell* zaura_shell,
                                       struct wl_array* states) {
  auto* self = static_cast<WaylandZAuraShell*>(data);
  char* desk_name = reinterpret_cast<char*>(states->data);
  self->desks_.clear();
  while (desk_name < reinterpret_cast<char*>(states->data) + states->size) {
    std::string str(desk_name, strlen(desk_name));
    self->desks_.push_back(str);
    desk_name += strlen(desk_name) + 1;
  }
}

// static
void WaylandZAuraShell::OnDeskActivationChanged(void* data,
                                                struct zaura_shell* zaura_shell,
                                                int active_desk_index) {
  auto* self = static_cast<WaylandZAuraShell*>(data);
  self->active_desk_index_ = active_desk_index;
}

}  // namespace ui
