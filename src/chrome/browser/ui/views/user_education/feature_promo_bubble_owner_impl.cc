// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/user_education/feature_promo_bubble_owner_impl.h"
#include "base/callback.h"
#include "base/no_destructor.h"
#include "base/token.h"
#include "chrome/browser/ui/views/user_education/feature_promo_bubble_view.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/views/widget/widget.h"

FeaturePromoBubbleOwnerImpl::FeaturePromoBubbleOwnerImpl() = default;
FeaturePromoBubbleOwnerImpl::~FeaturePromoBubbleOwnerImpl() = default;

// static
FeaturePromoBubbleOwnerImpl* FeaturePromoBubbleOwnerImpl::GetInstance() {
  static base::NoDestructor<FeaturePromoBubbleOwnerImpl> instance;
  return instance.get();
}

bool FeaturePromoBubbleOwnerImpl::ActivateBubbleForAccessibility() {
  if (!bubble_)
    return false;

  bubble_->GetWidget()->Activate();
  bubble_->RequestFocus();
  return true;
}

absl::optional<base::Token> FeaturePromoBubbleOwnerImpl::ShowBubble(
    FeaturePromoBubbleView::CreateParams params,
    base::OnceClosure close_callback) {
  if (bubble_)
    return absl::nullopt;
  DCHECK(!bubble_id_);
  DCHECK(!close_callback_);

  bubble_ = FeaturePromoBubbleView::Create(std::move(params));
  bubble_id_ = base::Token::CreateRandom();
  widget_observation_.Observe(bubble_->GetWidget());
  close_callback_ = std::move(close_callback);

  return bubble_id_;
}

bool FeaturePromoBubbleOwnerImpl::BubbleIsShowing(base::Token bubble_id) {
  DCHECK_EQ((bubble_ != nullptr), bubble_id_.has_value());
  return bubble_id_ == bubble_id;
}

bool FeaturePromoBubbleOwnerImpl::AnyBubbleIsShowing() {
  DCHECK_EQ((bubble_ != nullptr), bubble_id_.has_value());
  return bubble_;
}

void FeaturePromoBubbleOwnerImpl::CloseBubble(base::Token bubble_id) {
  if (bubble_id_ != bubble_id)
    return;
  DCHECK(bubble_);
  bubble_->GetWidget()->Close();
}

void FeaturePromoBubbleOwnerImpl::NotifyAnchorBoundsChanged() {
  if (bubble_)
    bubble_->OnAnchorBoundsChanged();
}

void FeaturePromoBubbleOwnerImpl::OnWidgetClosing(views::Widget* widget) {
  DCHECK(bubble_);
  DCHECK_EQ(widget, bubble_->GetWidget());
  HandleBubbleClosed();
}

void FeaturePromoBubbleOwnerImpl::OnWidgetDestroying(views::Widget* widget) {
  DCHECK(bubble_);
  DCHECK_EQ(widget, bubble_->GetWidget());
  HandleBubbleClosed();
}

void FeaturePromoBubbleOwnerImpl::HandleBubbleClosed() {
  widget_observation_.Reset();
  bubble_ = nullptr;
  bubble_id_ = absl::nullopt;
  std::move(close_callback_).Run();
}
