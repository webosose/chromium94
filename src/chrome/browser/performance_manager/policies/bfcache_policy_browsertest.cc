// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/memory/memory_pressure_monitor.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "base/util/memory_pressure/fake_memory_pressure_monitor.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/performance_manager/public/features.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/visibility.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace performance_manager {
namespace policies {

namespace {

struct PolicyTestParams {
  const std::string scenario;
  bool enable_policy = false;
  bool enable_trim_on_moderate_pressure = false;
};

class BFCachePolicyBrowserTest
    : public InProcessBrowserTest,
      public ::testing::WithParamInterface<PolicyTestParams> {
 public:
  ~BFCachePolicyBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    EnableFeature(::features::kBackForwardCache,
                  {{"TimeToLiveInBackForwardCacheInSeconds", "3600"},
                   {"ignore_outstanding_network_request_for_testing", "true"}});
    DisableFeature(::features::kBackForwardCacheMemoryControls);

    if (GetParam().enable_policy) {
      EnableFeature(
          performance_manager::features::kBFCachePerformanceManagerPolicy,
          {{"flush_on_moderate_pressure",
            GetParam().enable_trim_on_moderate_pressure ? "true" : "false"}});
    } else {
      DisableFeature(
          performance_manager::features::kBFCachePerformanceManagerPolicy);
    }

    feature_list_.InitWithFeaturesAndParameters(enabled_features_,
                                                disabled_features_);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());
  }

 protected:
  void EnableFeature(const base::Feature& feature,
                     const base::FieldTrialParams& params) {
    enabled_features_.emplace_back(feature, params);
  }

  void DisableFeature(const base::Feature& feature) {
    disabled_features_.emplace_back(feature);
  }

  content::WebContents* web_contents() const {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* top_frame_host() {
    return web_contents()->GetMainFrame();
  }

  void VerifyEvictionExpectation(bool should_be_evicted,
                                 content::RenderFrameHostWrapper& rfh) {
    if (should_be_evicted) {
      // When the page is evicted the RenderFrame will be deleted.
      rfh.WaitUntilRenderFrameDeleted();
    } else {
      EXPECT_EQ(rfh->GetLifecycleState(),
                content::RenderFrameHost::LifecycleState::kInBackForwardCache);
    }
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features_;
  std::vector<base::Feature> disabled_features_;
};

}  // namespace

IN_PROC_BROWSER_TEST_P(BFCachePolicyBrowserTest, CacheFlushed) {
  util::test::FakeMemoryPressureMonitor fake_memory_pressure_monitor;
  const GURL url_a(embedded_test_server()->GetURL("a.com", "/title1.html"));
  const GURL url_b(embedded_test_server()->GetURL("b.com", "/title1.html"));

  EXPECT_EQ(web_contents()->GetVisibility(), content::Visibility::VISIBLE);

  // Navigate to A.
  EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url_a));
  content::RenderFrameHostWrapper rfh_a(top_frame_host());

  // Navigate to B.
  EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  content::RenderFrameHostWrapper rfh_b(top_frame_host());

  // Ensure A is cached.
  EXPECT_EQ(rfh_a->GetLifecycleState(),
            content::RenderFrameHost::LifecycleState::kInBackForwardCache);

  if (GetParam().scenario == "FlushWhenTabBackgrounded") {
    // Backgrounding the page will evict it from BFCache.
    web_contents()->WasHidden();
    VerifyEvictionExpectation(
        /* should_be_evicted = */ GetParam().enable_policy, rfh_a);
  } else if (GetParam().scenario == "FlushWhenTabBackgroundDuringNavigation") {
    web_contents()->GetController().GoBack();
    // Make the tab backgrounded before the back navigation completes. |rfh_a|
    // will become the active frame and the cache will be flushed (i.e. |rfh_b|
    // will be deleted).
    web_contents()->WasHidden();
    EXPECT_TRUE(WaitForLoadStop(web_contents()));
    EXPECT_EQ(rfh_a.get(), top_frame_host());
    VerifyEvictionExpectation(
        /* should_be_evicted = */ GetParam().enable_policy, rfh_b);
  } else if (GetParam().scenario == "FlushOnModerateMemoryPressure") {
    // A moderate memory pressure signal will evict the page from BFCache.
    fake_memory_pressure_monitor.SetAndNotifyMemoryPressure(
        base::MemoryPressureListener::MemoryPressureLevel::
            MEMORY_PRESSURE_LEVEL_MODERATE);
    VerifyEvictionExpectation(
        /* should_be_evicted = */ (GetParam().enable_policy &&
                                   GetParam().enable_trim_on_moderate_pressure),
        rfh_a);
  } else if (GetParam().scenario == "FlushOnCriticalMemoryPressure") {
    // A critical memory pressure signal will evict the page from BFCache.
    fake_memory_pressure_monitor.SetAndNotifyMemoryPressure(
        base::MemoryPressureListener::MemoryPressureLevel::
            MEMORY_PRESSURE_LEVEL_CRITICAL);
    VerifyEvictionExpectation(
        /* should_be_evicted = */ GetParam().enable_policy, rfh_a);
  }
}

std::vector<PolicyTestParams> BFCachePolicyBrowserTestValues() {
  const std::vector<std::string> kTestScenarios = {
      "FlushWhenTabBackgrounded", "FlushWhenTabBackgroundDuringNavigation",
      "FlushOnModerateMemoryPressure", "FlushOnCriticalMemoryPressure"};
  std::vector<PolicyTestParams> test_cases;

  for (const auto& test_scenario : kTestScenarios) {
    test_cases.push_back({.scenario = test_scenario, .enable_policy = false});

    // Test the policy with |flush_on_moderate_pressure| enabled.
    test_cases.push_back({.scenario = test_scenario,
                          .enable_policy = true,
                          .enable_trim_on_moderate_pressure = true});

    // Test the policy with |flush_on_moderate_pressure| disabled.
    test_cases.push_back({.scenario = test_scenario,
                          .enable_policy = true,
                          .enable_trim_on_moderate_pressure = false});
  }

  return test_cases;
}

INSTANTIATE_TEST_SUITE_P(All,
                         BFCachePolicyBrowserTest,
                         testing::ValuesIn(BFCachePolicyBrowserTestValues()));

}  // namespace policies
}  // namespace performance_manager
