// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/full_restore/full_restore_data_handler.h"

#include "chrome/browser/apps/app_service/app_service_proxy.h"
#include "chrome/browser/apps/app_service/app_service_proxy_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/full_restore/full_restore_read_handler.h"
#include "components/full_restore/full_restore_save_handler.h"
#include "components/services/app_service/public/cpp/app_update.h"
#include "components/services/app_service/public/cpp/types_util.h"

namespace ash {
namespace full_restore {

FullRestoreDataHandler::FullRestoreDataHandler(Profile* profile)
    : profile_(profile) {
  DCHECK(
      apps::AppServiceProxyFactory::IsAppServiceAvailableForProfile(profile_));
  Observe(&apps::AppServiceProxyFactory::GetForProfile(profile_)
               ->AppRegistryCache());
}

FullRestoreDataHandler::~FullRestoreDataHandler() = default;

void FullRestoreDataHandler::OnAppUpdate(const apps::AppUpdate& update) {
  if (!update.ReadinessChanged() ||
      apps_util::IsInstalled(update.Readiness())) {
    return;
  }

  // If the user uninstalls an app, then installs it again at the system
  // startup phase, its restore data will be removed if the app isn't reopened.
  ::full_restore::FullRestoreReadHandler* read_handler =
      ::full_restore::FullRestoreReadHandler::GetInstance();
  read_handler->RemoveApp(profile_->GetPath(), update.AppId());

  ::full_restore::FullRestoreSaveHandler::GetInstance()->RemoveApp(
      profile_->GetPath(), update.AppId());
}

void FullRestoreDataHandler::OnAppRegistryCacheWillBeDestroyed(
    apps::AppRegistryCache* cache) {
  apps::AppRegistryCache::Observer::Observe(nullptr);
}

}  // namespace full_restore
}  // namespace ash
