// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEBID_FEDERATED_IDENTITY_REQUEST_PERMISSION_CONTEXT_FACTORY_H_
#define CHROME_BROWSER_WEBID_FEDERATED_IDENTITY_REQUEST_PERMISSION_CONTEXT_FACTORY_H_

#include "base/no_destructor.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class FederatedIdentityRequestPermissionContext;

// Factory to get or create an instance of
// FederatedIdentitySharingPermissionContext from a Profile.
class FederatedIdentityRequestPermissionContextFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static FederatedIdentityRequestPermissionContext* GetForProfile(
      content::BrowserContext* profile);
  static FederatedIdentityRequestPermissionContextFactory* GetInstance();

 private:
  friend class base::NoDestructor<
      FederatedIdentityRequestPermissionContextFactory>;

  FederatedIdentityRequestPermissionContextFactory();
  ~FederatedIdentityRequestPermissionContextFactory() override;

  // BrowserContextKeyedServiceFactory:
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
  void BrowserContextShutdown(content::BrowserContext* context) override;
};

#endif  // CHROME_BROWSER_WEBID_FEDERATED_IDENTITY_REQUEST_PERMISSION_CONTEXT_FACTORY_H_
