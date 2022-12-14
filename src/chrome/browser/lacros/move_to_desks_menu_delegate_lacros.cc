// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/lacros/move_to_desks_menu_delegate_lacros.h"

#include "chromeos/strings/grit/chromeos_strings.h"
#include "chromeos/ui/frame/move_to_desks_menu_model.h"
#include "ui/aura/window.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/platform_window/extensions/desk_extension.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_linux.h"
#include "ui/views/widget/widget.h"

namespace {

int MapCommandIdToDeskIndex(int command_id) {
  DCHECK_GE(command_id, chromeos::MoveToDesksMenuModel::MOVE_TO_DESK_1);
  DCHECK_LE(command_id, chromeos::MoveToDesksMenuModel::MOVE_TO_DESK_8);
  return command_id - chromeos::MoveToDesksMenuModel::MOVE_TO_DESK_1;
}

bool IsAssignToAllDesksCommand(int command_id) {
  return command_id ==
         chromeos::MoveToDesksMenuModel::TOGGLE_ASSIGN_TO_ALL_DESKS;
}

}  // namespace

// static
bool MoveToDesksMenuDelegateLacros::ShouldShowMoveToDesksMenu(
    aura::Window* window) {
  return views::DesktopWindowTreeHostLinux::From(window->GetHost())
             ->GetDeskExtension()
             ->GetNumberOfDesks() > 1;
}

MoveToDesksMenuDelegateLacros::MoveToDesksMenuDelegateLacros(
    views::Widget* widget)
    : widget_(widget) {}

bool MoveToDesksMenuDelegateLacros::IsCommandIdChecked(int command_id) const {
  const bool assigned_to_all_desks = widget_->IsVisibleOnAllWorkspaces();
  if (IsAssignToAllDesksCommand(command_id))
    return assigned_to_all_desks;

  return !assigned_to_all_desks &&
         MapCommandIdToDeskIndex(command_id) ==
             views::DesktopWindowTreeHostLinux::From(
                 widget_->GetNativeWindow()->GetHost())
                 ->GetDeskExtension()
                 ->GetActiveDeskIndex();
}

bool MoveToDesksMenuDelegateLacros::IsCommandIdEnabled(int command_id) const {
  if (IsAssignToAllDesksCommand(command_id))
    return true;

  return MapCommandIdToDeskIndex(command_id) <
         views::DesktopWindowTreeHostLinux::From(
             widget_->GetNativeWindow()->GetHost())
             ->GetDeskExtension()
             ->GetNumberOfDesks();
}

bool MoveToDesksMenuDelegateLacros::IsCommandIdVisible(int command_id) const {
  return IsCommandIdEnabled(command_id);
}

bool MoveToDesksMenuDelegateLacros::IsItemForCommandIdDynamic(
    int command_id) const {
  // The potential command_id is from MoveToDesksMenuModel::MOVE_TO_DESK_1
  // to MoveToDesksMenuModel::MOVE_TO_DESK_8,
  // MoveToDesksMenuModel::TOGGLE_ASSIGN_TO_ALL_DESKS.
  // For Move window to desk menu, all the menu items are dynamic.
  // Therefore, checking whether command_id is within the range from
  // MOVE_TO_DESK_1 to TOGGLE_ASSIGN_TO_ALL_DESKS.
  return chromeos::MoveToDesksMenuModel::MOVE_TO_DESK_1 <= command_id &&
         command_id <=
             chromeos::MoveToDesksMenuModel::TOGGLE_ASSIGN_TO_ALL_DESKS;
}

std::u16string MoveToDesksMenuDelegateLacros::GetLabelForCommandId(
    int command_id) const {
  if (IsAssignToAllDesksCommand(command_id))
    return l10n_util::GetStringUTF16(IDS_ASSIGN_TO_ALL_DESKS);

  // It gets desk name for all the desks, and desk items are all dynamic here.
  // Therefore, for the desk a user adds, it returns the name of the desk.
  // Otherwise, the desk name is empty string.
  return views::DesktopWindowTreeHostLinux::From(
             widget_->GetNativeWindow()->GetHost())
      ->GetDeskExtension()
      ->GetDeskName(MapCommandIdToDeskIndex(command_id));
}

void MoveToDesksMenuDelegateLacros::ExecuteCommand(int command_id,
                                                   int event_flags) {
  if (IsAssignToAllDesksCommand(command_id)) {
    widget_->SetVisibleOnAllWorkspaces(!widget_->IsVisibleOnAllWorkspaces());
  } else {
    views::DesktopWindowTreeHostLinux::From(
        widget_->GetNativeWindow()->GetHost())
        ->GetDeskExtension()
        ->SendToDeskAtIndex(MapCommandIdToDeskIndex(command_id));
  }
}
