// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/optimization_guide/optimization_guide_web_contents_observer.h"

#include "chrome/browser/optimization_guide/chrome_hints_manager.h"
#include "chrome/browser/optimization_guide/optimization_guide_keyed_service.h"
#include "chrome/browser/optimization_guide/optimization_guide_keyed_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/optimization_guide/core/hints_fetcher.h"
#include "components/optimization_guide/core/hints_processing_util.h"
#include "components/optimization_guide/core/optimization_guide_enums.h"
#include "components/optimization_guide/core/optimization_guide_features.h"
#include "components/optimization_guide/proto/hints.pb.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace {

bool IsValidOptimizationGuideNavigation(
    content::NavigationHandle* navigation_handle) {
  // TODO(https://crbug.com/1218946): With MPArch there may be multiple main
  // frames. This caller was converted automatically to the primary main frame
  // to preserve its semantics. Follow up to confirm correctness.
  return navigation_handle->IsInPrimaryMainFrame() &&
         navigation_handle->GetURL().SchemeIsHTTPOrHTTPS();
}

}  // namespace

OptimizationGuideWebContentsObserver::OptimizationGuideWebContentsObserver(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  optimization_guide_keyed_service_ =
      OptimizationGuideKeyedServiceFactory::GetForProfile(
          Profile::FromBrowserContext(web_contents->GetBrowserContext()));
}

OptimizationGuideWebContentsObserver::~OptimizationGuideWebContentsObserver() =
    default;

OptimizationGuideNavigationData* OptimizationGuideWebContentsObserver::
    GetOrCreateOptimizationGuideNavigationData(
        content::NavigationHandle* navigation_handle) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_EQ(web_contents(), navigation_handle->GetWebContents());

  int64_t navigation_id = navigation_handle->GetNavigationId();
  if (inflight_optimization_guide_navigation_datas_.find(navigation_id) ==
      inflight_optimization_guide_navigation_datas_.end()) {
    // We do not have one already - create one.
    inflight_optimization_guide_navigation_datas_[navigation_id] =
        std::make_unique<OptimizationGuideNavigationData>(
            navigation_id, navigation_handle->NavigationStart());
  }

  DCHECK(inflight_optimization_guide_navigation_datas_.find(navigation_id) !=
         inflight_optimization_guide_navigation_datas_.end());
  return inflight_optimization_guide_navigation_datas_.find(navigation_id)
      ->second.get();
}

void OptimizationGuideWebContentsObserver::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Clear any leftover hint requests from a previous navigation.
  // TODO(https://crbug.com/1218946): With MPArch there may be multiple main
  // frames. This caller was converted automatically to the primary main frame
  // to preserve its semantics. Follow up to confirm correctness.
  if (navigation_handle->IsInPrimaryMainFrame()) {
    ClearHintsToFetchBasedOnPredictions(navigation_handle);
  }

  if (!IsValidOptimizationGuideNavigation(navigation_handle))
    return;

  if (!optimization_guide_keyed_service_)
    return;

  OptimizationGuideNavigationData* navigation_data =
      GetOrCreateOptimizationGuideNavigationData(navigation_handle);
  navigation_data->set_navigation_url(navigation_handle->GetURL());
  optimization_guide_keyed_service_->OnNavigationStartOrRedirect(
      navigation_data);
}

void OptimizationGuideWebContentsObserver::DidRedirectNavigation(
    content::NavigationHandle* navigation_handle) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!IsValidOptimizationGuideNavigation(navigation_handle))
    return;

  if (!optimization_guide_keyed_service_)
    return;

  OptimizationGuideNavigationData* navigation_data =
      GetOrCreateOptimizationGuideNavigationData(navigation_handle);
  navigation_data->set_navigation_url(navigation_handle->GetURL());
  optimization_guide_keyed_service_->OnNavigationStartOrRedirect(
      navigation_data);
}

void OptimizationGuideWebContentsObserver::ClearHintsToFetchBasedOnPredictions(
    content::NavigationHandle* navigation_handle) {
  hints_target_urls_.clear();
  sent_batched_hints_request_ = false;
}

void OptimizationGuideWebContentsObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Clear any leftover hint requests from a previous navigation.
  // TODO(https://crbug.com/1218946): With MPArch there may be multiple main
  // frames. This caller was converted automatically to the primary main frame
  // to preserve its semantics. Follow up to confirm correctness.
  if (navigation_handle->IsInPrimaryMainFrame()) {
    ClearHintsToFetchBasedOnPredictions(navigation_handle);
  }

  if (!IsValidOptimizationGuideNavigation(navigation_handle)) {
    return;
  }

  // Delete Optimization Guide information later, so that other
  // DidFinishNavigation methods can reliably use
  // GetOptimizationGuideNavigationData regardless of order of
  // WebContentsObservers.
  // Note that a lot of Navigations (sub-frames, same document, non-committed,
  // etc.) might not have navigation data associated with them, but we reduce
  // likelihood of future leaks by always trying to remove the data.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &OptimizationGuideWebContentsObserver::NotifyNavigationFinish,
          weak_factory_.GetWeakPtr(), navigation_handle->GetNavigationId(),
          navigation_handle->GetRedirectChain()));
}

void OptimizationGuideWebContentsObserver::DocumentOnLoadCompletedInMainFrame(
    content::RenderFrameHost* render_frame_host) {
  if (!render_frame_host->GetLastCommittedURL().SchemeIsHTTPOrHTTPS()) {
    return;
  }
  if (web_contents() !=
      content::WebContents::FromRenderFrameHost(render_frame_host)) {
    // The current web contents isn't for the main frame that reached onload.
    return;
  }

  if (!optimization_guide_keyed_service_) {
    return;
  }

  // Give the renderer some time to send us predictions that might have come
  // at onload.
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(
          &OptimizationGuideWebContentsObserver::FetchHintsUsingManager,
          weak_factory_.GetWeakPtr(),
          optimization_guide_keyed_service_->GetHintsManager()),
      optimization_guide::features::GetOnloadDelayForHintsFetching());
}

void OptimizationGuideWebContentsObserver::FetchHintsUsingManager(
    optimization_guide::ChromeHintsManager* hints_manager) {
  DCHECK(hints_manager);
  sent_batched_hints_request_ = true;
  hints_manager->FetchHintsForURLs(std::move(hints_target_urls_.vector()));
  hints_target_urls_.clear();
}

void OptimizationGuideWebContentsObserver::NotifyNavigationFinish(
    int64_t navigation_id,
    const std::vector<GURL>& navigation_redirect_chain) {
  auto nav_data_iter =
      inflight_optimization_guide_navigation_datas_.find(navigation_id);
  if (nav_data_iter == inflight_optimization_guide_navigation_datas_.end())
    return;

  if (optimization_guide_keyed_service_) {
    optimization_guide_keyed_service_->OnNavigationFinish(
        navigation_redirect_chain);
  }

  // We keep the last navigation data around to keep track of events happening
  // for the navigation that can happen after commit, such as a fetch for the
  // navigation successfully completing (which is not guaranteed to come back
  // before commit, if at all).
  last_navigation_data_ = std::move(nav_data_iter->second);
  inflight_optimization_guide_navigation_datas_.erase(navigation_id);
}

void OptimizationGuideWebContentsObserver::FlushLastNavigationData() {
  if (last_navigation_data_)
    last_navigation_data_.reset();
}

void OptimizationGuideWebContentsObserver::AddURLsToBatchFetchBasedOnPrediction(
    std::vector<GURL> urls,
    content::WebContents* web_contents) {
  if (!this->web_contents()) {
    return;
  }
  DCHECK_EQ(this->web_contents(), web_contents);
  if (sent_batched_hints_request_) {
    return;
  }
  for (const GURL& url : urls) {
    hints_target_urls_.insert(url);
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(OptimizationGuideWebContentsObserver)
