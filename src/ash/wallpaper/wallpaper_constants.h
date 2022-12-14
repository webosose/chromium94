// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WALLPAPER_WALLPAPER_CONSTANTS_H_
#define ASH_WALLPAPER_WALLPAPER_CONSTANTS_H_

#include "ash/login/ui/login_constants.h"
#include "ash/style/ash_color_provider.h"

namespace ash {

namespace wallpaper_constants {

// Blur sigma used for normal wallpaper.
constexpr float kClear = 0.f;
// Blur sigma used in overview mode.
constexpr float kOverviewBlur =
    static_cast<float>(AshColorProvider::LayerBlurSigma::kBlurDefault);
// Blur sigma in lock/login screen.
constexpr float kLockLoginBlur = 30.0f;

}  // namespace wallpaper_constants

}  // namespace ash

#endif  // ASH_WALLPAPER_WALLPAPER_CONSTANTS_H_
