// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_FULL_RESTORE_FULL_RESTORE_UTIL_H_
#define ASH_WM_FULL_RESTORE_FULL_RESTORE_UTIL_H_

#include "ash/ash_export.h"
#include "components/full_restore/window_info.h"

namespace aura {
class Window;
}

namespace ash {

// Builds the WindowInfo for |window|. Optionally passes |activation_index|,
// which is used to set |WindowInfo.activation_index| if it has value.
// Otherwise, |WindowInfo.activation_index| will be calculated from
// |mru_windows|.
ASH_EXPORT std::unique_ptr<::full_restore::WindowInfo> BuildWindowInfo(
    aura::Window* window,
    absl::optional<int> activation_index,
    const std::vector<aura::Window*>& mru_windows);

}  // namespace ash

#endif  // ASH_WM_FULL_RESTORE_FULL_RESTORE_UTIL_H_
