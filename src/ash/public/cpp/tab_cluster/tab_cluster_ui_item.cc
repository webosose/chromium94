// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/tab_cluster/tab_cluster_ui_item.h"

namespace ash {

////////////////////////////////////////////////////////////////////////////////
// TabClusterUIItem::Info:
TabClusterUIItem::Info::Info() = default;

TabClusterUIItem::Info::Info(const Info&) = default;

TabClusterUIItem::Info& TabClusterUIItem::Info::operator=(
    const TabClusterUIItem::Info&) = default;

TabClusterUIItem::Info::~Info() = default;

////////////////////////////////////////////////////////////////////////////////
// TabClusterUIItem:
TabClusterUIItem::TabClusterUIItem() = default;

TabClusterUIItem::TabClusterUIItem(const TabClusterUIItem::Info& info) {
  Init(info);
}

TabClusterUIItem::~TabClusterUIItem() = default;

void TabClusterUIItem::Init(const TabClusterUIItem::Info& info) {
  old_info_ = current_info_;
  current_info_ = info;
}

}  // namespace ash
