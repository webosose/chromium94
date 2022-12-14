// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/cloud_content_scanning/binary_upload_service_factory.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/safe_browsing/cloud_content_scanning/binary_upload_service.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/safe_browsing/core/common/features.h"
#include "content/public/browser/browser_context.h"

namespace safe_browsing {

// static
BinaryUploadService* BinaryUploadServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<BinaryUploadService*>(
      GetInstance()->GetServiceForBrowserContext(profile, /* create= */
                                                 true));
}

// static
BinaryUploadServiceFactory* BinaryUploadServiceFactory::GetInstance() {
  return base::Singleton<BinaryUploadServiceFactory>::get();
}

BinaryUploadServiceFactory::BinaryUploadServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BinaryUploadService",
          BrowserContextDependencyManager::GetInstance()) {}

KeyedService* BinaryUploadServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (base::FeatureList::IsEnabled(kSafeBrowsingRemoveCookies)) {
    return new BinaryUploadService(profile);
  } else {
    return new BinaryUploadService(
        g_browser_process->safe_browsing_service()->GetURLLoaderFactory(),
        profile);
  }
}

content::BrowserContext* BinaryUploadServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}

}  // namespace safe_browsing
