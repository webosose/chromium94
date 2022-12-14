// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_POLICY_INVALIDATION_FAKE_AFFILIATED_INVALIDATION_SERVICE_PROVIDER_H_
#define CHROME_BROWSER_ASH_POLICY_INVALIDATION_FAKE_AFFILIATED_INVALIDATION_SERVICE_PROVIDER_H_

#include "base/macros.h"
#include "chrome/browser/ash/policy/invalidation/affiliated_invalidation_service_provider.h"

namespace policy {

class FakeAffiliatedInvalidationServiceProvider
    : public AffiliatedInvalidationServiceProvider {
 public:
  FakeAffiliatedInvalidationServiceProvider();

  // AffiliatedInvalidationServiceProvider:
  void RegisterConsumer(Consumer* consumer) override;
  void UnregisterConsumer(Consumer* consumer) override;
  void Shutdown() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeAffiliatedInvalidationServiceProvider);
};

}  // namespace policy

#endif  // CHROME_BROWSER_ASH_POLICY_INVALIDATION_FAKE_AFFILIATED_INVALIDATION_SERVICE_PROVIDER_H_
