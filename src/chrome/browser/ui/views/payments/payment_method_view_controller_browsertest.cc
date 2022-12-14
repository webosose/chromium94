// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/payments/payment_request_browsertest_base.h"
#include "chrome/browser/ui/views/payments/payment_request_dialog_view_ids.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/payments/content/payment_request.h"
#include "components/payments/content/payment_request_state.h"
#include "components/payments/content/service_worker_payment_app_finder.h"
#include "components/payments/core/test_payment_manifest_downloader.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace payments {

class PaymentMethodViewControllerTest : public PaymentRequestBrowserTestBase {
 protected:
  PaymentMethodViewControllerTest()
      : gpay_server_(net::EmbeddedTestServer::TYPE_HTTPS),
        kylepay_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    gpay_server_.ServeFilesFromSourceDirectory(
        "components/test/data/payments/google.com/");
    ASSERT_TRUE(gpay_server_.Start());

    kylepay_server_.ServeFilesFromSourceDirectory(
        "components/test/data/payments/kylepay.com/");
    ASSERT_TRUE(kylepay_server_.Start());
    PaymentRequestBrowserTestBase::SetUpOnMainThread();
  }

  void SetDownloaderAndIgnorePortInOriginComparisonForTesting() {
    content::BrowserContext* context =
        GetActiveWebContents()->GetBrowserContext();
    auto downloader = std::make_unique<TestDownloader>(
        context->GetDefaultStoragePartition()
            ->GetURLLoaderFactoryForBrowserProcess());
    downloader->AddTestServerURL("https://kylepay.com/",
                                 kylepay_server_.GetURL("kylepay.com", "/"));
    downloader->AddTestServerURL("https://google.com/",
                                 gpay_server_.GetURL("google.com", "/"));
    ServiceWorkerPaymentAppFinder::GetOrCreateForCurrentDocument(
        GetActiveWebContents()->GetMainFrame())
        ->SetDownloaderAndIgnorePortInOriginComparisonForTesting(
            std::move(downloader));
  }

 private:
  net::EmbeddedTestServer gpay_server_;
  net::EmbeddedTestServer kylepay_server_;

  DISALLOW_COPY_AND_ASSIGN(PaymentMethodViewControllerTest);
};

IN_PROC_BROWSER_TEST_F(PaymentMethodViewControllerTest, OneCardSelected) {
  NavigateTo("/payment_request_no_shipping_test.html");
  autofill::AutofillProfile billing_profile(autofill::test::GetFullProfile());
  AddAutofillProfile(billing_profile);
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(billing_profile.guid());
  AddCreditCard(card);

  InvokePaymentRequestUI();
  OpenPaymentMethodScreen();

  PaymentRequest* request = GetPaymentRequests(GetActiveWebContents()).front();
  EXPECT_EQ(1U, request->state()->available_apps().size());

  views::View* list_view = dialog_view()->GetViewByID(
      static_cast<int>(DialogViewID::PAYMENT_METHOD_SHEET_LIST_VIEW));
  EXPECT_TRUE(list_view);
  EXPECT_EQ(1u, list_view->children().size());

  EXPECT_EQ(request->state()->available_apps().front().get(),
            request->state()->selected_app());
  views::View* checkmark_view = list_view->children().front()->GetViewByID(
      static_cast<int>(DialogViewID::CHECKMARK_VIEW));
  EXPECT_TRUE(checkmark_view->GetVisible());
}

IN_PROC_BROWSER_TEST_F(PaymentMethodViewControllerTest,
                       OneCardSelectedOutOfMany) {
  NavigateTo("/payment_request_no_shipping_test.html");
  autofill::AutofillProfile billing_profile(autofill::test::GetFullProfile());
  AddAutofillProfile(billing_profile);

  autofill::CreditCard card1 = autofill::test::GetCreditCard();
  card1.set_billing_address_id(billing_profile.guid());

  // Ensure that this card is the first suggestion.
  card1.set_use_count(5U);
  AddCreditCard(card1);

  // Slightly different visa.
  autofill::CreditCard card2 = autofill::test::GetCreditCard();
  card2.SetNumber(u"4111111111111112");
  card2.set_billing_address_id(billing_profile.guid());
  card2.set_use_count(1U);
  AddCreditCard(card2);

  InvokePaymentRequestUI();
  OpenPaymentMethodScreen();

  PaymentRequest* request = GetPaymentRequests(GetActiveWebContents()).front();
  EXPECT_EQ(2U, request->state()->available_apps().size());
  EXPECT_EQ(request->state()->available_apps().front().get(),
            request->state()->selected_app());

  views::View* list_view = dialog_view()->GetViewByID(
      static_cast<int>(DialogViewID::PAYMENT_METHOD_SHEET_LIST_VIEW));
  EXPECT_TRUE(list_view);
  EXPECT_EQ(2u, list_view->children().size());

  EXPECT_EQ(request->state()->available_apps().front().get(),
            request->state()->selected_app());
  views::View* checkmark_view = list_view->children()[0]->GetViewByID(
      static_cast<int>(DialogViewID::CHECKMARK_VIEW));
  EXPECT_TRUE(checkmark_view->GetVisible());

  views::View* checkmark_view2 = list_view->children()[1]->GetViewByID(
      static_cast<int>(DialogViewID::CHECKMARK_VIEW));
  EXPECT_FALSE(checkmark_view2->GetVisible());

  ResetEventWaiter(DialogEvent::BACK_NAVIGATION);
  // Simulate selecting the second card.
  ClickOnDialogViewAndWait(list_view->children()[1]);

  EXPECT_EQ(request->state()->available_apps().back().get(),
            request->state()->selected_app());

  OpenPaymentMethodScreen();
  list_view = dialog_view()->GetViewByID(
      static_cast<int>(DialogViewID::PAYMENT_METHOD_SHEET_LIST_VIEW));

  ResetEventWaiter(DialogEvent::BACK_NAVIGATION);
  // Clicking on the second card again should not modify any state, and should
  // return to the main payment sheet.
  ClickOnDialogViewAndWait(list_view->children()[1]);

  EXPECT_EQ(request->state()->available_apps().back().get(),
            request->state()->selected_app());
}

IN_PROC_BROWSER_TEST_F(PaymentMethodViewControllerTest, EditButtonOpensEditor) {
  NavigateTo("/payment_request_no_shipping_test.html");
  AddCreditCard(autofill::test::GetCreditCard());

  InvokePaymentRequestUI();
  OpenPaymentMethodScreen();

  views::View* list_view = dialog_view()->GetViewByID(
      static_cast<int>(DialogViewID::PAYMENT_METHOD_SHEET_LIST_VIEW));
  EXPECT_TRUE(list_view);
  EXPECT_EQ(1u, list_view->children().size());

  views::View* edit_button = list_view->children().front()->GetViewByID(
      static_cast<int>(DialogViewID::EDIT_ITEM_BUTTON));

  ResetEventWaiter(DialogEvent::CREDIT_CARD_EDITOR_OPENED);
  ClickOnDialogViewAndWait(edit_button);
}

IN_PROC_BROWSER_TEST_F(PaymentMethodViewControllerTest,
                       DoNotShowAddCardWhenBasicCardIsNotSupported) {
  NavigateTo("/payment_request_bobpay_and_cards_test.html");
  SetDownloaderAndIgnorePortInOriginComparisonForTesting();

  ResetEventWaiterForDialogOpened();
  content::ExecuteScriptAsync(GetActiveWebContents(), R"(
    testPaymentMethods([
      {supportedMethods: 'https://google.com/pay'},
      {supportedMethods: 'https://kylepay.com/webpay'},
    ]);
  )");
  WaitForObservedEvent();

  // Confirm that "Add card" button is not shown since "basic-card" is not
  // requested.
  OpenPaymentMethodScreen();
  views::View* add_card_button = dialog_view()->GetViewByID(
      static_cast<int>(DialogViewID::PAYMENT_METHOD_ADD_CARD_BUTTON));
  EXPECT_EQ(nullptr, add_card_button);
}

}  // namespace payments
