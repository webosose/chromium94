// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copied from chrome/browser/notifications/notification_permission_context.cc

#include "neva/app_runtime/browser/notifications/notification_permission_context.h"

#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/permission_util.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/blink/public/mojom/permissions_policy/permissions_policy_feature.mojom.h"
#include "url/gurl.h"

// static
void NotificationPermissionContext::UpdatePermission(
    content::BrowserContext* browser_context,
    const GURL& origin,
    ContentSetting setting) {}

NotificationPermissionContext::NotificationPermissionContext(
    content::BrowserContext* browser_context)
    : PermissionContextBase(browser_context,
                            ContentSettingsType::NOTIFICATIONS,
                            blink::mojom::PermissionsPolicyFeature::kNotFound) {
}

NotificationPermissionContext::~NotificationPermissionContext() {}

void NotificationPermissionContext::RequestPermission(
    content::WebContents* web_contents,
    const permissions::PermissionRequestID& id,
    const GURL& requesting_frame,
    bool user_gesture,
    BrowserPermissionCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  GURL requesting_origin = requesting_frame.GetOrigin();
  GURL embedding_origin =
      permissions::PermissionUtil::GetLastCommittedOriginAsURL(web_contents);

  // We are going to get permission status or to show a prompt now.
  DecidePermission(web_contents, id, requesting_origin, embedding_origin,
                   user_gesture, std::move(callback));
}

ContentSetting NotificationPermissionContext::GetPermissionStatusInternal(
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const GURL& embedding_origin) const {
  return ContentSetting::CONTENT_SETTING_ALLOW;
}

void NotificationPermissionContext::DecidePermission(
    content::WebContents* web_contents,
    const permissions::PermissionRequestID& id,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    bool user_gesture,
    permissions::BrowserPermissionCallback callback) {
  permissions::PermissionRequestManager::CreateForWebContents(web_contents);
  permissions::PermissionContextBase::DecidePermission(
      web_contents, id, requesting_origin, embedding_origin, user_gesture,
      std::move(callback));
}

void NotificationPermissionContext::UpdateContentSetting(
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    ContentSetting content_setting,
    bool is_one_time) {}

bool NotificationPermissionContext::IsRestrictedToSecureOrigins() const {
  return false;
}
