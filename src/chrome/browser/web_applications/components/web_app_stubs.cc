// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(crbug.com/1231621): Implement some/all of these once Fuchsia supports
// them.

#include "chrome/browser/web_applications/components/web_app_file_handler_registration.h"
#include "chrome/browser/web_applications/components/web_app_shortcut.h"

#include "base/notreached.h"

namespace web_app {

bool ShouldRegisterFileHandlersWithOs() {
  return false;
}

void RegisterFileHandlersWithOs(const AppId& app_id,
                                const std::string& app_name,
                                Profile* profile,
                                const apps::FileHandlers& file_handlers) {
  NOTIMPLEMENTED();
}

void UnregisterFileHandlersWithOs(const AppId& app_id,
                                  Profile* profile,
                                  base::OnceCallback<void(bool)> callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(false);
}

namespace internals {

bool RegisterRunOnOsLogin(const ShortcutInfo& shortcut_info) {
  NOTIMPLEMENTED();
  return false;
}

bool UnregisterRunOnOsLogin(const std::string& app_id,
                            const base::FilePath& profile_path,
                            const std::u16string& shortcut_title) {
  NOTIMPLEMENTED();
  return false;
}

bool CreatePlatformShortcuts(const base::FilePath& web_app_path,
                             const ShortcutLocations& creation_locations,
                             ShortcutCreationReason creation_reason,
                             const ShortcutInfo& shortcut_info) {
  NOTIMPLEMENTED();
  return false;
}

void UpdatePlatformShortcuts(const base::FilePath& web_app_path,
                             const std::u16string& old_app_title,
                             const ShortcutInfo& shortcut_info) {
  NOTIMPLEMENTED();
}

bool DeletePlatformShortcuts(const base::FilePath& web_app_path,
                             const ShortcutInfo& shortcut_info) {
  NOTIMPLEMENTED();
  return false;
}

void DeleteAllShortcutsForProfile(const base::FilePath& profile_path) {
  NOTIMPLEMENTED();
}

ShortcutLocations GetAppExistingShortCutLocationImpl(
    const ShortcutInfo& shortcut_info) {
  NOTIMPLEMENTED();
  return {};
}

}  // namespace internals

}  // namespace web_app
