// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/button/label_button_label.h"

#include <map>
#include <memory>

#include "third_party/skia/include/core/SkColor.h"
#include "ui/native_theme/native_theme_base.h"
#include "ui/views/test/views_test_base.h"

namespace views {
namespace {

// Basic NativeTheme that can customize colors.
class TestNativeTheme : public ui::NativeThemeBase {
 public:
  TestNativeTheme() = default;

  void Set(ColorId id, SkColor color) { colors_[id] = color; }

  // NativeThemeBase:
  SkColor GetSystemColorDeprecated(ColorId color_id,
                                   ColorScheme color_scheme,
                                   bool apply_processing) const override {
    return colors_.count(color_id) ? colors_.find(color_id)->second
                                   : SK_ColorMAGENTA;
  }

 private:
  std::map<ColorId, SkColor> colors_;

  DISALLOW_COPY_AND_ASSIGN(TestNativeTheme);
};

// LabelButtonLabel subclass that reports its text color whenever a paint is
// scheduled.
class TestLabel : public internal::LabelButtonLabel {
 public:
  explicit TestLabel(SkColor* last_color)
      : LabelButtonLabel(std::u16string(), views::style::CONTEXT_BUTTON),
        last_color_(last_color) {}

  // LabelButtonLabel:
  void OnDidSchedulePaint(const gfx::Rect& r) override {
    LabelButtonLabel::OnDidSchedulePaint(r);
    *last_color_ = GetEnabledColor();
  }

 private:
  SkColor* last_color_;

  DISALLOW_COPY_AND_ASSIGN(TestLabel);
};

}  // namespace

class LabelButtonLabelTest : public ViewsTestBase {
 public:
  LabelButtonLabelTest() = default;

  void SetUp() override {
    ViewsTestBase::SetUp();

    widget_ = CreateTestWidget();
    label_ =
        widget_->SetContentsView(std::make_unique<TestLabel>(&last_color_));
    label_->SetAutoColorReadabilityEnabled(false);
  }

  void TearDown() override {
    widget_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  SkColor last_color_ = gfx::kPlaceholderColor;
  std::unique_ptr<views::Widget> widget_;
  TestLabel* label_;
  TestNativeTheme theme1_;
  TestNativeTheme theme2_;

 private:
  DISALLOW_COPY_AND_ASSIGN(LabelButtonLabelTest);
};

// Test that LabelButtonLabel reacts properly to themed and overridden colors.
TEST_F(LabelButtonLabelTest, Colors) {
  // First one comes from the default theme. This check ensures the SK_ColorRED
  // placeholder initializers were replaced.
  SkColor default_theme_enabled_color =
      label_->GetNativeTheme()->GetSystemColor(
          ui::NativeTheme::kColorId_LabelEnabledColor);
  EXPECT_EQ(default_theme_enabled_color, last_color_);

  // Note these are not kColorId_Button{Enabled,Disabled}Color because label
  // buttons use label colors. See LabelButton::ResetColorsFromNativeTheme().
  theme1_.Set(ui::NativeTheme::kColorId_LabelEnabledColor, SK_ColorGREEN);
  theme1_.Set(ui::NativeTheme::kColorId_LabelDisabledColor, SK_ColorYELLOW);
  label_->SetNativeThemeForTesting(&theme1_);

  // Setting the theme should paint.
  EXPECT_EQ(SK_ColorGREEN, last_color_);

  label_->SetEnabled(false);
  EXPECT_EQ(SK_ColorYELLOW, last_color_);

  // Set up a second theme. View::SetNativeTheme() makes the reasonable
  // assumption that NativeTheme doesn't change its mind about things unless a
  // Widget triggers it (which it can do as a friend of RootView).
  theme2_.Set(ui::NativeTheme::kColorId_LabelEnabledColor, SK_ColorBLUE);
  theme2_.Set(ui::NativeTheme::kColorId_LabelDisabledColor, SK_ColorGRAY);
  label_->SetNativeThemeForTesting(&theme2_);

  EXPECT_EQ(SK_ColorGRAY, last_color_);

  label_->SetEnabled(true);
  EXPECT_EQ(SK_ColorBLUE, last_color_);

  // Override the theme for the disabled color.
  label_->SetDisabledColor(SK_ColorRED);

  // Still enabled, so not RED yet.
  EXPECT_EQ(SK_ColorBLUE, last_color_);

  label_->SetEnabled(false);
  EXPECT_EQ(SK_ColorRED, last_color_);

  label_->SetDisabledColor(SK_ColorMAGENTA);
  EXPECT_EQ(SK_ColorMAGENTA, last_color_);

  // Disabled still overridden after a theme change.
  label_->SetNativeThemeForTesting(&theme1_);
  EXPECT_EQ(SK_ColorMAGENTA, last_color_);

  // The enabled color still gets its value from the theme.
  label_->SetEnabled(true);
  EXPECT_EQ(SK_ColorGREEN, last_color_);

  label_->SetEnabledColor(SK_ColorYELLOW);
  label_->SetDisabledColor(SK_ColorCYAN);
  EXPECT_EQ(SK_ColorYELLOW, last_color_);
  label_->SetEnabled(false);
  EXPECT_EQ(SK_ColorCYAN, last_color_);
}

}  // namespace views
