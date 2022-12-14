// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBCODECS_RECLAIMABLE_CODEC_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBCODECS_RECLAIMABLE_CODEC_H_

#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/timer.h"

namespace blink {

class DOMException;

class MODULES_EXPORT ReclaimableCodec : public GarbageCollectedMixin {
 public:
  ReclaimableCodec();

  // GarbageCollectedMixin override.
  void Trace(Visitor*) const override;

 protected:
  void MarkCodecActive();

  virtual void OnCodecReclaimed(DOMException*) = 0;

 private:
  void ActivityTimerFired(TimerBase*);
  void StartTimer();

  // This is used to make sure that there are two consecutive ticks of the
  // timer, before we reclaim for inactivity. This prevents immediately
  // reclaiming otherwise active codecs, right after a page suspended/resumed.
  bool last_tick_was_inactive_ = false;

  base::TimeTicks last_activity_;
  HeapTaskRunnerTimer<ReclaimableCodec> activity_timer_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBCODECS_AUDIO_DATA_H_
