// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/global_media_controls/media_notification_list_view.h"

#include "base/containers/contains.h"
#include "chrome/browser/ui/views/global_media_controls/media_notification_container_impl_view.h"
#include "chrome/browser/ui/views/global_media_controls/overlay_media_notification_view.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/border.h"
#include "ui/views/controls/scrollbar/overlay_scroll_bar.h"
#include "ui/views/layout/box_layout.h"

namespace {

constexpr int kMediaListMaxHeight = 478;

// Thickness of separator border.
constexpr int kMediaListSeparatorThickness = 2;

std::unique_ptr<views::Border> CreateMediaListSeparatorBorder(SkColor color,
                                                              int thickness) {
  return views::CreateSolidSidedBorder(/*top=*/thickness,
                                       /*left=*/0,
                                       /*bottom=*/0,
                                       /*right=*/0, color);
}

}  // anonymous namespace

MediaNotificationListView::SeparatorStyle::SeparatorStyle(
    SkColor separator_color,
    int separator_thickness)
    : separator_color(separator_color),
      separator_thickness(separator_thickness) {}

MediaNotificationListView::MediaNotificationListView()
    : MediaNotificationListView(absl::nullopt) {}

MediaNotificationListView::MediaNotificationListView(
    const absl::optional<SeparatorStyle>& separator_style)
    : separator_style_(separator_style) {
  SetBackgroundColor(absl::nullopt);
  SetContents(std::make_unique<views::View>());
  contents()->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  ClipHeightTo(0, kMediaListMaxHeight);

  SetVerticalScrollBar(
      std::make_unique<views::OverlayScrollBar>(/*horizontal=*/false));
  SetHorizontalScrollBar(
      std::make_unique<views::OverlayScrollBar>(/*horizontal=*/true));
}

MediaNotificationListView::~MediaNotificationListView() = default;

void MediaNotificationListView::ShowNotification(
    const std::string& id,
    std::unique_ptr<MediaNotificationContainerImplView> notification) {
  DCHECK(!base::Contains(notifications_, id));
  DCHECK_NE(nullptr, notification.get());

  // If this isn't the first notification, then create a top-sided separator
  // border.
  if (!notifications_.empty()) {
    if (separator_style_.has_value()) {
      notification->SetBorder(CreateMediaListSeparatorBorder(
          separator_style_->separator_color,
          separator_style_->separator_thickness));
    } else {
      notification->SetBorder(CreateMediaListSeparatorBorder(
          GetNativeTheme()->GetSystemColor(
              ui::NativeTheme::kColorId_MenuSeparatorColor),
          kMediaListSeparatorThickness));
    }
  }

  notifications_[id] = contents()->AddChildView(std::move(notification));

  contents()->InvalidateLayout();
  PreferredSizeChanged();
}

void MediaNotificationListView::HideNotification(const std::string& id) {
  RemoveNotification(id);
}

std::unique_ptr<OverlayMediaNotification> MediaNotificationListView::PopOut(
    const std::string& id,
    gfx::Rect bounds) {
  std::unique_ptr<MediaNotificationContainerImplView> notification =
      RemoveNotification(id);
  if (!notification)
    return nullptr;

  return std::make_unique<OverlayMediaNotificationView>(
      id, std::move(notification), bounds, nullptr);
}

std::unique_ptr<MediaNotificationContainerImplView>
MediaNotificationListView::RemoveNotification(const std::string& id) {
  if (!base::Contains(notifications_, id))
    return nullptr;

  // If we're removing the topmost notification and there are others, then we
  // need to remove the top-sided separator border from the new topmost
  // notification.
  if (contents()->children().size() > 1 &&
      contents()->children().at(0) == notifications_[id]) {
    contents()->children().at(1)->SetBorder(nullptr);
  }

  // Remove the notification. Note that since |RemoveChildView()| does not
  // delete the notification, we now have ownership.
  contents()->RemoveChildView(notifications_[id]);
  std::unique_ptr<MediaNotificationContainerImplView> notification(
      notifications_[id]);
  notifications_.erase(id);

  contents()->InvalidateLayout();
  PreferredSizeChanged();

  return notification;
}

BEGIN_METADATA(MediaNotificationListView, views::ScrollView)
END_METADATA
