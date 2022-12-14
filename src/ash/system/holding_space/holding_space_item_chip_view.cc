// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/holding_space/holding_space_item_chip_view.h"

#include <algorithm>

#include "ash/bubble/bubble_utils.h"
#include "ash/public/cpp/holding_space/holding_space_client.h"
#include "ash/public/cpp/holding_space/holding_space_constants.h"
#include "ash/public/cpp/holding_space/holding_space_controller.h"
#include "ash/public/cpp/holding_space/holding_space_item.h"
#include "ash/public/cpp/holding_space/holding_space_progress.h"
#include "ash/public/cpp/rounded_image_view.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/style/ash_color_provider.h"
#include "ash/system/holding_space/holding_space_item_view.h"
#include "ash/system/holding_space/holding_space_progress_ring.h"
#include "ash/system/holding_space/holding_space_progress_ring_animation.h"
#include "ash/system/holding_space/holding_space_view_delegate.h"
#include "base/bind.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_owner.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/gfx/skia_paint_util.h"
#include "ui/gfx/transform_util.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/box_layout_view.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"

namespace ash {
namespace {

// Appearance.
constexpr int kChildSpacing = 8;
constexpr int kLabelMaskGradientWidth = 16;
constexpr gfx::Insets kLabelMargins(/*top=*/4, 0, /*bottom=*/4, /*right=*/2);
constexpr gfx::Insets kPadding(0, /*left=*/8, 0, /*right=*/10);
constexpr int kPreferredHeight = 40;
constexpr int kPreferredWidth = 160;
constexpr int kSecondaryActionIconSize = 16;

// Animation.
constexpr base::TimeDelta kInProgressImageScaleDuration =
    base::TimeDelta::FromMilliseconds(150);
constexpr float kInProgressImageScaleFactor = 0.7f;

// Helpers ---------------------------------------------------------------------

template <typename... T>
base::RepeatingCallback<void(T...)> IgnoreArgs(
    base::RepeatingCallback<void()> callback) {
  return base::BindRepeating([](T...) {}).Then(std::move(callback));
}

// ObservableRoundedImageView --------------------------------------------------

class ObservableRoundedImageView : public RoundedImageView {
 public:
  ObservableRoundedImageView() = default;
  ObservableRoundedImageView(const ObservableRoundedImageView&) = delete;
  ObservableRoundedImageView& operator=(const ObservableRoundedImageView&) =
      delete;
  ~ObservableRoundedImageView() override = default;

  using BoundsChangedCallback = base::RepeatingCallback<void()>;
  void SetBoundsChangedCallback(BoundsChangedCallback bounds_changed_callback) {
    bounds_changed_callback_ = std::move(bounds_changed_callback);
  }

 private:
  // RoundedImageView:
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override {
    RoundedImageView::OnBoundsChanged(previous_bounds);
    if (!bounds_changed_callback_.is_null())
      bounds_changed_callback_.Run();
  }

  BoundsChangedCallback bounds_changed_callback_;
};

BEGIN_VIEW_BUILDER(/*no export*/, ObservableRoundedImageView, RoundedImageView)
VIEW_BUILDER_PROPERTY(ObservableRoundedImageView::BoundsChangedCallback,
                      BoundsChangedCallback)
END_VIEW_BUILDER

// PaintCallbackLabel ----------------------------------------------------------

class PaintCallbackLabel : public views::Label {
 public:
  PaintCallbackLabel() = default;
  PaintCallbackLabel(const PaintCallbackLabel&) = delete;
  PaintCallbackLabel& operator=(const PaintCallbackLabel&) = delete;
  ~PaintCallbackLabel() override = default;

  using Callback = base::RepeatingCallback<void(views::Label*, gfx::Canvas*)>;
  void SetCallback(Callback callback) { callback_ = std::move(callback); }

  void SetPaintToLayer(bool fills_bounds_opaquely) {
    views::Label::SetPaintToLayer();
    layer()->SetFillsBoundsOpaquely(fills_bounds_opaquely);
  }

  void SetStyle(bubble_utils::LabelStyle style) {
    bubble_utils::ApplyStyle(this, style);
  }

  void SetViewAccessibilityIsIgnored(bool is_ignored) {
    GetViewAccessibility().OverrideIsIgnored(is_ignored);
  }

 private:
  // views::Label:
  void OnPaint(gfx::Canvas* canvas) override {
    views::Label::OnPaint(canvas);
    if (!callback_.is_null())
      callback_.Run(this, canvas);
  }

  Callback callback_;
};

BEGIN_VIEW_BUILDER(/*no export*/, PaintCallbackLabel, views::Label)
VIEW_BUILDER_PROPERTY(PaintCallbackLabel::Callback, Callback)
VIEW_BUILDER_PROPERTY(bubble_utils::LabelStyle, Style)
VIEW_BUILDER_PROPERTY(bool, PaintToLayer)
VIEW_BUILDER_PROPERTY(bool, ViewAccessibilityIsIgnored)
END_VIEW_BUILDER

// ProgressRingView ------------------------------------------------------------

class ProgressRingView : public views::View {
 public:
  ProgressRingView() = default;
  ProgressRingView(const ProgressRingView&) = delete;
  ProgressRingView& operator=(const ProgressRingView&) = delete;
  ~ProgressRingView() override = default;

  // Sets the underlying `item` for which to indicate progress.
  // NOTE: This method should be invoked only once.
  void SetHoldingSpaceItem(const HoldingSpaceItem* item) {
    DCHECK(!progress_ring_);
    progress_ring_ = HoldingSpaceProgressRing::CreateForItem(item);

    SetPaintToLayer();
    layer()->SetFillsBoundsOpaquely(false);
    layer()->Add(progress_ring_->layer());
  }

 private:
  // views::View:
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override {
    if (progress_ring_)
      progress_ring_->layer()->SetBounds(GetLocalBounds());
  }

  void OnThemeChanged() override {
    views::View::OnThemeChanged();
    if (progress_ring_)
      progress_ring_->InvalidateLayer();
  }

  std::unique_ptr<HoldingSpaceProgressRing> progress_ring_;
};

BEGIN_VIEW_BUILDER(/*no export*/, ProgressRingView, views::View)
VIEW_BUILDER_PROPERTY(const HoldingSpaceItem*, HoldingSpaceItem)
END_VIEW_BUILDER

}  // namespace
}  // namespace ash

DEFINE_VIEW_BUILDER(/*no export*/, ash::ObservableRoundedImageView)
DEFINE_VIEW_BUILDER(/*no export*/, ash::PaintCallbackLabel)
DEFINE_VIEW_BUILDER(/*no export*/, ash::ProgressRingView)

namespace ash {
namespace {

// Helpers ---------------------------------------------------------------------

// Returns a label builder.
// NOTE: A11y events are handled by `HoldingSpaceItemChipView`.
views::Builder<PaintCallbackLabel> CreateLabelBuilder() {
  auto label = views::Builder<PaintCallbackLabel>();
  label.SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT)
      .SetPaintToLayer(/*fills_bounds_opaquely=*/false)
      .SetViewAccessibilityIsIgnored(true);
  return label;
}

// Returns a secondary action builder.
views::Builder<views::ImageButton> CreateSecondaryActionBuilder() {
  using HorizontalAlignment = views::ImageButton::HorizontalAlignment;
  using VerticalAlignment = views::ImageButton::VerticalAlignment;
  auto secondary_action = views::Builder<views::ImageButton>();
  secondary_action.SetFocusBehavior(views::View::FocusBehavior::NEVER)
      .SetImageHorizontalAlignment(HorizontalAlignment::ALIGN_CENTER)
      .SetImageVerticalAlignment(VerticalAlignment::ALIGN_MIDDLE);
  return secondary_action;
}

// TODO(crbug.com/1202796): Create ash colors.
// Returns the theme color to use for text in multiselect.
SkColor GetMultiSelectTextColor() {
  return AshColorProvider::Get()->IsDarkModeEnabled() ? gfx::kGoogleBlue100
                                                      : gfx::kGoogleBlue800;
}

}  // namespace

// HoldingSpaceItemChipView ----------------------------------------------------

HoldingSpaceItemChipView::HoldingSpaceItemChipView(
    HoldingSpaceViewDelegate* delegate,
    const HoldingSpaceItem* item)
    : HoldingSpaceItemView(delegate, item) {
  using CrossAxisAlignment = views::BoxLayout::CrossAxisAlignment;
  using MainAxisAlignment = views::BoxLayout::MainAxisAlignment;
  using Orientation = views::BoxLayout::Orientation;

  auto layout_manager = std::make_unique<views::FlexLayout>();
  layout_manager->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetCrossAxisAlignment(views::LayoutAlignment::kCenter)
      .SetCollapseMargins(true)
      .SetIgnoreDefaultMainAxisMargins(true)
      .SetInteriorMargin(gfx::Insets(kPadding))
      .SetDefault(views::kMarginsKey, gfx::Insets(0, kChildSpacing));

  auto paint_label_mask_callback = base::BindRepeating(
      &HoldingSpaceItemChipView::OnPaintLabelMask, base::Unretained(this));

  auto secondary_action_callback =
      base::BindRepeating(&HoldingSpaceItemChipView::OnSecondaryActionPressed,
                          base::Unretained(this));

  views::Builder<HoldingSpaceItemChipView>(this)
      .SetPreferredSize(gfx::Size(kPreferredWidth, kPreferredHeight))
      .SetLayoutManager(std::move(layout_manager))
      .AddChild(
          views::Builder<ProgressRingView>()
              .SetHoldingSpaceItem(item)
              .SetUseDefaultFillLayout(true)
              .AddChild(views::Builder<ObservableRoundedImageView>()
                            .SetCornerRadius(kHoldingSpaceChipIconSize / 2)
                            .SetBoundsChangedCallback(base::BindRepeating(
                                &HoldingSpaceItemChipView::UpdateImageTransform,
                                base::Unretained(this)))
                            .CopyAddressTo(&image_)
                            .SetID(kHoldingSpaceItemImageId))
              .AddChild(CreateCheckmarkBuilder())
              .AddChild(
                  views::Builder<views::View>()
                      .CopyAddressTo(&secondary_action_container_)
                      .SetID(kHoldingSpaceItemSecondaryActionContainerId)
                      .SetUseDefaultFillLayout(true)
                      .SetVisible(false)
                      .AddChild(CreateSecondaryActionBuilder()
                                    .CopyAddressTo(&secondary_action_pause_)
                                    .SetID(kHoldingSpaceItemPauseButtonId)
                                    .SetCallback(secondary_action_callback)
                                    .SetVisible(false))
                      .AddChild(CreateSecondaryActionBuilder()
                                    .CopyAddressTo(&secondary_action_resume_)
                                    .SetID(kHoldingSpaceItemResumeButtonId)
                                    .SetCallback(secondary_action_callback)
                                    .SetFlipCanvasOnPaintForRTLUI(false)
                                    .SetVisible(false))))
      .AddChild(
          views::Builder<views::View>()
              .SetUseDefaultFillLayout(true)
              .SetProperty(views::kFlexBehaviorKey,
                           views::FlexSpecification(
                               views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded))
              .AddChild(
                  views::Builder<views::BoxLayoutView>()
                      .SetOrientation(Orientation::kVertical)
                      .SetMainAxisAlignment(MainAxisAlignment::kCenter)
                      .SetCrossAxisAlignment(CrossAxisAlignment::kStretch)
                      .SetInsideBorderInsets(kLabelMargins)
                      .AddChild(
                          CreateLabelBuilder()
                              .CopyAddressTo(&primary_label_)
                              .SetID(kHoldingSpaceItemPrimaryChipLabelId)
                              .SetStyle(bubble_utils::LabelStyle::kChipTitle)
                              .SetElideBehavior(gfx::ELIDE_MIDDLE)
                              .SetCallback(paint_label_mask_callback))
                      .AddChild(
                          CreateLabelBuilder()
                              .CopyAddressTo(&secondary_label_)
                              .SetID(kHoldingSpaceItemSecondaryChipLabelId)
                              .SetStyle(bubble_utils::LabelStyle::kChipBody)
                              .SetElideBehavior(gfx::FADE_TAIL)
                              .SetCallback(paint_label_mask_callback)))
              .AddChild(views::Builder<views::BoxLayoutView>()
                            .SetOrientation(Orientation::kHorizontal)
                            .SetMainAxisAlignment(MainAxisAlignment::kEnd)
                            .SetCrossAxisAlignment(CrossAxisAlignment::kCenter)
                            .AddChild(CreatePrimaryActionBuilder(
                                /*min_size=*/gfx::Size()))))
      .BuildChildren();

  // Subscribe to be notified of changes to `item`'s image.
  image_skia_changed_subscription_ =
      item->image().AddImageSkiaChangedCallback(base::BindRepeating(
          &HoldingSpaceItemChipView::UpdateImage, base::Unretained(this)));

  // Subscribe to be notified of changes to `item`'s progress ring animation.
  progress_ring_animation_changed_subscription_ =
      HoldingSpaceAnimationRegistry::GetInstance()
          ->AddProgressRingAnimationChangedCallbackForKey(
              item, IgnoreArgs<HoldingSpaceProgressRingAnimation*>(
                        base::BindRepeating(
                            &HoldingSpaceItemChipView::UpdateImageTransform,
                            base::Unretained(this))));

  UpdateImage();
  UpdateLabels();
}

HoldingSpaceItemChipView::~HoldingSpaceItemChipView() = default;

views::View* HoldingSpaceItemChipView::GetTooltipHandlerForPoint(
    const gfx::Point& point) {
  // Tooltip events should be handled top level, not by descendents.
  return HitTestPoint(point) ? this : nullptr;
}

std::u16string HoldingSpaceItemChipView::GetTooltipText(
    const gfx::Point& point) const {
  std::u16string primary_tooltip = primary_label_->GetTooltipText(point);
  std::u16string secondary_tooltip = secondary_label_->GetTooltipText(point);

  // If there is neither a primary nor a secondary tooltip which should be
  // shown, then there is no tooltip to be shown at all.
  if (primary_tooltip.empty() && secondary_tooltip.empty())
    return base::EmptyString16();

  // If there is no primary tooltip, fallback to using the primary text. This
  // would occur if the `primary_label_` is not elided in same way.
  if (primary_tooltip.empty())
    primary_tooltip = primary_label_->GetText();

  // If there is no secondary tooltip, fallback to using the secondary text.
  // This would occur if the `secondary_label_` is not elided in some way.
  if (secondary_tooltip.empty())
    secondary_tooltip = secondary_label_->GetText();

  // If there still is no secondary tooltip, only the primary tooltip should be
  // shown. This would occur if there is no visible `secondary_label_`.
  if (secondary_tooltip.empty())
    return primary_tooltip;

  // Otherwise, concatenate and return the primary and secondary tooltips. This
  // will look something of the form: "filename.txt, Paused, 10/100 MB".
  return l10n_util::GetStringFUTF16(
      IDS_ASH_HOLDING_SPACE_CHIP_A11Y_NAME_AND_TOOLTIP, primary_tooltip,
      secondary_tooltip);
}

void HoldingSpaceItemChipView::OnHoldingSpaceItemUpdated(
    const HoldingSpaceItem* item,
    uint32_t updated_fields) {
  HoldingSpaceItemView::OnHoldingSpaceItemUpdated(item, updated_fields);
  if (this->item() == item) {
    UpdateImage();
    UpdateLabels();
    UpdateSecondaryAction();
  }
}

void HoldingSpaceItemChipView::OnPrimaryActionVisibilityChanged(bool visible) {
  // Labels must be repainted to update their masks for
  // `primary_action_container()`  visibility.
  primary_label_->SchedulePaint();
  secondary_label_->SchedulePaint();
}

void HoldingSpaceItemChipView::OnSelectionUiChanged() {
  HoldingSpaceItemView::OnSelectionUiChanged();
  UpdateLabels();
  UpdateSecondaryAction();
}

void HoldingSpaceItemChipView::OnMouseEvent(ui::MouseEvent* event) {
  HoldingSpaceItemView::OnMouseEvent(event);
  switch (event->type()) {
    case ui::ET_MOUSE_ENTERED:
    case ui::ET_MOUSE_EXITED:
      UpdateSecondaryAction();
      break;
    default:
      break;
  }
}

void HoldingSpaceItemChipView::OnThemeChanged() {
  HoldingSpaceItemView::OnThemeChanged();
  UpdateImage();
  UpdateLabels();

  // Pause.
  const SkColor icon_color = AshColorProvider::Get()->GetContentLayerColor(
      AshColorProvider::ContentLayerType::kButtonIconColor);
  secondary_action_pause_->SetImage(
      views::Button::STATE_NORMAL,
      gfx::CreateVectorIcon(kPauseIcon, kSecondaryActionIconSize, icon_color));

  // Resume.
  secondary_action_resume_->SetImage(
      views::Button::STATE_NORMAL,
      gfx::CreateVectorIcon(kResumeIcon, kSecondaryActionIconSize, icon_color));
}

void HoldingSpaceItemChipView::OnPaintLabelMask(views::Label* label,
                                                gfx::Canvas* canvas) {
  // If the `primary_action_container()` isn't visible, masking is unnecessary.
  if (!primary_action_container()->GetVisible())
    return;

  // If the `primary_action_container()` is visible, `label` fades out its tail
  // to avoid overlap.
  gfx::Point gradient_start, gradient_end;
  if (base::i18n::IsRTL()) {
    gradient_end.set_x(primary_action_container()->width());
    gradient_start.set_x(gradient_end.x() + kLabelMaskGradientWidth);
  } else {
    gradient_end.set_x(label->width() - primary_action_container()->width());
    gradient_start.set_x(gradient_end.x() - kLabelMaskGradientWidth);
  }

  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setBlendMode(SkBlendMode::kDstIn);
  flags.setShader(gfx::CreateGradientShader(
      gradient_start, gradient_end, SK_ColorBLACK, SK_ColorTRANSPARENT));

  canvas->DrawRect(label->GetLocalBounds(), flags);
}

void HoldingSpaceItemChipView::OnSecondaryActionPressed() {
  // If the associated `item()` has been deleted then `this` is in the process
  // of being destroyed and no action needs to be taken.
  if (!item())
    return;

  DCHECK_NE(secondary_action_pause_->GetVisible(),
            secondary_action_resume_->GetVisible());

  if (delegate())
    delegate()->OnHoldingSpaceItemViewSecondaryActionPressed(this);

  // Pause.
  if (secondary_action_pause_->GetVisible()) {
    HoldingSpaceController::Get()->client()->PauseItems({item()});
    return;
  }

  // Resume.
  HoldingSpaceController::Get()->client()->ResumeItems({item()});
}

void HoldingSpaceItemChipView::UpdateImage() {
  // If the associated `item()` has been deleted then `this` is in the process
  // of being destroyed and no action needs to be taken.
  if (!item())
    return;

  // Image.
  image_->SetImage(item()->image().GetImageSkia(
      gfx::Size(kHoldingSpaceChipIconSize, kHoldingSpaceChipIconSize),
      /*dark_background=*/AshColorProvider::Get()->IsDarkModeEnabled()));
  SchedulePaint();

  // Transform.
  UpdateImageTransform();
}

void HoldingSpaceItemChipView::UpdateImageTransform() {
  // If the associated `item()` has been deleted then `this` is in the process
  // of being destroyed and no action needs to be taken.
  if (!item())
    return;

  // Wait until `image_` has been laid out before updating transform. Once
  // bounds have been set, `UpdateImageTransform()` will be invoked again.
  if (image_->bounds().IsEmpty())
    return;

  const bool is_item_visibly_in_progress =
      !item()->progress().IsHidden() && !item()->progress().IsComplete();

  const HoldingSpaceProgressRingAnimation* progress_ring_animation =
      HoldingSpaceAnimationRegistry::GetInstance()
          ->GetProgressRingAnimationForKey(item());

  gfx::Transform transform;
  if (is_item_visibly_in_progress || progress_ring_animation) {
    transform = gfx::GetScaleTransform(image_->bounds().CenterPoint(),
                                       kInProgressImageScaleFactor);
  }

  if (!image_->layer()) {
    image_->SetPaintToLayer();
    image_->layer()->SetFillsBoundsOpaquely(false);
  }

  if (image_->layer()->GetTargetTransform() == transform)
    return;

  // Non-identity transforms take effect immediately so as not to cause overlap
  // between the `image_` and progress ring.
  if (!transform.IsIdentity()) {
    image_->layer()->SetTransform(transform);
    return;
  }

  // Identify transforms are animated.
  ui::ScopedLayerAnimationSettings settings(image_->layer()->GetAnimator());
  settings.SetTransitionDuration(kInProgressImageScaleDuration);
  settings.SetTweenType(gfx::Tween::Type::LINEAR_OUT_SLOW_IN);
  image_->layer()->SetTransform(transform);
}

void HoldingSpaceItemChipView::UpdateLabels() {
  // If the associated `item()` has been deleted then `this` is in the process
  // of being destroyed and no action needs to be taken.
  if (!item())
    return;

  const bool multiselect =
      delegate() && delegate()->selection_ui() ==
                        HoldingSpaceViewDelegate::SelectionUi::kMultiSelect;

  // Primary.
  const std::u16string last_primary_text = primary_label_->GetText();
  primary_label_->SetText(item()->GetText());
  primary_label_->SetEnabledColor(
      selected() && multiselect
          ? GetMultiSelectTextColor()
          : AshColorProvider::Get()->GetContentLayerColor(
                AshColorProvider::ContentLayerType::kTextColorPrimary));

  // Secondary.
  const std::u16string last_secondary_text = secondary_label_->GetText();
  secondary_label_->SetText(
      item()->secondary_text().value_or(base::EmptyString16()));
  secondary_label_->SetEnabledColor(
      selected() && multiselect
          ? GetMultiSelectTextColor()
          : AshColorProvider::Get()->GetContentLayerColor(
                AshColorProvider::ContentLayerType::kTextColorSecondary));
  secondary_label_->SetVisible(!secondary_label_->GetText().empty());

  // Updating accessibility and tooltip is only necessary if the text displayed
  // in `this` view has changed.
  if (primary_label_->GetText() == last_primary_text &&
      secondary_label_->GetText() == last_secondary_text) {
    return;
  }

  // Accessibility.
  std::u16string accessible_name =
      secondary_label_->GetText().empty()
          ? primary_label_->GetText()
          : l10n_util::GetStringFUTF16(
                IDS_ASH_HOLDING_SPACE_CHIP_A11Y_NAME_AND_TOOLTIP,
                primary_label_->GetText(), secondary_label_->GetText());
  GetViewAccessibility().OverrideName(accessible_name);
  NotifyAccessibilityEvent(ax::mojom::Event::kTextChanged,
                           /*send_native_event=*/true);

  // Tooltip.
  TooltipTextChanged();
}

void HoldingSpaceItemChipView::UpdateSecondaryAction() {
  // If the associated `item()` has been deleted then `this` is in the process
  // of being destroyed and no action needs to be taken.
  if (!item())
    return;

  const bool has_secondary_action = !checkmark()->GetVisible() &&
                                    !item()->progress().IsComplete() &&
                                    IsMouseHovered();

  if (!has_secondary_action) {
    image_->SetVisible(!checkmark()->GetVisible());
    secondary_action_container_->SetVisible(false);
    return;
  }

  // Pause/resume.
  const bool is_item_paused = item()->IsPaused();
  secondary_action_pause_->SetVisible(!is_item_paused);
  secondary_action_resume_->SetVisible(is_item_paused);

  image_->SetVisible(false);
  secondary_action_container_->SetVisible(true);
}

BEGIN_METADATA(HoldingSpaceItemChipView, HoldingSpaceItemView)
END_METADATA

}  // namespace ash
