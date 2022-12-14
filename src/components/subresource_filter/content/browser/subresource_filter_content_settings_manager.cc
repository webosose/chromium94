// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/browser/subresource_filter_content_settings_manager.h"

#include <string>

#include "base/bind.h"
#include "base/check.h"
#include "base/time/default_clock.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_constraints.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "url/gurl.h"

namespace subresource_filter {

namespace {

// Key into the website setting dict for the smart UI.
const char kInfobarLastShownTimeKey[] = "InfobarLastShownTime";
const char kActivatedKey[] = "Activated";
const char kNonRenewingExpiryTime[] = "NonRenewingExpiryTime";

bool ShouldUseSmartUI() {
#if defined(OS_ANDROID)
  return true;
#endif
  return false;
}

}  // namespace

constexpr base::TimeDelta
    SubresourceFilterContentSettingsManager::kDelayBeforeShowingInfobarAgain;

constexpr base::TimeDelta
    SubresourceFilterContentSettingsManager::kMaxPersistMetadataDuration;

SubresourceFilterContentSettingsManager::
    SubresourceFilterContentSettingsManager(
        HostContentSettingsMap* settings_map)
    : settings_map_(settings_map),
      clock_(std::make_unique<base::DefaultClock>(base::DefaultClock())),
      should_use_smart_ui_(ShouldUseSmartUI()) {
  DCHECK(settings_map_);
}

SubresourceFilterContentSettingsManager::
    ~SubresourceFilterContentSettingsManager() = default;

ContentSetting SubresourceFilterContentSettingsManager::GetSitePermission(
    const GURL& url) const {
  return settings_map_->GetContentSetting(url, GURL(),
                                          ContentSettingsType::ADS);
}

void SubresourceFilterContentSettingsManager::AllowlistSite(const GURL& url) {
  settings_map_->SetContentSettingDefaultScope(
      url, GURL(), ContentSettingsType::ADS, CONTENT_SETTING_ALLOW);
}

void SubresourceFilterContentSettingsManager::OnDidShowUI(const GURL& url) {
  std::unique_ptr<base::DictionaryValue> dict = GetSiteMetadata(url);
  if (!dict)
    dict = CreateMetadataDictWithActivation(true /* is_activated */);

  double now = clock_->Now().ToDoubleT();
  dict->SetDouble(kInfobarLastShownTimeKey, now);
  SetSiteMetadata(url, std::move(dict));
}

bool SubresourceFilterContentSettingsManager::ShouldShowUIForSite(
    const GURL& url) const {
  if (!should_use_smart_ui())
    return true;

  std::unique_ptr<base::DictionaryValue> dict = GetSiteMetadata(url);
  if (!dict)
    return true;

  double last_shown_time_double = 0;
  if (dict->GetDouble(kInfobarLastShownTimeKey, &last_shown_time_double)) {
    base::Time last_shown = base::Time::FromDoubleT(last_shown_time_double);
    if (clock_->Now() - last_shown < kDelayBeforeShowingInfobarAgain)
      return false;
  }
  return true;
}

void SubresourceFilterContentSettingsManager::SetSiteMetadataBasedOnActivation(
    const GURL& url,
    bool is_activated,
    ActivationSource activation_source,
    std::unique_ptr<base::DictionaryValue> additional_data) {
  std::unique_ptr<base::DictionaryValue> dict = GetSiteMetadata(url);

  if (!is_activated &&
      ShouldDeleteDataWithNoActivation(dict.get(), activation_source)) {
    // If we are clearing metadata, there should be no additional_data dict.
    DCHECK(!additional_data);
    SetSiteMetadata(url, nullptr);
    return;
  }

  // Do not create new metadata if it exists already, it could clobber
  // existing data.
  if (!dict)
    dict = CreateMetadataDictWithActivation(is_activated /* is_activated */);
  else
    dict->SetBoolKey(kActivatedKey, is_activated);

  if (additional_data)
    dict->MergeDictionary(additional_data.get());

  // Ads intervention metadata should not be deleted by changes in activation
  // during the metrics collection period (kMaxPersistMetadataDuration).
  // Setting the key kNonRenewingExpiryTime enforces this behavior in
  // SetSiteMetadata.
  if (activation_source == ActivationSource::kAdsIntervention) {
    // If we have an expiry time set, then we are already tracking
    // an ads intervention. Since we should not be able to trigger a new ads
    // intervention once we should be blocking ads, do not change the expiry
    // time or overwrite existing ads intervention metadata,
    if (dict->FindDoubleKey(kNonRenewingExpiryTime))
      return;
    double expiry_time =
        (clock_->Now() + kMaxPersistMetadataDuration).ToDoubleT();
    dict->SetDoubleKey(kNonRenewingExpiryTime, expiry_time);
  }

  SetSiteMetadata(url, std::move(dict));
}

std::unique_ptr<base::DictionaryValue>
SubresourceFilterContentSettingsManager::GetSiteMetadata(
    const GURL& url) const {
  return base::DictionaryValue::From(settings_map_->GetWebsiteSetting(
      url, GURL(), ContentSettingsType::ADS_DATA, nullptr));
}

void SubresourceFilterContentSettingsManager::SetSiteMetadataForTesting(
    const GURL& url,
    std::unique_ptr<base::DictionaryValue> dict) {
  SetSiteMetadata(url, std::move(dict));
}

void SubresourceFilterContentSettingsManager::SetSiteMetadata(
    const GURL& url,
    std::unique_ptr<base::DictionaryValue> dict) {
  if (url.is_empty())
    return;

  // Metadata expires after kMaxPersistMetadataDuration by default. If
  // kNonRenewingExpiryTime was previously set, then we are storing ads
  // intervention metadata and should not override the expiry time that
  // was previously set.
  base::Time expiry_time = base::Time::Now() + kMaxPersistMetadataDuration;
  if (dict && dict->HasKey(kNonRenewingExpiryTime)) {
    absl::optional<double> metadata_expiry_time =
        dict->FindDoubleKey(kNonRenewingExpiryTime);
    DCHECK(metadata_expiry_time);
    expiry_time = base::Time::FromDoubleT(*metadata_expiry_time);
  }

  content_settings::ContentSettingConstraints constraints = {expiry_time};
  settings_map_->SetWebsiteSettingDefaultScope(
      url, GURL(), ContentSettingsType::ADS_DATA, std::move(dict), constraints);
}

std::unique_ptr<base::DictionaryValue>
SubresourceFilterContentSettingsManager::CreateMetadataDictWithActivation(
    bool is_activated) {
  auto dict = std::make_unique<base::DictionaryValue>();
  dict->SetBoolKey(kActivatedKey, is_activated);

  return dict;
}

bool SubresourceFilterContentSettingsManager::ShouldDeleteDataWithNoActivation(
    base::DictionaryValue* dict,
    ActivationSource activation_source) {
  // For the ads intervention dry run experiment we want to make sure that
  // non activated pages get properly tagged for metrics collection. Don't
  // delete them from storage until their associated intervention _would have_
  // expired.
  if (activation_source != ActivationSource::kSafeBrowsing)
    return false;

  if (!dict)
    return true;

  absl::optional<double> metadata_expiry_time =
      dict->FindDoubleKey(kNonRenewingExpiryTime);

  if (!metadata_expiry_time)
    return true;

  base::Time expiry_time = base::Time::FromDoubleT(*metadata_expiry_time);
  return clock_->Now() > expiry_time;
}

bool SubresourceFilterContentSettingsManager::GetSiteActivationFromMetadata(
    const GURL& url) {
  std::unique_ptr<base::DictionaryValue> dict = GetSiteMetadata(url);

  // If there is no dict, this is metadata V1, absence of metadata
  // implies no activation.
  if (!dict)
    return false;

  absl::optional<bool> site_activation_status =
      dict->FindBoolKey(kActivatedKey);

  // If there is no explicit site activation status, it is metadata V1:
  // use the presence of metadata as indicative of the site activation.
  // Otherwise it is metadata V2, we return the activation stored in
  // kActivatedKey.
  return !site_activation_status || *site_activation_status;
}

void SubresourceFilterContentSettingsManager::ClearSiteMetadata(
    const GURL& url) {
  SetSiteMetadata(url, nullptr);
}

void SubresourceFilterContentSettingsManager::ClearMetadataForAllSites() {
  settings_map_->ClearSettingsForOneType(ContentSettingsType::ADS_DATA);
}

}  // namespace subresource_filter
