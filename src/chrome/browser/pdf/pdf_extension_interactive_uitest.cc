// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/pdf/pdf_extension_test_util.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_browsertest_util.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/browser_plugin_guest_manager.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/api/extensions_api_client.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"

#if defined(TOOLKIT_VIEWS) && defined(USE_AURA)
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/events/gesture_event_details.h"
#include "ui/events/types/event_type.h"
#include "ui/views/touchui/touch_selection_menu_views.h"
#include "ui/views/widget/any_widget_observer.h"
#endif  // defined(TOOLKIT_VIEWS) && defined(USE_AURA)

namespace {
using ::pdf_extension_test_util::ConvertPageCoordToScreenCoord;
using ::pdf_extension_test_util::EnsurePDFHasLoaded;
}  // namespace

class PDFExtensionInteractiveUITest : public extensions::ExtensionApiTest {
 public:
  ~PDFExtensionInteractiveUITest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    content::IsolateAllSitesForTesting(command_line);
  }

  void SetUpOnMainThread() override {
    extensions::ExtensionApiTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->InitializeAndListen());
    content::SetupCrossSiteRedirector(embedded_test_server());
    embedded_test_server()->StartAcceptingConnections();
  }

  void TearDownOnMainThread() override {
    ASSERT_TRUE(embedded_test_server()->ShutdownAndWaitUntilComplete());
    extensions::ExtensionApiTest::TearDownOnMainThread();
  }

  content::WebContents* LoadPdfGetGuestContents(const GURL& url) {
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
    if (!EnsurePDFHasLoaded(GetActiveWebContents()))
      return nullptr;

    content::WebContents* contents = GetActiveWebContents();
    content::BrowserPluginGuestManager* guest_manager =
        contents->GetBrowserContext()->GetGuestManager();
    return guest_manager->GetFullPageGuest(contents);
  }

  content::WebContents* GetActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }
};

#if defined(TOOLKIT_VIEWS) && defined(USE_AURA)
// On text selection, a touch selection menu should pop up. On clicking ellipsis
// icon on the menu, the context menu should open up.
IN_PROC_BROWSER_TEST_F(PDFExtensionInteractiveUITest,
                       ContextMenuOpensFromTouchSelectionMenu) {
  const GURL url = embedded_test_server()->GetURL("/pdf/text_large.pdf");
  content::WebContents* const guest_contents = LoadPdfGetGuestContents(url);
  ASSERT_TRUE(guest_contents);

  views::NamedWidgetShownWaiter waiter(views::test::AnyWidgetTestPasskey{},
                                       "TouchSelectionMenuViews");
  gfx::Point screen_pos =
      ConvertPageCoordToScreenCoord(guest_contents, {12, 12});
  content::SimulateTouchEventAt(GetActiveWebContents(), ui::ET_TOUCH_PRESSED,
                                screen_pos);
  bool success = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetActiveWebContents(),
      "window.addEventListener('message', function(event) {"
      "  if (event.data.type == 'touchSelectionOccurred')"
      "    window.domAutomationController.send(true);"
      "});",
      &success));
  ASSERT_TRUE(success);
  content::SimulateTouchEventAt(GetActiveWebContents(), ui::ET_TOUCH_RELEASED,
                                screen_pos);
  views::Widget* widget = waiter.WaitIfNeededAndGet();
  ASSERT_TRUE(widget);
  views::View* menu = widget->GetContentsView();
  ASSERT_TRUE(menu);
  views::View* ellipsis_button = menu->GetViewByID(
      views::TouchSelectionMenuViews::ButtonViewId::kEllipsisButton);
  ASSERT_TRUE(ellipsis_button);
  ContextMenuWaiter context_menu_observer;
  ui::GestureEvent tap(0, 0, 0, ui::EventTimeForNow(),
                       ui::GestureEventDetails(ui::ET_GESTURE_TAP));
  ellipsis_button->OnGestureEvent(&tap);
  context_menu_observer.WaitForMenuOpenAndClose();

  // Verify that the expected context menu items are present.
  //
  // Note that the assertion below doesn't use exact matching via
  // testing::ElementsAre, because some platforms may include unexpected extra
  // elements (e.g. an extra separator and IDC=100 has been observed on some Mac
  // bots).
  EXPECT_THAT(
      context_menu_observer.GetCapturedCommandIds(),
      testing::IsSupersetOf(
          {IDC_CONTENT_CONTEXT_COPY, IDC_CONTENT_CONTEXT_SEARCHWEBFOR,
           IDC_PRINT, IDC_CONTENT_CONTEXT_ROTATECW,
           IDC_CONTENT_CONTEXT_ROTATECCW, IDC_CONTENT_CONTEXT_INSPECTELEMENT}));
}
#endif  // defined(TOOLKIT_VIEWS) && defined(USE_AURA)
