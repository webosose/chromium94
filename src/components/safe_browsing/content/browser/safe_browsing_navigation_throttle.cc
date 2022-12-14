// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/content/browser/safe_browsing_navigation_throttle.h"

#include "base/memory/ptr_util.h"
#include "components/safe_browsing/content/browser/safe_browsing_blocking_page.h"
#include "components/safe_browsing/content/browser/safe_browsing_blocking_page_factory.h"
#include "components/safe_browsing/content/browser/ui_manager.h"
#include "components/security_interstitials/content/security_interstitial_tab_helper.h"
#include "components/security_interstitials/core/unsafe_resource.h"
#include "content/public/browser/navigation_handle.h"

namespace safe_browsing {

SafeBrowsingNavigationThrottle::SafeBrowsingNavigationThrottle(
    content::NavigationHandle* handle,
    SafeBrowsingUIManager* manager)
    : content::NavigationThrottle(handle), manager_(manager) {}

const char* SafeBrowsingNavigationThrottle::GetNameForLogging() {
  return "SafeBrowsingNavigationThrottle";
}

content::NavigationThrottle::ThrottleCheckResult
SafeBrowsingNavigationThrottle::WillFailRequest() {
  if (!manager_) {
    return content::NavigationThrottle::PROCEED;
  }

  security_interstitials::UnsafeResource resource;
  content::NavigationHandle* handle = navigation_handle();

  if (manager_->PopUnsafeResourceForURL(handle->GetURL(), &resource)) {
    SafeBrowsingBlockingPage* blocking_page =
        manager_->blocking_page_factory()->CreateSafeBrowsingPage(
            manager_, handle->GetWebContents(), handle->GetURL(), {resource},
            true);
    manager_->ForwardSecurityInterstitialShownExtensionEventToEmbedder(
        handle->GetWebContents(), handle->GetURL(),
        SafeBrowsingUIManager::GetThreatTypeStringForInterstitial(
            resource.threat_type),
        /*net_error_code=*/0);
    std::string error_page_content = blocking_page->GetHTMLContents();
    security_interstitials::SecurityInterstitialTabHelper::
        AssociateBlockingPage(handle->GetWebContents(),
                              handle->GetNavigationId(),
                              base::WrapUnique(blocking_page));

    return content::NavigationThrottle::ThrottleCheckResult(
        CANCEL, net::ERR_BLOCKED_BY_CLIENT, error_page_content);
  }

  return content::NavigationThrottle::PROCEED;
}

}  // namespace safe_browsing
