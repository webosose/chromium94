// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include "base/bind.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "build/chromeos_buildflags.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/toolbar/app_menu_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/app_menu.h"
#include "chrome/browser/ui/views/toolbar/browser_app_menu_button.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/base/interaction/element_tracker.h"
#include "ui/base/interaction/expect_call_in_scope.h"
#include "ui/base/interaction/interaction_sequence.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/interaction/element_tracker_views.h"
#include "ui/views/interaction/interaction_sequence_views.h"

namespace {

views::View* ElementToView(ui::TrackedElement* element) {
  return element->AsA<views::TrackedElementViews>()->view();
}

}  // namespace

// This test ensures basic compatibility of InteractionSequence[Views] and a
// live browser. It verifies that a simple journey of opening the main menu and
// identifying a specific menu item works.
//
// Located in user_education because it serves as the foundation for Tutorials
// (along with being the primary use case).
//
// In the future, we should add additional specific cases, such as opening a
// dialog or multiple submenus.
class InteractionSequenceBrowserTest : public InProcessBrowserTest {};

#if BUILDFLAG(IS_CHROMEOS_LACROS)
#define MAYBE_OpenMainMenuAndViewHelpItem DISABLED_OpenMainMenuAndViewHelpItem
#else
#define MAYBE_OpenMainMenuAndViewHelpItem OpenMainMenuAndViewHelpItem
#endif
IN_PROC_BROWSER_TEST_F(InteractionSequenceBrowserTest,
                       MAYBE_OpenMainMenuAndViewHelpItem) {
  UNCALLED_MOCK_CALLBACK(ui::InteractionSequence::AbortedCallback, aborted);
  UNCALLED_MOCK_CALLBACK(ui::InteractionSequence::CompletedCallback, completed);

  const ui::ElementContext context =
      views::ElementTrackerViews::GetContextForView(
          BrowserView::GetBrowserViewForBrowser(browser()));

  // Demonstrate that we can find the app button without having to pick through
  // the BrowserView hierarcny.
  views::View* const button_view = ElementToView(
      ui::ElementTracker::GetElementTracker()->GetFirstMatchingElement(
          BrowserAppMenuButton::kIdentifier, context));
  BrowserAppMenuButton* const app_menu_button =
      static_cast<BrowserAppMenuButton*>(button_view);
  DCHECK_EQ(std::string("BrowserAppMenuButton"),
            std::string(app_menu_button->GetClassName()));

  // Define a simple sequence of:
  // - spotting the app menu button
  // - clicking the app menu button
  // - observing a submenu item
  auto sequence =
      ui::InteractionSequence::Builder()
          .SetCompletedCallback(completed.Get())
          .SetAbortedCallback(aborted.Get())
          .AddStep(
              views::InteractionSequenceViews::WithInitialView(app_menu_button))
          .AddStep(ui::InteractionSequence::StepBuilder()
                       .SetElementID(BrowserAppMenuButton::kIdentifier)
                       .SetType(ui::InteractionSequence::StepType::kActivated)
                       .Build())
          .AddStep(ui::InteractionSequence::StepBuilder()
                       .SetElementID(AppMenuModel::kHistoryMenuItem)
                       .SetType(ui::InteractionSequence::StepType::kShown)
                       .Build())
          .Build();

  // Start the sequence and verify that it does not proceed.
  sequence->Start();

  // Click the app menu button, displaying the target element.
  EXPECT_CALL_IN_SCOPE(completed, Run, {
    app_menu_button->OnKeyPressed(ui::KeyEvent(ui::ET_KEY_PRESSED,
                                               ui::VKEY_SPACE, ui::EF_NONE,
                                               ui::EventTimeForNow()));

    app_menu_button->OnKeyReleased(ui::KeyEvent(ui::ET_KEY_RELEASED,
                                                ui::VKEY_SPACE, ui::EF_NONE,
                                                ui::EventTimeForNow()));
  });

  // Verify that we found the correct element and it is visible.
  EXPECT_TRUE(ui::ElementTracker::GetElementTracker()->IsElementVisible(
      AppMenuModel::kHistoryMenuItem, context));
  views::MenuItemView* const history_menu_item =
      app_menu_button->app_menu()->root_menu_item()->GetMenuItemByID(
          IDC_RECENT_TABS_MENU);
  EXPECT_EQ(
      history_menu_item,
      ElementToView(
          ui::ElementTracker::GetElementTracker()->GetFirstMatchingElement(
              AppMenuModel::kHistoryMenuItem, context)));
}
