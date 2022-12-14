// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TIME_TIME_VIEW_H_
#define ASH_SYSTEM_TIME_TIME_VIEW_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/system/model/clock_observer.h"
#include "ash/system/tray/actionable_view.h"
#include "base/i18n/time_formatting.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "components/session_manager/session_manager_types.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/shadow_value.h"
#include "ui/views/view.h"

namespace base {
class Time;
}

namespace views {
class Label;
}

namespace ash {

class ClockModel;

namespace tray {

// Tray view used to display the current time.
// Exported for tests.
class ASH_EXPORT TimeView : public ActionableView, public ClockObserver {
 public:
  enum class ClockLayout {
    HORIZONTAL_CLOCK,
    VERTICAL_CLOCK,
  };

  TimeView(ClockLayout clock_layout, ClockModel* model);
  ~TimeView() override;

  // Updates clock layout.
  void UpdateClockLayout(ClockLayout clock_layout);

  // Updates the time text color.
  void SetTextColor(SkColor color, bool auto_color_readability_enabled = false);

  // Updates the time text fontlist.
  void SetTextFont(const gfx::FontList& font_list);

  // Updates the time text shadow values.
  void SetTextShadowValues(const gfx::ShadowValues& shadows);

  // Shows the date in horizontal view when |show_date_when_horizontal| is true.
  void SetShowDateWhenHorizontal(bool show_date_when_horizontal);

  // ClockObserver:
  void OnDateFormatChanged() override;
  void OnSystemClockTimeUpdated() override;
  void OnSystemClockCanSetTimeChanged(bool can_set_time) override;
  void Refresh() override;

  base::HourClockType GetHourTypeForTesting() const;

  // views::View:
  const char* GetClassName() const override;

 private:
  friend class TimeViewTest;
  friend class TimeTrayItemViewTest;

  // ActionableView:
  bool PerformAction(const ui::Event& event) override;

  // views::View:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void ChildPreferredSizeChanged(views::View* child) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;

  // Updates the displayed text for the current time and calls SetTimer().
  void UpdateText();

  // Updates the format of the displayed time.
  void UpdateTimeFormat();

  // Updates labels to display the current time.
  void UpdateTextInternal(const base::Time& now);

  void SetupSubviews(ClockLayout clock_layout);
  void SetupLabel(views::Label* label);

  // Starts |timer_| to schedule the next update.
  void SetTimer(const base::Time& now);

  // Subviews used for different layouts.
  // When either of the subviews is in use, it transfers the ownership to the
  // views hierarchy and becomes nullptr.
  std::unique_ptr<views::View> horizontal_view_;
  std::unique_ptr<views::View> vertical_view_;

  // Label text used for the normal horizontal shelf.
  views::Label* horizontal_label_;

  // The time label is split into two lines for the vertical shelf.
  views::Label* vertical_label_hours_;
  views::Label* vertical_label_minutes_;

  // Indicates if date should be show in horizontal view.
  bool show_date_when_horizontal_ = false;

  // Invokes UpdateText() when the displayed time should change.
  base::OneShotTimer timer_;

  ClockModel* const model_;

  DISALLOW_COPY_AND_ASSIGN(TimeView);
};

}  // namespace tray
}  // namespace ash

#endif  // ASH_SYSTEM_TIME_TIME_VIEW_H_
