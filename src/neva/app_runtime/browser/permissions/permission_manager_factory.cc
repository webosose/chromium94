// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copied from chrome/browser/permissions/permission_manager_factory.cc

#include "neva/app_runtime/browser/permissions/permission_manager_factory.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"
#include "neva/app_runtime/browser/notifications/notification_permission_context.h"

namespace {
permissions::PermissionManager::PermissionContextMap CreatePermissionContexts(
    content::BrowserContext* context) {
  // Create default permission contexts initially.
  permissions::PermissionManager::PermissionContextMap permission_contexts;
  permission_contexts[ContentSettingsType::NOTIFICATIONS] =
      std::make_unique<NotificationPermissionContext>(context);
  return permission_contexts;
}
}  // namespace
// static
permissions::PermissionManager* PermissionManagerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<permissions::PermissionManager*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
PermissionManagerFactory* PermissionManagerFactory::GetInstance() {
  return base::Singleton<PermissionManagerFactory>::get();
}
PermissionManagerFactory::PermissionManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "PermissionManagerFactory",
          BrowserContextDependencyManager::GetInstance()) {}
PermissionManagerFactory::~PermissionManagerFactory() {}

// BrowserContextKeyedServiceFactory methods:
KeyedService* PermissionManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new permissions::PermissionManager(context,
                                            CreatePermissionContexts(context));
}
