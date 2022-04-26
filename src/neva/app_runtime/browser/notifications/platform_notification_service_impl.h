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
#ifndef NEVA_APP_RUNTIME_BROWSER_NOTIFICATIONS_PLATFORM_NOTIFICATION_SERVICE_IMPL_H_
#define NEVA_APP_RUNTIME_BROWSER_NOTIFICATIONS_PLATFORM_NOTIFICATION_SERVICE_IMPL_H_

#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/platform_notification_service.h"

namespace content {
class BrowserContext;
}

namespace neva_app_runtime {
class PlatformNotificationServiceImpl
    : public content::PlatformNotificationService,
      public KeyedService {
 public:
  explicit PlatformNotificationServiceImpl(content::BrowserContext* context);
  ~PlatformNotificationServiceImpl() override = default;
  // Displays the notification described in |notification_data| to the user.
  // This method must be called on the UI thread. |document_url| is empty when
  // the display notification originates from a worker.
  void DisplayNotification(
      const std::string& notification_id,
      const GURL& origin,
      const GURL& document_url,
      const blink::PlatformNotificationData& notification_data,
      const blink::NotificationResources& notification_resources) override;

  // Displays the persistent notification described in |notification_data| to
  // the user. This method must be called on the UI thread.
  void DisplayPersistentNotification(
      const std::string& notification_id,
      const GURL& service_worker_origin,
      const GURL& origin,
      const blink::PlatformNotificationData& notification_data,
      const blink::NotificationResources& notification_resources) override;

  // Closes the notification identified by |notification_id|. This method must
  // be called on the UI thread.
  void CloseNotification(const std::string& notification_id) override;

  // Closes the persistent notification identified by |notification_id|. This
  // method must be called on the UI thread.
  void ClosePersistentNotification(const std::string& notification_id) override;

  // Retrieves the ids of all currently displaying notifications and
  // posts |callback| with the result.
  void GetDisplayedNotifications(
      DisplayedNotificationsCallback callback) override;

  // Schedules a job to run at |timestamp| and call TriggerNotifications
  // on all PlatformNotificationContext instances.
  void ScheduleTrigger(base::Time timestamp) override;

  // Reads the value of the next notification trigger time for this profile.
  // This will return base::Time::Max if there is no trigger set.
  base::Time ReadNextTriggerTimestamp() override;

  // Reads the value of the next persistent notification ID from the profile and
  // increments the value, as it is called once per notification write.
  int64_t ReadNextPersistentNotificationId() override;

  // Records a given notification to UKM.
  void RecordNotificationUkmEvent(
      const content::NotificationDatabaseData& data) override;

 private:
  content::BrowserContext* context_ = nullptr;
};

}  // namespace neva_app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_NOTIFICATIONS_PLATFORM_NOTIFICATION_SERVICE_IMPL_H_
