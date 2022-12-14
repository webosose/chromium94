// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/site_isolation/site_isolation_policy.h"

#include "base/containers/contains.h"
#include "base/json/values_util.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_functions.h"
#include "base/system/sys_info.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/site_isolation/features.h"
#include "components/site_isolation/pref_names.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/site_isolation_policy.h"
#include "content/public/common/content_features.h"

namespace site_isolation {

namespace {

using IsolatedOriginSource =
    content::ChildProcessSecurityPolicy::IsolatedOriginSource;

}  // namespace

// static
bool SiteIsolationPolicy::IsIsolationForPasswordSitesEnabled() {
  // If the user has explicitly enabled site isolation for password sites from
  // chrome://flags or from the command line, honor this regardless of policies
  // that may disable site isolation.  In particular, this means that the
  // chrome://flags switch for this feature takes precedence over any memory
  // threshold restrictions and over a switch for disabling site isolation.
  if (base::FeatureList::GetInstance()->IsFeatureOverriddenFromCommandLine(
          features::kSiteIsolationForPasswordSites.name,
          base::FeatureList::OVERRIDE_ENABLE_FEATURE)) {
    return true;
  }

  // Don't isolate anything when site isolation is turned off by the user or
  // policy. This includes things like the switches::kDisableSiteIsolation
  // command-line switch, the corresponding "Disable site isolation" entry in
  // chrome://flags, enterprise policy controlled via
  // switches::kDisableSiteIsolationForPolicy, and memory threshold checks in
  // ShouldDisableSiteIsolationDueToMemoryThreshold().
  if (!content::SiteIsolationPolicy::AreDynamicIsolatedOriginsEnabled())
    return false;

  // The feature needs to be checked last, because checking the feature
  // activates the field trial and assigns the client either to a control or an
  // experiment group - such assignment should be final.
  return base::FeatureList::IsEnabled(features::kSiteIsolationForPasswordSites);
}

// static
bool SiteIsolationPolicy::IsIsolationForOAuthSitesEnabled() {
  // If the user has explicitly enabled site isolation for OAuth sites from the
  // command line, honor this regardless of policies that may disable site
  // isolation.
  if (base::FeatureList::GetInstance()->IsFeatureOverriddenFromCommandLine(
          features::kSiteIsolationForOAuthSites.name,
          base::FeatureList::OVERRIDE_ENABLE_FEATURE)) {
    return true;
  }

  // Don't isolate anything when site isolation is turned off by the user or
  // policy. This includes things like the switches::kDisableSiteIsolation
  // command-line switch, the corresponding "Disable site isolation" entry in
  // chrome://flags, enterprise policy controlled via
  // switches::kDisableSiteIsolationForPolicy, and memory threshold checks in
  // ShouldDisableSiteIsolationDueToMemoryThreshold().
  if (!content::SiteIsolationPolicy::AreDynamicIsolatedOriginsEnabled())
    return false;

  // The feature needs to be checked last, because checking the feature
  // activates the field trial and assigns the client either to a control or an
  // experiment group - such assignment should be final.
  return base::FeatureList::IsEnabled(features::kSiteIsolationForOAuthSites);
}

// static
bool SiteIsolationPolicy::IsEnterprisePolicyApplicable() {
#if defined(OS_ANDROID)
  // https://crbug.com/844118: Limiting policy to devices with > 1GB RAM.
  // Using 1077 rather than 1024 because 1) it helps ensure that devices with
  // exactly 1GB of RAM won't get included because of inaccuracies or off-by-one
  // errors and 2) this is the bucket boundary in Memory.Stats.Win.TotalPhys2.
  bool have_enough_memory = base::SysInfo::AmountOfPhysicalMemoryMB() > 1077;
  return have_enough_memory;
#else
  return true;
#endif
}

// static
bool SiteIsolationPolicy::ShouldDisableSiteIsolationDueToMemoryThreshold() {
  // The memory threshold behavior differs for desktop and Android:
  // - Android uses a 1900MB default threshold, which is the threshold used by
  //   password-triggered site isolation - see docs in
  //   https://crbug.com/849815.  This can be overridden via a param defined in
  //   a kSitePerProcessOnlyForHighMemoryClients field trial.
  // - Desktop does not enforce a default memory threshold, but for now we
  //   still support a threshold defined via a
  //   kSitePerProcessOnlyForHighMemoryClients field trial.  The trial
  //   typically carries the threshold in a param; if it doesn't, use a default
  //   that's slightly higher than 1GB (see https://crbug.com/844118).
  //
  // TODO(alexmos): currently, this threshold applies to all site isolation
  // modes.  Eventually, we may need separate thresholds for different modes,
  // such as full site isolation vs. password-triggered site isolation.
#if defined(OS_ANDROID)
  constexpr int kDefaultMemoryThresholdMb = 1900;
#else
  constexpr int kDefaultMemoryThresholdMb = 1077;
#endif

  // TODO(acolwell): Rename feature since it now affects more than just the
  // site-per-process case.
  if (base::FeatureList::IsEnabled(
          features::kSitePerProcessOnlyForHighMemoryClients)) {
    int memory_threshold_mb = base::GetFieldTrialParamByFeatureAsInt(
        features::kSitePerProcessOnlyForHighMemoryClients,
        features::kSitePerProcessOnlyForHighMemoryClientsParamName,
        kDefaultMemoryThresholdMb);
    return base::SysInfo::AmountOfPhysicalMemoryMB() <= memory_threshold_mb;
  }

#if defined(OS_ANDROID)
  if (base::SysInfo::AmountOfPhysicalMemoryMB() <= kDefaultMemoryThresholdMb) {
    return true;
  }
#endif

  return false;
}

// static
void SiteIsolationPolicy::PersistIsolatedOrigin(
    content::BrowserContext* context,
    const url::Origin& origin,
    IsolatedOriginSource source) {
  DCHECK(context);
  DCHECK(!context->IsOffTheRecord());
  DCHECK(!origin.opaque());

  // This function currently supports two sources for persistence, for
  // user-triggered and web-triggered isolated origins.
  if (source == IsolatedOriginSource::USER_TRIGGERED) {
    PersistUserTriggeredIsolatedOrigin(context, origin);
  } else if (source == IsolatedOriginSource::WEB_TRIGGERED) {
    PersistWebTriggeredIsolatedOrigin(context, origin);
  } else {
    NOTREACHED();
  }
}

// static
void SiteIsolationPolicy::PersistUserTriggeredIsolatedOrigin(
    content::BrowserContext* context,
    const url::Origin& origin) {
  // User-triggered isolated origins are currently stored in a simple list of
  // unlimited size.
  // TODO(alexmos): Cap the maximum number of entries and evict older entries.
  // See https://crbug.com/1172407.
  ListPrefUpdate update(user_prefs::UserPrefs::Get(context),
                        site_isolation::prefs::kUserTriggeredIsolatedOrigins);
  base::ListValue* list = update.Get();
  base::Value value(origin.Serialize());
  if (!base::Contains(list->GetList(), value))
    list->Append(std::move(value));
}

// static
void SiteIsolationPolicy::PersistWebTriggeredIsolatedOrigin(
    content::BrowserContext* context,
    const url::Origin& origin) {
  // Web-triggered isolated origins are stored in a dictionary of (origin,
  // timestamp) pairs.  The number of entries is capped by a field trial param,
  // and older entries are evicted.
  DictionaryPrefUpdate update(
      user_prefs::UserPrefs::Get(context),
      site_isolation::prefs::kWebTriggeredIsolatedOrigins);
  base::DictionaryValue* dict = update.Get();

  // Add the origin.  If it already exists, this will just update the
  // timestamp.
  dict->SetKey(origin.Serialize(), base::TimeToValue(base::Time::Now()));

  // Check whether the maximum number of stored sites was exceeded and remove
  // one or more entries, starting with the oldest timestamp. Note that more
  // than one entry may need to be removed, since the maximum number of entries
  // could change over time (via a change in the field trial param).
  size_t max_size =
      ::features::kSiteIsolationForCrossOriginOpenerPolicyMaxSitesParam.Get();
  while (dict->DictSize() > max_size) {
    auto items = dict->DictItems();
    auto oldest_site_time_pair = std::min_element(
        items.begin(), items.end(), [](auto pair_a, auto pair_b) {
          absl::optional<base::Time> time_a = base::ValueToTime(pair_a.second);
          absl::optional<base::Time> time_b = base::ValueToTime(pair_b.second);
          // has_value() should always be true unless the prefs were corrupted.
          // In that case, prioritize the corrupted entry for removal.
          return (time_a.has_value() ? time_a.value() : base::Time::Min()) <
                 (time_b.has_value() ? time_b.value() : base::Time::Min());
        });
    dict->RemoveKey(oldest_site_time_pair->first);
  }
}

// static
void SiteIsolationPolicy::ApplyPersistedIsolatedOrigins(
    content::BrowserContext* browser_context) {
  auto* policy = content::ChildProcessSecurityPolicy::GetInstance();

  // If the user turned off password-triggered isolation, don't apply any
  // stored isolated origins, but also don't clear them from prefs, so that
  // they can be used if password-triggered isolation is re-enabled later.
  if (IsIsolationForPasswordSitesEnabled()) {
    std::vector<url::Origin> origins;
    for (const auto& value : user_prefs::UserPrefs::Get(browser_context)
                                 ->GetList(prefs::kUserTriggeredIsolatedOrigins)
                                 ->GetList()) {
      origins.push_back(url::Origin::Create(GURL(value.GetString())));
    }

    if (!origins.empty()) {
      policy->AddFutureIsolatedOrigins(
          origins, IsolatedOriginSource::USER_TRIGGERED, browser_context);
    }

    base::UmaHistogramCounts1000(
        "SiteIsolation.SavedUserTriggeredIsolatedOrigins.Size", origins.size());
  }

  // Similarly, load saved web-triggered isolated origins only if isolation of
  // COOP sites (currently the only source of these origins) is enabled with
  // persistence, but don't remove them from prefs otherwise.
  if (content::SiteIsolationPolicy::ShouldPersistIsolatedCOOPSites()) {
    std::vector<url::Origin> origins;
    std::vector<std::string> expired_entries;

    auto* pref_service = user_prefs::UserPrefs::Get(browser_context);
    auto* dict =
        pref_service->GetDictionary(prefs::kWebTriggeredIsolatedOrigins);
    if (dict) {
      for (auto site_time_pair : dict->DictItems()) {
        // Only isolate origins that haven't expired.
        absl::optional<base::Time> timestamp =
            base::ValueToTime(site_time_pair.second);
        base::TimeDelta expiration_timeout =
            ::features::
                kSiteIsolationForCrossOriginOpenerPolicyExpirationTimeoutParam
                    .Get();
        if (timestamp.has_value() &&
            base::Time::Now() - timestamp.value() <= expiration_timeout) {
          origins.push_back(url::Origin::Create(GURL(site_time_pair.first)));
        } else {
          expired_entries.push_back(site_time_pair.first);
        }
      }
      // Remove expired entries (as well as those with an invalid timestamp).
      if (!expired_entries.empty()) {
        DictionaryPrefUpdate update(pref_service,
                                    prefs::kWebTriggeredIsolatedOrigins);
        base::DictionaryValue* updated_dict = update.Get();
        for (const auto& entry : expired_entries)
          updated_dict->RemoveKey(entry);
      }
    }

    if (!origins.empty()) {
      policy->AddFutureIsolatedOrigins(
          origins, IsolatedOriginSource::WEB_TRIGGERED, browser_context);
    }

    base::UmaHistogramCounts100(
        "SiteIsolation.SavedWebTriggeredIsolatedOrigins.Size", origins.size());
  }
}

// static
void SiteIsolationPolicy::IsolateStoredOAuthSites(
    content::BrowserContext* browser_context,
    const std::vector<url::Origin>& logged_in_sites) {
  // Only isolate logged-in sites if the corresponding feature is enabled and
  // other isolation requirements (such as memory threshold) are satisfied.
  // Note that we don't clear logged-in sites from prefs if site isolation is
  // disabled so that they can be used if isolation is re-enabled later.
  if (!IsIsolationForOAuthSitesEnabled())
    return;

  auto* policy = content::ChildProcessSecurityPolicy::GetInstance();
  policy->AddFutureIsolatedOrigins(
      logged_in_sites,
      content::ChildProcessSecurityPolicy::IsolatedOriginSource::USER_TRIGGERED,
      browser_context);

  // Note that the max count matches
  // login_detection::GetOauthLoggedInSitesMaxSize().
  base::UmaHistogramCounts100("SiteIsolation.SavedOAuthSites.Size",
                              logged_in_sites.size());
}

// static
void SiteIsolationPolicy::IsolateNewOAuthURL(
    content::BrowserContext* browser_context,
    const GURL& signed_in_url) {
  if (!IsIsolationForOAuthSitesEnabled())
    return;

  // OAuth information is currently persisted and restored by other layers. See
  // login_detection::prefs::SaveSiteToOAuthSignedInList().
  constexpr bool kShouldPersist = false;

  content::SiteInstance::StartIsolatingSite(
      browser_context, signed_in_url,
      content::ChildProcessSecurityPolicy::IsolatedOriginSource::USER_TRIGGERED,
      kShouldPersist);
}

// static
bool SiteIsolationPolicy::ShouldPdfCompositorBeEnabledForOopifs() {
  // We only create pdf compositor client and use pdf compositor service when
  // one of the site isolation modes that forces OOPIFs is on. This includes
  // full site isolation on desktop, password-triggered site isolation on
  // Android for high-memory devices, and/or isolated origins specified via
  // command line, enterprise policy, or field trials.
  //
  // TODO(weili, thestig): Eventually, we should remove this check and use pdf
  // compositor service by default for printing.
  return content::SiteIsolationPolicy::UseDedicatedProcessesForAllSites() ||
         IsIsolationForPasswordSitesEnabled() ||
         content::SiteIsolationPolicy::AreIsolatedOriginsEnabled();
}

}  // namespace site_isolation
