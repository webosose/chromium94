// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_AUDIO_UNIFIED_VOLUME_VIEW_H_
#define ASH_SYSTEM_AUDIO_UNIFIED_VOLUME_VIEW_H_

#include "ash/components/audio/cras_audio_handler.h"
#include "ash/system/audio/unified_volume_slider_controller.h"
#include "ash/system/unified/unified_slider_view.h"

namespace ash {

// View of a slider that can change audio volume.
class UnifiedVolumeView : public UnifiedSliderView,
                          public CrasAudioHandler::AudioObserver {
 public:
  UnifiedVolumeView(UnifiedVolumeSliderController* controller,
                    UnifiedVolumeSliderController::Delegate* delegate,
                    bool in_bubble);
  ~UnifiedVolumeView() override;

  // views::View:
  const char* GetClassName() const override;

 private:
  void Update(bool by_user);

  // CrasAudioHandler::AudioObserver:
  void OnOutputNodeVolumeChanged(uint64_t node_id, int volume) override;
  void OnOutputMuteChanged(bool mute_on) override;
  void OnAudioNodesChanged() override;
  void OnActiveOutputNodeChanged() override;
  void OnActiveInputNodeChanged() override;

  // UnifiedSliderView:
  void ChildVisibilityChanged(views::View* child) override;

  // views::Button::PressedCallback
  void OnLiveCaptionButtonPressed();

  // Whether the volume slider is in the bubble, as opposed to the system tray.
  const bool in_bubble_;

  views::ToggleImageButton* const live_caption_button_;
  views::Button* const more_button_;

  DISALLOW_COPY_AND_ASSIGN(UnifiedVolumeView);
};

}  // namespace ash

#endif  // ASH_SYSTEM_AUDIO_UNIFIED_VOLUME_VIEW_H_
