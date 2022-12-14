// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/media_router/media_router_integration_browsertest.h"

#include "base/files/file_util.h"
#include "build/build_config.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace media_router {

// TODO(https://crbug.com/822231): Flaky in Chromium waterfall.
IN_PROC_BROWSER_TEST_F(MediaRouterIntegrationBrowserTest, MANUAL_Dialog_Basic) {
  OpenTestPage(FILE_PATH_LITERAL("basic_test.html"));
  test_ui_->ShowCastDialog();
  test_ui_->WaitForSinkAvailable(receiver_);
  test_ui_->StartCastingFromCastDialog(receiver_);
  test_ui_->WaitForAnyRoute();

  if (!test_ui_->IsCastDialogShown())
    test_ui_->ShowCastDialog();

  ASSERT_EQ("Test Route", test_ui_->GetStatusTextForSink(receiver_));

  test_ui_->StopCastingFromCastDialog(receiver_);
  test_ui_->WaitUntilNoRoutes();
  // TODO(takumif): Remove the HideCastDialog() call once the dialog can close
  // on its own.
  test_ui_->HideCastDialog();
}

// TODO(https://crbug.com/822231): Flaky in Chromium waterfall.
IN_PROC_BROWSER_TEST_F(MediaRouterIntegrationBrowserTest,
                       MANUAL_Dialog_RouteCreationTimedOut) {
  // The hardcoded timeout route creation timeout for the UI.
  // See kCreateRouteTimeoutSeconds in media_router_ui.cc.
  test_provider_->set_delay(base::TimeDelta::FromSeconds(20));
  OpenTestPage(FILE_PATH_LITERAL("basic_test.html"));
  test_ui_->ShowCastDialog();
  test_ui_->WaitForSinkAvailable(receiver_);

  base::TimeTicks start_time(base::TimeTicks::Now());
  test_ui_->StartCastingFromCastDialog(receiver_);
  test_ui_->WaitForAnyIssue();

  base::TimeDelta elapsed(base::TimeTicks::Now() - start_time);
  base::TimeDelta expected_timeout(base::TimeDelta::FromSeconds(20));

  EXPECT_GE(elapsed, expected_timeout);
  EXPECT_LE(elapsed - expected_timeout, base::TimeDelta::FromSeconds(5));

  std::string issue_title = test_ui_->GetIssueTextForSink(receiver_);
  // TODO(imcheng): Fix host name for file schemes (crbug.com/560576).
  ASSERT_EQ(l10n_util::GetStringFUTF8(
                IDS_MEDIA_ROUTER_ISSUE_CREATE_ROUTE_TIMEOUT, u"file:///"),
            issue_title);

  ASSERT_EQ(test_ui_->GetRouteIdForSink(receiver_), "");
  test_ui_->HideCastDialog();
}

IN_PROC_BROWSER_TEST_F(MediaRouterIntegrationBrowserTest,
                       PRE_OpenDialogAfterEnablingMediaRouting) {
  SetEnableMediaRouter(false);
}

IN_PROC_BROWSER_TEST_F(MediaRouterIntegrationBrowserTest,
                       OpenDialogAfterEnablingMediaRouting) {
  // Enable media routing and open media router dialog.
  SetEnableMediaRouter(true);
  OpenTestPage(FILE_PATH_LITERAL("basic_test.html"));
  test_ui_->ShowCastDialog();
  ASSERT_TRUE(test_ui_->IsCastDialogShown());
  test_ui_->HideCastDialog();
}

IN_PROC_BROWSER_TEST_F(MediaRouterIntegrationBrowserTest,
                       DisableMediaRoutingWhenDialogIsOpened) {
  // Open media router dialog.
  OpenTestPage(FILE_PATH_LITERAL("basic_test.html"));
  test_ui_->ShowCastDialog();
  ASSERT_TRUE(test_ui_->IsCastDialogShown());

  // Disable media routing.
  SetEnableMediaRouter(false);

  ASSERT_FALSE(test_ui_->IsCastDialogShown());
}

}  // namespace media_router
