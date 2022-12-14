// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_WALLPAPER_ONLINE_WALLPAPER_PARAMS_H_
#define ASH_PUBLIC_CPP_WALLPAPER_ONLINE_WALLPAPER_PARAMS_H_

#include <cstdint>
#include <string>

#include "ash/public/cpp/wallpaper/wallpaper_types.h"
#include "components/account_id/account_id.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace ash {

struct ASH_PUBLIC_EXPORT OnlineWallpaperParams {
  // The user's account id.
  AccountId account_id;
  // The unique identifier of the wallpaper. Empty when the image is auto
  // refreshed from old wallpaper app.
  // TODO(b/193788853): Make this required after deprecating old wallpaper app.
  absl::optional<uint64_t> asset_id;
  // The wallpaper url.
  GURL url;
  // The wallpaper collection id .e.g. city_for_chromebook.
  std::string collection_id;
  // The layout of the wallpaper, used for wallpaper resizing.
  WallpaperLayout layout;
  // If true, show the wallpaper immediately but doesn't change the user
  // wallpaper info until |ConfirmPreviewWallpaper| is called.
  bool preview_mode;
  // Indicate the params is a result of a user's request. i.e clicking on an
  // image.
  bool from_user = false;
  // If the |WallpaperInfo| generated from these params should have type
  // |WallpaperType::DAILY|.
  bool daily_refresh_enabled = false;

  OnlineWallpaperParams(const AccountId& account_id,
                        const absl::optional<uint64_t>& asset_id,
                        const GURL& url,
                        const std::string& collection_id,
                        WallpaperLayout layout,
                        bool preview_mode,
                        bool from_user,
                        bool daily_refresh_enabled);

  OnlineWallpaperParams(const OnlineWallpaperParams& other);

  OnlineWallpaperParams(OnlineWallpaperParams&& other);

  OnlineWallpaperParams& operator=(const OnlineWallpaperParams& other);

  ~OnlineWallpaperParams();
};

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_WALLPAPER_ONLINE_WALLPAPER_PARAMS_H_
