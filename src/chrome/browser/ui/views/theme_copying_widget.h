// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_THEME_COPYING_WIDGET_H_
#define CHROME_BROWSER_UI_VIEWS_THEME_COPYING_WIDGET_H_

#include "base/scoped_observation.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

// This widget uses a reference widget to provide its NativeTheme and
// ThemeProvider.
class ThemeCopyingWidget : public views::Widget, public views::WidgetObserver {
 public:
  explicit ThemeCopyingWidget(views::Widget* role_model);
  ThemeCopyingWidget(const ThemeCopyingWidget&) = delete;
  ThemeCopyingWidget& operator=(const ThemeCopyingWidget&) = delete;
  ~ThemeCopyingWidget() override;

  // views::Widget:
  const ui::NativeTheme* GetNativeTheme() const override;
  const ui::ThemeProvider* GetThemeProvider() const override;

  // views::WidgetObserver:
  void OnWidgetDestroying(Widget* widget) override;
  void OnWidgetThemeChanged(Widget* widget) override;

 private:
  // The widget we'll copy our theme from.
  views::Widget* role_model_;

  base::ScopedObservation<views::Widget, views::WidgetObserver>
      observed_widget_{this};
};

#endif  // CHROME_BROWSER_UI_VIEWS_THEME_COPYING_WIDGET_H_
