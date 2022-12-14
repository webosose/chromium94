// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sharesheet/sharesheet_service_delegate.h"

#include <utility>

#include "base/callback.h"
#include "chrome/browser/sharesheet/sharesheet_service.h"
#include "chrome/browser/ui/ash/sharesheet/sharesheet_bubble_view_delegate.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/views/view.h"

namespace sharesheet {

SharesheetServiceDelegate::SharesheetServiceDelegate(
    gfx::NativeWindow native_window,
    SharesheetService* sharesheet_service)
    : native_window_(native_window), sharesheet_service_(sharesheet_service) {
#if BUILDFLAG(IS_CHROMEOS_ASH)
  sharesheet_controller_ =
      std::make_unique<ash::sharesheet::SharesheetBubbleViewDelegate>(
          native_window_, this);
#else
  NOTIMPLEMENTED();
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)
}

SharesheetServiceDelegate::~SharesheetServiceDelegate() = default;

gfx::NativeWindow SharesheetServiceDelegate::GetNativeWindow() {
  return native_window_;
}

SharesheetController* SharesheetServiceDelegate::GetSharesheetController() {
  return sharesheet_controller_.get();
}

Profile* SharesheetServiceDelegate::GetProfile() {
  return sharesheet_service_->GetProfile();
}

void SharesheetServiceDelegate::ShowBubble(std::vector<TargetInfo> targets,
                                           apps::mojom::IntentPtr intent,
                                           DeliveredCallback delivered_callback,
                                           CloseCallback close_callback) {
  sharesheet_controller_->ShowBubble(std::move(targets), std::move(intent),
                                     std::move(delivered_callback),
                                     std::move(close_callback));
}

#if BUILDFLAG(IS_CHROMEOS_ASH)
// Skips the generic Sharesheet bubble and directly displays the
// NearbyShare bubble dialog.
void SharesheetServiceDelegate::ShowNearbyShareBubbleForArc(
    apps::mojom::IntentPtr intent,
    DeliveredCallback delivered_callback,
    CloseCallback close_callback) {
  sharesheet_controller_->ShowNearbyShareBubbleForArc(
      std::move(intent), std::move(delivered_callback),
      std::move(close_callback));
}
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Invoked immediately after an action has launched in the event that UI
// changes need to occur at this point.
void SharesheetServiceDelegate::OnActionLaunched() {
  sharesheet_controller_->OnActionLaunched();
}

void SharesheetServiceDelegate::CloseBubble(SharesheetResult result) {
  sharesheet_controller_->CloseBubble(result);
}

void SharesheetServiceDelegate::OnBubbleClosed(
    const std::u16string& active_action) {
  sharesheet_service_->OnBubbleClosed(native_window_, active_action);
  // This object is now deleted and nothing can be accessed any more.
}

void SharesheetServiceDelegate::OnTargetSelected(
    const std::u16string& target_name,
    const TargetType type,
    apps::mojom::IntentPtr intent,
    views::View* share_action_view) {
  sharesheet_service_->OnTargetSelected(native_window_, target_name, type,
                                        std::move(intent), share_action_view);
}

bool SharesheetServiceDelegate::OnAcceleratorPressed(
    const ui::Accelerator& accelerator,
    const std::u16string& active_action) {
  return sharesheet_service_->OnAcceleratorPressed(accelerator, active_action);
}

const gfx::VectorIcon* SharesheetServiceDelegate::GetVectorIcon(
    const std::u16string& display_name) {
  return sharesheet_service_->GetVectorIcon(display_name);
}

}  // namespace sharesheet
