// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SITE_ISOLATION_SITE_ISOLATION_POLICY_H_
#define COMPONENTS_SITE_ISOLATION_SITE_ISOLATION_POLICY_H_

#include <vector>

#include "base/macros.h"
#include "content/public/browser/child_process_security_policy.h"

class GURL;

namespace content {
class BrowserContext;
}

namespace url {
class Origin;
}

namespace site_isolation {

// A centralized place for making policy decisions about site isolation modes
// which can be shared between content embedders. This supplements
// content::SiteIsolationPolicy with features that may be useful to embedders.
//
// These methods can be called from any thread.
class SiteIsolationPolicy {
 public:
  // Returns true if the site isolation mode for isolating sites where users
  // enter passwords is enabled.
  static bool IsIsolationForPasswordSitesEnabled();

  // Returns true if the site isolation mode for isolating sites where users
  // log in via OAuth, as determined by runtime heuristics.
  static bool IsIsolationForOAuthSitesEnabled();

  // Returns true if Site Isolation related enterprise policies should take
  // effect (e.g. such policies might not be applicable to low-end Android
  // devices because of 1) performance impact and 2) infeasibility of
  // Spectre-like attacks on such devices).
  static bool IsEnterprisePolicyApplicable();

  // Saves a new dynamic isolated origin to user prefs associated with
  // `context` so that it can be persisted across restarts. `source`
  // specifies why the isolated origin was added; different sources may have
  // different persistence policies.
  static void PersistIsolatedOrigin(
      content::BrowserContext* context,
      const url::Origin& origin,
      content::ChildProcessSecurityPolicy::IsolatedOriginSource source);

  // Reads and applies any isolated origins stored in user prefs associated with
  // |browser_context|.  This is expected to be called on startup after user
  // prefs have been loaded.
  static void ApplyPersistedIsolatedOrigins(
      content::BrowserContext* browser_context);

  // Helper to register all passed-in `logged_in_sites` as isolated sites in
  // the provided `browser_context`. Should be called on startup before any
  // navigations in `browser_context`.
  static void IsolateStoredOAuthSites(
      content::BrowserContext* browser_context,
      const std::vector<url::Origin>& logged_in_sites);

  // Called when runtime heuristics have determined a user logging in via
  // OAuth on `signed_in_url`, so that site isolation can be applied to the
  // corresponding site (i.e., scheme + eTLD+1).  Used only when site isolation
  // for OAuth sites is enabled (see IsIsolationForOAuthSitesEnabled() above),
  // which is typically on Android.
  static void IsolateNewOAuthURL(content::BrowserContext* browser_context,
                                 const GURL& signed_in_url);

  // Determines whether Site Isolation should be disabled because the device
  // does not have the minimum required amount of memory.
  //
  // TODO(alexmos): Currently, the memory threshold is shared for all site
  // isolation modes, including strict site isolation and password site
  // isolation.  In the future, some site isolation modes may require their own
  // memory threshold.
  static bool ShouldDisableSiteIsolationDueToMemoryThreshold();

  // Returns true if the PDF compositor should be enabled to allow out-of-
  // process iframes (OOPIF's) to print properly.
  static bool ShouldPdfCompositorBeEnabledForOopifs();

 private:
  // Helpers for implementing PersistIsolatedOrigin().
  static void PersistUserTriggeredIsolatedOrigin(
      content::BrowserContext* context,
      const url::Origin& origin);
  static void PersistWebTriggeredIsolatedOrigin(
      content::BrowserContext* context,
      const url::Origin& origin);

  DISALLOW_IMPLICIT_CONSTRUCTORS(SiteIsolationPolicy);
};

}  // namespace site_isolation

#endif  // COMPONENTS_SITE_ISOLATION_SITE_ISOLATION_POLICY_H_
