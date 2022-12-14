// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCESSIBILITY_AUTOCLICK_AUTOCLICK_RING_HANDLER_H_
#define ASH_ACCESSIBILITY_AUTOCLICK_AUTOCLICK_RING_HANDLER_H_

#include "base/macros.h"
#include "base/timer/timer.h"
#include "ui/gfx/animation/linear_animation.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/widget/widget.h"

namespace ash {

// AutoclickRingHandler displays an animated affordance that is shown
// on autoclick gesture. The animation is a semi-transparent ring which
// fills with white.
class AutoclickRingHandler : public gfx::LinearAnimation {
 public:
  AutoclickRingHandler();
  ~AutoclickRingHandler() override;

  void StartGesture(base::TimeDelta duration,
                    const gfx::Point& center_point_in_screen,
                    views::Widget* widget);
  void StopGesture();
  void SetGestureCenter(const gfx::Point& center_point_in_screen,
                        views::Widget* widget);

  void SetSize(int radius);

 private:
  class AutoclickRingView;

  // The default values of the autoclick ring widget size.
  const int kAutoclickRingInnerRadius = 20;

  enum class AnimationType {
    NONE,
    GROW_ANIMATION,
  };

  void StartAnimation(base::TimeDelta duration);
  void StopAutoclickRing();

  // Overridden from gfx::LinearAnimation.
  void AnimateToState(double state) override;
  void AnimationStopped() override;

  AutoclickRingView* view_ = nullptr;
  views::Widget* ring_widget_ = nullptr;
  // Location of the simulated mouse event from auto click in screen
  // coordinates.
  gfx::Point tap_down_location_;
  AnimationType current_animation_type_ = AnimationType::NONE;
  base::TimeDelta animation_duration_;
  int radius_ = kAutoclickRingInnerRadius;

  DISALLOW_COPY_AND_ASSIGN(AutoclickRingHandler);
};

}  // namespace ash

#endif  // ASH_ACCESSIBILITY_AUTOCLICK_AUTOCLICK_RING_HANDLER_H_
