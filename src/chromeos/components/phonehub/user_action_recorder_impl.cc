// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/phonehub/user_action_recorder_impl.h"

#include "base/metrics/histogram_functions.h"
#include "chromeos/components/phonehub/feature_status.h"
#include "chromeos/components/phonehub/feature_status_provider.h"

namespace chromeos {
namespace phonehub {

UserActionRecorderImpl::UserActionRecorderImpl(
    FeatureStatusProvider* feature_status_provider)
    : feature_status_provider_(feature_status_provider) {}

UserActionRecorderImpl::~UserActionRecorderImpl() = default;

void UserActionRecorderImpl::RecordUiOpened() {
  if (feature_status_provider_->GetStatus() ==
      FeatureStatus::kEnabledAndConnected) {
    HandleUserAction(UserAction::kUiOpened);
  }
}

void UserActionRecorderImpl::RecordTetherConnectionAttempt() {
  HandleUserAction(UserAction::kTether);
}

void UserActionRecorderImpl::RecordDndAttempt() {
  HandleUserAction(UserAction::kDnd);
}

void UserActionRecorderImpl::RecordFindMyDeviceAttempt() {
  HandleUserAction(UserAction::kFindMyDevice);
}

void UserActionRecorderImpl::RecordBrowserTabOpened() {
  HandleUserAction(UserAction::kBrowserTab);
}

void UserActionRecorderImpl::RecordNotificationDismissAttempt() {
  HandleUserAction(UserAction::kNotificationDismissal);
}

void UserActionRecorderImpl::RecordNotificationReplyAttempt() {
  HandleUserAction(UserAction::kNotificationReply);
}

void UserActionRecorderImpl::HandleUserAction(UserAction action) {
  base::UmaHistogramEnumeration("PhoneHub.CompletedUserAction", action);
}

}  // namespace phonehub
}  // namespace chromeos
