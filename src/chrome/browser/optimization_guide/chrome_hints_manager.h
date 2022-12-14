// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OPTIMIZATION_GUIDE_CHROME_HINTS_MANAGER_H_
#define CHROME_BROWSER_OPTIMIZATION_GUIDE_CHROME_HINTS_MANAGER_H_

#include "chrome/browser/navigation_predictor/navigation_predictor_keyed_service.h"
#include "components/optimization_guide/core/hints_manager.h"

class Profile;

namespace optimization_guide {

class ChromeHintsManager : public HintsManager,
                           public NavigationPredictorKeyedService::Observer {
 public:
  ChromeHintsManager(
      Profile* profile,
      PrefService* pref_service,
      optimization_guide::OptimizationGuideStore* hint_store,
      optimization_guide::TopHostProvider* top_host_provider,
      optimization_guide::TabUrlProvider* tab_url_provider,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      network::NetworkConnectionTracker* network_connection_tracker);

  ~ChromeHintsManager() override;

  // Unhooks the observer from the navigation predictor service.
  void Shutdown();

  // NavigationPredictorKeyedService::Observer:
  void OnPredictionUpdated(
      const absl::optional<NavigationPredictorKeyedService::Prediction>
          prediction) override;

 private:
  // A reference to the profile. Not owned.
  Profile* profile_ = nullptr;
};

}  // namespace optimization_guide

#endif  // CHROME_BROWSER_OPTIMIZATION_GUIDE_CHROME_HINTS_MANAGER_H_
