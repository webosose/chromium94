// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBLAYER_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_FACTORY_H_
#define WEBLAYER_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace weblayer {

class BackgroundFetchDelegateImpl;

class BackgroundFetchDelegateFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  // Returns whether the Background Fetch API should be enabled.
  static bool IsEnabled();

  static BackgroundFetchDelegateImpl* GetForBrowserContext(
      content::BrowserContext* context);
  static BackgroundFetchDelegateFactory* GetInstance();

  BackgroundFetchDelegateFactory(const BackgroundFetchDelegateFactory&) =
      delete;
  BackgroundFetchDelegateFactory& operator=(
      const BackgroundFetchDelegateFactory&) = delete;

 private:
  friend struct base::DefaultSingletonTraits<BackgroundFetchDelegateFactory>;

  BackgroundFetchDelegateFactory();
  ~BackgroundFetchDelegateFactory() override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace weblayer

#endif  // WEBLAYER_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_FACTORY_H_
