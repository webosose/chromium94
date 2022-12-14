// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/dialog_model.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_dialog_model_host.h"

namespace chrome {

namespace {

// Unique identifiers used for getting related DialogModelFields.
constexpr int kRemoteAddressId = 1;
constexpr int kRemotePortId = 2;

class DirectSocketsConnectionBubbleDelegate : public ui::DialogModelDelegate {
 public:
  explicit DirectSocketsConnectionBubbleDelegate(
      OnProceedCallback proceed_callback)
      : proceed_callback_(std::move(proceed_callback)) {}

  // Callback functions for when the dialog is accepted or not, with |accepted|
  // set to true or false.
  void OnProceed(bool accepted) {
    std::u16string remote_address;
    std::u16string remote_port;
    // Only send back the user input if the dialog is accepted.
    if (accepted) {
      remote_address =
          dialog_model()->GetTextfieldByUniqueId(kRemoteAddressId)->text();
      remote_port =
          dialog_model()->GetTextfieldByUniqueId(kRemotePortId)->text();
    }
    std::move(proceed_callback_)
        .Run(accepted, base::UTF16ToUTF8(remote_address),
             base::UTF16ToUTF8(remote_port));
  }

  void OnClose() {
    // OnClose() may be called after OnProceed(), in which case the callback is
    // null.
    if (proceed_callback_)
      OnProceed(/*accepted=*/false);
  }

 private:
  OnProceedCallback proceed_callback_;
};

}  // namespace

void ShowDirectSocketsConnectionDialog(Browser* browser,
                                       const std::string& address,
                                       OnProceedCallback proceed_callback) {
  auto bubble_delegate_unique =
      std::make_unique<DirectSocketsConnectionBubbleDelegate>(
          std::move(proceed_callback));
  DirectSocketsConnectionBubbleDelegate* bubble_delegate =
      bubble_delegate_unique.get();

  auto dialog_model =
      ui::DialogModel::Builder(std::move(bubble_delegate_unique))
          .SetTitle(l10n_util::GetStringUTF16(
              IDS_DIRECT_SOCKETS_CONNECTION_BUBBLE_TITLE_LABEL))
          .SetWindowClosingCallback(
              base::BindOnce(&DirectSocketsConnectionBubbleDelegate::OnClose,
                             base::Unretained(bubble_delegate)))
          .AddOkButton(
              base::BindOnce(&DirectSocketsConnectionBubbleDelegate::OnProceed,
                             base::Unretained(bubble_delegate),
                             /*accepted=*/true),
              l10n_util::GetStringUTF16(IDS_OK))
          .AddCancelButton(
              base::BindOnce(&DirectSocketsConnectionBubbleDelegate::OnProceed,
                             base::Unretained(bubble_delegate),
                             /*accepted=*/false),
              l10n_util::GetStringUTF16(IDS_CANCEL))
          .AddTextfield(
              l10n_util::GetStringUTF16(
                  IDS_DIRECT_SOCKETS_CONNECTION_BUBBLE_ADDRESS_LABEL),
              base::UTF8ToUTF16(address),
              ui::DialogModelTextfield::Params().SetUniqueId(kRemoteAddressId))
          .AddTextfield(
              l10n_util::GetStringUTF16(
                  IDS_DIRECT_SOCKETS_CONNECTION_BUBBLE_PORT_LABEL),
              std::u16string(),
              ui::DialogModelTextfield::Params().SetUniqueId(kRemotePortId))
          .SetInitiallyFocusedField(kRemoteAddressId)
          .Build();

  views::BubbleDialogDelegate::CreateBubble(
      std::make_unique<views::BubbleDialogModelHost>(
          std::move(dialog_model),
          BrowserView::GetBrowserViewForBrowser(browser),
          views::BubbleBorder::FLOAT))
      ->Show();
}

}  // namespace chrome
