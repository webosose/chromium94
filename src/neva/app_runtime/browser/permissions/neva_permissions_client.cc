// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copied from chrome/browser/permissions/chrome_permissions_client.cc

#include "neva/app_runtime/browser/permissions/neva_permissions_client.h"

#include "components/content_settings/core/browser/cookie_settings.h"
#include "neva/app_runtime/browser/host_content_settings_map_factory.h"
#include "neva/app_runtime/browser/permissions/permission_manager_factory.h"
#include "neva/app_runtime/browser/permissions/permission_prompt.h"
#include "neva/app_runtime/browser/permissions/permission_prompt_webos.h"

// static
NevaPermissionsClient* NevaPermissionsClient::GetInstance() {
  static base::NoDestructor<NevaPermissionsClient> instance;
  return instance.get();
}

HostContentSettingsMap* NevaPermissionsClient::GetSettingsMap(
    content::BrowserContext* browser_context) {
  return neva_app_runtime::HostContentSettingsMapFactory::GetForBrowserContext(
      browser_context);
}

scoped_refptr<content_settings::CookieSettings>
NevaPermissionsClient::GetCookieSettings(
    content::BrowserContext* browser_context) {
  return scoped_refptr<content_settings::CookieSettings>(nullptr);
}

bool NevaPermissionsClient::IsSubresourceFilterActivated(
    content::BrowserContext* browser_context,
    const GURL& url) {
  return false;
}

permissions::PermissionDecisionAutoBlocker*
NevaPermissionsClient::GetPermissionDecisionAutoBlocker(
    content::BrowserContext* browser_context) {
  return nullptr;
}

permissions::PermissionManager* NevaPermissionsClient::GetPermissionManager(
    content::BrowserContext* browser_context) {
  return PermissionManagerFactory::GetForBrowserContext(browser_context);
}

permissions::ObjectPermissionContextBase*
NevaPermissionsClient::GetChooserContext(
    content::BrowserContext* browser_context,
    ContentSettingsType type) {
  return nullptr;
}

bool NevaPermissionsClient::CanBypassEmbeddingOriginCheck(
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  return true;
}
std::unique_ptr<permissions::PermissionPrompt>
NevaPermissionsClient::CreatePrompt(
    content::WebContents* web_contents,
    permissions::PermissionPrompt::Delegate* delegate) {
  return CreatePermissionPrompt(web_contents, delegate);
}
