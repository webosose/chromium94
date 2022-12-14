// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/search/instant_types.h"

NtpTheme::NtpTheme() = default;

NtpTheme::NtpTheme(const NtpTheme& other) = default;

NtpTheme::~NtpTheme() = default;

bool NtpTheme::operator==(const NtpTheme& rhs) const {
  return using_default_theme == rhs.using_default_theme &&
         custom_background_url == rhs.custom_background_url &&
         custom_background_attribution_line_1 ==
             rhs.custom_background_attribution_line_1 &&
         custom_background_attribution_line_2 ==
             rhs.custom_background_attribution_line_2 &&
         custom_background_attribution_action_url ==
             rhs.custom_background_attribution_action_url &&
         collection_id == rhs.collection_id &&
         background_color == rhs.background_color &&
         text_color == rhs.text_color &&
         text_color_light == rhs.text_color_light && theme_id == rhs.theme_id &&
         image_horizontal_alignment == rhs.image_horizontal_alignment &&
         image_vertical_alignment == rhs.image_vertical_alignment &&
         image_tiling == rhs.image_tiling &&
         has_attribution == rhs.has_attribution &&
         logo_alternate == rhs.logo_alternate &&
         has_theme_image == rhs.has_theme_image;
}

InstantMostVisitedItem::InstantMostVisitedItem()
    : title_source(ntp_tiles::TileTitleSource::UNKNOWN),
      source(ntp_tiles::TileSource::TOP_SITES) {}

InstantMostVisitedItem::InstantMostVisitedItem(
    const InstantMostVisitedItem& other) = default;

InstantMostVisitedItem::~InstantMostVisitedItem() {}

InstantMostVisitedInfo::InstantMostVisitedInfo() = default;

InstantMostVisitedInfo::InstantMostVisitedInfo(
    const InstantMostVisitedInfo& other) = default;

InstantMostVisitedInfo::~InstantMostVisitedInfo() {}
