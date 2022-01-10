// Copyright (c) 2022 LG Electronics, Inc.
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

#include "neva/app_runtime/browser/notifications/notification_platform_bridge_webos.h"

#include "base/json/json_writer.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "neva/pal_service/luna/luna_client.h"
#include "ui/message_center/public/cpp/notification.h"

namespace neva_app_runtime {

// static
std::unique_ptr<NotificationPlatformBridge>
NotificationPlatformBridge::Create() {
  return std::make_unique<NotificationPlatformBridgeWebos>();
}

// static
bool NotificationPlatformBridge::CanHandleType(
    NotificationHandler::Type notification_type) {
  return notification_type != NotificationHandler::Type::TRANSIENT;
}

NotificationPlatformBridgeWebos::NotificationPlatformBridgeWebos() = default;

NotificationPlatformBridgeWebos::~NotificationPlatformBridgeWebos() = default;

void NotificationPlatformBridgeWebos::Display(
    NotificationHandler::Type notification_type,
    content::BrowserContext* profile,
    const message_center::Notification& notification,
    std::unique_ptr<NotificationCommon::Metadata> metadata) {
  if (!luna_client_) {
    pal::luna::Client::Params params;
    params.name = "com.webos.notification.client";
    luna_client_ = pal::luna::CreateClient(params);
  }
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("sourceId", metadata->web_app_id);
  std::string message = "[" + base::UTF16ToUTF8(notification.title()) + "]";
  if (!notification.message().empty()) {
    message += " " + base::UTF16ToUTF8(notification.message());
  }
  dict.SetStringKey("message", message);
  dict.SetStringKey("type", "advanced");
  dict.SetBoolKey("persistent", true);
  std::string param;
  base::JSONWriter::Write(dict, &param);
  luna_client_->Call("luna://com.webos.notification/createToast", param);
}

void NotificationPlatformBridgeWebos::Close(
    content::BrowserContext* profile,
    const std::string& notification_id) {
  NOTIMPLEMENTED();
}

void NotificationPlatformBridgeWebos::GetDisplayed(
    content::BrowserContext* profile,
    GetDisplayedNotificationsCallback callback) const {
  NOTIMPLEMENTED();
}

void NotificationPlatformBridgeWebos::SetReadyCallback(
    NotificationBridgeReadyCallback callback) {
  NOTIMPLEMENTED();
}

void NotificationPlatformBridgeWebos::DisplayServiceShutDown(
    content::BrowserContext* profile) {
  NOTIMPLEMENTED();
}

}  // namespace neva_app_runtime
