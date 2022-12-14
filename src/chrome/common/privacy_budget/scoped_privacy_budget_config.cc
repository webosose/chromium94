// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/privacy_budget/scoped_privacy_budget_config.h"

#include "base/check.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/common/privacy_budget/field_trial_param_conversions.h"
#include "chrome/common/privacy_budget/privacy_budget_features.h"
#include "chrome/common/privacy_budget/types.h"
#include "third_party/blink/public/common/privacy_budget/identifiability_study_settings.h"
#include "third_party/blink/public/common/privacy_budget/identifiable_surface.h"

namespace test {

ScopedPrivacyBudgetConfig::Parameters::Parameters() = default;
ScopedPrivacyBudgetConfig::Parameters::Parameters(const Parameters&) = default;
ScopedPrivacyBudgetConfig::Parameters::Parameters(Parameters&&) = default;
ScopedPrivacyBudgetConfig::Parameters::~Parameters() = default;

ScopedPrivacyBudgetConfig::~ScopedPrivacyBudgetConfig() {
  DCHECK(applied_) << "ScopedPrivacyBudgetConfig instance created but not "
                      "applied. Did you forget to call Apply()?";
}

ScopedPrivacyBudgetConfig::ScopedPrivacyBudgetConfig() = default;

ScopedPrivacyBudgetConfig::ScopedPrivacyBudgetConfig(
    const Parameters& parameters) {
  Apply(parameters);
}

ScopedPrivacyBudgetConfig::ScopedPrivacyBudgetConfig(Presets presets) {
  switch (presets) {
    case kEnable:
      Apply(Parameters());
      break;

    case kDisable: {
      Parameters parameters;
      parameters.enabled = false;
      Apply(parameters);
      break;
    }
  }
}

void ScopedPrivacyBudgetConfig::Apply(const Parameters& parameters) {
  blink::IdentifiabilityStudySettings::ResetStateForTesting();
  DCHECK(!applied_);
  applied_ = true;

  if (!parameters.enabled) {
    scoped_feature_list_.InitAndDisableFeature(features::kIdentifiabilityStudy);
    return;
  }

  Parameters defaults;

  base::FieldTrialParams ftp;

  ftp.insert({features::kIdentifiabilityStudyGeneration.name,
              base::NumberToString(parameters.generation)});

  if (!parameters.blocked_surfaces.empty()) {
    ftp.insert(
        {features::kIdentifiabilityStudyBlockedMetrics.name,
         EncodeIdentifiabilityFieldTrialParam(parameters.blocked_surfaces)});
  }
  if (!parameters.blocked_types.empty()) {
    ftp.insert(
        {features::kIdentifiabilityStudyBlockedTypes.name,
         EncodeIdentifiabilityFieldTrialParam(parameters.blocked_types)});
  }

  ftp.insert({features::kIdentifiabilityStudySurfaceSelectionRate.name,
              base::NumberToString(parameters.surface_selection_rate)});

  if (parameters.max_surfaces != defaults.max_surfaces) {
    ftp.insert({features::kIdentifiabilityStudyMaxSurfaces.name,
                base::NumberToString(parameters.max_surfaces)});
  }
  if (!parameters.per_surface_sampling_rate.empty()) {
    ftp.insert({features::kIdentifiabilityStudyPerSurfaceSettings.name,
                EncodeIdentifiabilityFieldTrialParam(
                    parameters.per_surface_sampling_rate)});
  }
  if (!parameters.per_type_sampling_rate.empty()) {
    ftp.insert({features::kIdentifiabilityStudyPerTypeSettings.name,
                EncodeIdentifiabilityFieldTrialParam(
                    parameters.per_type_sampling_rate)});
  }
  if (!parameters.per_surface_cost.empty()) {
    ftp.insert(
        {features::kIdentifiabilityStudyPerHashCost.name,
         EncodeIdentifiabilityFieldTrialParam(parameters.per_surface_cost)});
  }
  if (!parameters.per_type_cost.empty()) {
    ftp.insert(
        {features::kIdentifiabilityStudyPerTypeCost.name,
         EncodeIdentifiabilityFieldTrialParam(parameters.per_type_cost)});
  }
  if (!parameters.equivalence_classes.empty()) {
    ftp.insert(
        {features::kIdentifiabilityStudySurfaceEquivalenceClasses.name,
         EncodeIdentifiabilityFieldTrialParam(parameters.equivalence_classes)});
  }

  scoped_feature_list_.InitAndEnableFeatureWithParameters(
      features::kIdentifiabilityStudy, ftp);
}

}  // namespace test
