// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/eche_app_ui/eche_notification_click_handler.h"

#include "chromeos/components/multidevice/logging/logging.h"
#include "chromeos/components/phonehub/phone_hub_manager.h"

namespace chromeos {
namespace eche_app {

EcheNotificationClickHandler::EcheNotificationClickHandler(
    phonehub::PhoneHubManager* phone_hub_manager,
    FeatureStatusProvider* feature_status_provider,
    LaunchEcheAppFunction launch_eche_app_function,
    CloseEcheAppFunction close_eche_app_function)
    : feature_status_provider_(feature_status_provider),
      launch_eche_app_function_(launch_eche_app_function),
      close_eche_app_function_(close_eche_app_function) {
  handler_ = phone_hub_manager->GetNotificationInteractionHandler();
  feature_status_provider_->AddObserver(this);
  if (handler_ && IsClickable(feature_status_provider_->GetStatus())) {
    handler_->AddNotificationClickHandler(this);
    is_click_handler_set_ = true;
  } else {
    PA_LOG(INFO)
        << "No Phone Hub interaction handler to set Eche click handler";
  }
}

EcheNotificationClickHandler::~EcheNotificationClickHandler() {
  feature_status_provider_->RemoveObserver(this);
  if (is_click_handler_set_ && handler_)
    handler_->RemoveNotificationClickHandler(this);
}

void EcheNotificationClickHandler::HandleNotificationClick(
    int64_t notification_id,
    const phonehub::Notification::AppMetadata& app_metadata) {
  launch_eche_app_function_.Run(notification_id, app_metadata.package_name);
}

void EcheNotificationClickHandler::OnFeatureStatusChanged() {
  if (!handler_) {
    PA_LOG(INFO)
        << "No Phone Hub interaction handler to set Eche click handler";
    return;
  }

  bool clickable = IsClickable(feature_status_provider_->GetStatus());
  if (!is_click_handler_set_ && clickable) {
    handler_->AddNotificationClickHandler(this);
    is_click_handler_set_ = true;
  } else if (is_click_handler_set_ && !clickable) {
    handler_->RemoveNotificationClickHandler(this);
    is_click_handler_set_ = false;
    if (NeedClose(feature_status_provider_->GetStatus())) {
      PA_LOG(INFO) << "Close Eche app window";
      close_eche_app_function_.Run();
    }
  }
}

bool EcheNotificationClickHandler::IsClickable(FeatureStatus status) {
  return status == FeatureStatus::kDisconnected ||
         status == FeatureStatus::kConnecting ||
         status == FeatureStatus::kConnected;
}

// Checks FeatureStatus that eche feature is not able to use.
bool EcheNotificationClickHandler::NeedClose(FeatureStatus status) {
  return status == FeatureStatus::kIneligible ||
         status == FeatureStatus::kDisabled ||
         status == FeatureStatus::kDependentFeature;
}
}  // namespace eche_app
}  // namespace chromeos
