// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PAYMENTS_PAYMENT_REQUEST_ROW_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_PAYMENTS_PAYMENT_REQUEST_ROW_VIEW_H_

#include "base/memory/weak_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/button/button.h"

namespace payments {

// This class implements a clickable row of the Payment Request dialog that
// darkens on hover and displays a horizontal ruler on its lower bound.
class PaymentRequestRowView
    : public views::Button,
      public base::SupportsWeakPtr<PaymentRequestRowView> {
 public:
  METADATA_HEADER(PaymentRequestRowView);
  // Creates a row view. If |clickable| is true, the row will be shaded on hover
  // and handle click events. |insets| are used as padding around the content.
  PaymentRequestRowView(PressedCallback callback,
                        bool clickable,
                        const gfx::Insets& insets);
  PaymentRequestRowView(const PaymentRequestRowView&) = delete;
  PaymentRequestRowView& operator=(const PaymentRequestRowView&) = delete;
  ~PaymentRequestRowView() override;

  void set_previous_row(base::WeakPtr<PaymentRequestRowView> previous_row) {
    previous_row_ = previous_row;
  }

 protected:
  bool GetClickable() const;

 private:
  // Sets this row's background to the theme's hovered color to indicate that
  // it's begin hovered or it's focused.
  void SetActiveBackground();

  // Show/hide the separator at the bottom of the row. This is used to hide the
  // separator when the row is hovered.
  void ShowBottomSeparator();
  void HideBottomSeparator();

  // Updates the border used for the bottom separator based on
  // `bottom_separator_visible_`.
  void UpdateBottomSeparator();

  // Sets the row as |highlighted| or not. A row is highlighted if it's hovered
  // on or focused, in which case it hides its bottom separator and gets a light
  // colored background color.
  void SetIsHighlighted(bool highlighted);

  // views::Button:
  void StateChanged(ButtonState old_state) override;
  void OnThemeChanged() override;

  // views::View:
  void OnFocus() override;
  void OnBlur() override;

  bool clickable_;
  bool bottom_separator_visible_ = false;
  gfx::Insets insets_;

  // A non-owned pointer to the previous row object in the UI. Used to hide the
  // bottom border of the previous row when highlighting this one. May be null.
  base::WeakPtr<PaymentRequestRowView> previous_row_;
};

}  // namespace payments

#endif  // CHROME_BROWSER_UI_VIEWS_PAYMENTS_PAYMENT_REQUEST_ROW_VIEW_H_
