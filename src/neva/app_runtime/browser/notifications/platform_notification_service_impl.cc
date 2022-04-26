// Copyright 2022 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
#include "neva/app_runtime/browser/notifications/platform_notification_service_impl.h"

#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "content/public/browser/notification_event_dispatcher.h"

namespace neva_app_runtime {

PlatformNotificationServiceImpl::PlatformNotificationServiceImpl(
    content::BrowserContext* context)
    : context_(context) {}

void PlatformNotificationServiceImpl::DisplayNotification(
    const std::string& notification_id,
    const GURL& origin,
    const GURL& document_url,
    const blink::PlatformNotificationData& notification_data,
    const blink::NotificationResources& notification_resources) {
  LOG(INFO) << __func__ << " NOTIMPLEMENTED";
}

// Displays the persistent notification described in |notification_data| to
// the user. This method must be called on the UI thread.
void PlatformNotificationServiceImpl::DisplayPersistentNotification(
    const std::string& notification_id,
    const GURL& service_worker_origin,
    const GURL& origin,
    const blink::PlatformNotificationData& notification_data,
    const blink::NotificationResources& notification_resources) {
  LOG(INFO) << __func__ << " NOTIMPLEMENTED";
}

// Closes the notification identified by |notification_id|. This method must
// be called on the UI thread.
void PlatformNotificationServiceImpl::CloseNotification(
    const std::string& notification_id) {
  LOG(INFO) << __func__ << " NOTIMPLEMENTED";
}

// Closes the persistent notification identified by |notification_id|. This
// method must be called on the UI thread.
void PlatformNotificationServiceImpl::ClosePersistentNotification(
    const std::string& notification_id) {
  LOG(INFO) << __func__ << " NOTIMPLEMENTED";
}

// Retrieves the ids of all currently displaying notifications and
// posts |callback| with the result.
void PlatformNotificationServiceImpl::GetDisplayedNotifications(
    DisplayedNotificationsCallback callback) {
  LOG(INFO) << __func__ << " NOTIMPLEMENTED";
  std::set<std::string> displayed_notifications;
  std::move(callback).Run(std::move(displayed_notifications), true);
}

// Schedules a job to run at |timestamp| and call TriggerNotifications
// on all PlatformNotificationContext instances.
void PlatformNotificationServiceImpl::ScheduleTrigger(base::Time timestamp) {
  LOG(INFO) << __func__ << " NOTIMPLEMENTED";
}

// Reads the value of the next notification trigger time for this profile.
// This will return base::Time::Max if there is no trigger set.
base::Time PlatformNotificationServiceImpl::ReadNextTriggerTimestamp() {
  LOG(INFO) << __func__ << " NOTIMPLEMENTED";
  return base::Time::Max();
}

// Reads the value of the next persistent notification ID from the profile and
// increments the value, as it is called once per notification write.
int64_t PlatformNotificationServiceImpl::ReadNextPersistentNotificationId() {
  LOG(INFO) << __func__ << " NOTIMPLEMENTED";
  return 0;
}

// Records a given notification to UKM.
void PlatformNotificationServiceImpl::RecordNotificationUkmEvent(
    const content::NotificationDatabaseData& data) {
  LOG(INFO) << __func__ << " NOTIMPLEMENTED";
}

}  // namespace neva_app_runtime
