// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/user_education/feature_promo_bubble_view.h"

#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/browser_app_menu_button.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "content/public/test/browser_test.h"
#include "ui/events/base_event_utils.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/test/widget_test.h"

class FeaturePromoBubbleViewInteractiveTest : public InProcessBrowserTest {
 public:
  FeaturePromoBubbleViewInteractiveTest() = default;
  ~FeaturePromoBubbleViewInteractiveTest() override = default;

 protected:
  FeaturePromoBubbleView::CreateParams GetBubbleParams() {
    FeaturePromoBubbleView::CreateParams params;
    params.body_text = u"To X, do Y";
    params.anchor_view = BrowserView::GetBrowserViewForBrowser(browser())
                             ->toolbar()
                             ->app_menu_button();
    params.arrow = views::BubbleBorder::TOP_RIGHT;
    params.timeout_no_interaction = absl::nullopt;
    params.timeout_after_interaction = absl::nullopt;
    return params;
  }
};

IN_PROC_BROWSER_TEST_F(FeaturePromoBubbleViewInteractiveTest,
                       WidgetNotActivatedByDefault) {
  auto params = GetBubbleParams();
  params.focus_on_create = false;

  auto* const browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* const focus_manager = browser_view->GetWidget()->GetFocusManager();
  EXPECT_TRUE(browser_view->GetWidget()->IsActive());

  browser_view->FocusToolbar();
  views::View* const initial_focused_view = focus_manager->GetFocusedView();
  EXPECT_NE(nullptr, initial_focused_view);

  auto* const bubble = FeaturePromoBubbleView::Create(std::move(params));
  views::test::WidgetVisibleWaiter(bubble->GetWidget()).Wait();

  EXPECT_TRUE(browser_view->GetWidget()->IsActive());
  EXPECT_FALSE(bubble->GetWidget()->IsActive());
}

IN_PROC_BROWSER_TEST_F(FeaturePromoBubbleViewInteractiveTest,
                       WidgetActivatedWhenRequested) {
  auto params = GetBubbleParams();
  params.focus_on_create = true;

  auto* const browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  EXPECT_TRUE(browser_view->GetWidget()->IsActive());

  auto* const bubble = FeaturePromoBubbleView::Create(std::move(params));
  views::test::WidgetVisibleWaiter(bubble->GetWidget()).Wait();

  EXPECT_TRUE(bubble->GetWidget()->IsActive());

  // Browser view should lose activation.
  EXPECT_FALSE(browser_view->GetWidget()->IsActive());
}

namespace {
struct MockTimeoutTarget {
  int count = 0;

  void OnTimeout() { count++; }
};
}  // namespace

IN_PROC_BROWSER_TEST_F(FeaturePromoBubbleViewInteractiveTest,
                       FeaturePromoBubbleTimeout) {
  auto params = GetBubbleParams();

  base::TimeDelta interaction_time = base::TimeDelta::FromMilliseconds(1);
  params.timeout_no_interaction = interaction_time;
  params.timeout_after_interaction = interaction_time;

  MockTimeoutTarget timeout_target;
  params.timeout_callback = base::BindRepeating(
      &MockTimeoutTarget::OnTimeout, base::Unretained(&timeout_target));

  auto* const bubble = FeaturePromoBubbleView::Create(std::move(params));
  views::test::WidgetVisibleWaiter(bubble->GetWidget()).Wait();

  // Check that the timeout exists in the bubble.
  FeaturePromoBubbleTimeout* timeout = bubble->GetTimeoutForTesting();
  EXPECT_NE(nullptr, timeout);
  EXPECT_EQ(0, timeout_target.count);

  // Force the timeout codepath to be called
  timeout->OnTimeout();

  // Check that the timeout has occurred
  EXPECT_EQ(1, timeout_target.count);
}
