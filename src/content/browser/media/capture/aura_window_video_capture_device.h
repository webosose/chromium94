// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CAPTURE_AURA_WINDOW_VIDEO_CAPTURE_DEVICE_H_
#define CONTENT_BROWSER_MEDIA_CAPTURE_AURA_WINDOW_VIDEO_CAPTURE_DEVICE_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "build/chromeos_buildflags.h"
#include "content/browser/media/capture/frame_sink_video_capture_device.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace aura {
class Window;
}  // namespace aura

namespace content {

struct DesktopMediaID;

// Captures the displayed contents of an aura::Window, producing a stream of
// video frames.
class CONTENT_EXPORT AuraWindowVideoCaptureDevice final
    : public FrameSinkVideoCaptureDevice,
      public base::SupportsWeakPtr<AuraWindowVideoCaptureDevice> {
 public:
  explicit AuraWindowVideoCaptureDevice(const DesktopMediaID& source_id);
  ~AuraWindowVideoCaptureDevice() final;

#if BUILDFLAG(IS_CHROMEOS_ASH)
 protected:
  // Overrides FrameSinkVideoCaptureDevice::CreateCapturer() to create a
  // SlowWindowCapturerChromeOS for window capture where compositor frame sinks
  // are not present. If the new features::kAuraWindowSubtreeCapture flag is
  // enabled, this will use the base's frame sink capture code.
  // TODO(crbug.com/1210549): remove once we have determined the new path is
  // stable.
  void CreateCapturer(
      mojo::PendingReceiver<viz::mojom::FrameSinkVideoCapturer> receiver) final;
#endif

 private:
  // Monitors the target Window and notifies the base class if it is destroyed.
  class WindowTracker;

  // A helper that runs on the UI thread to monitor the target aura::Window, and
  // post a notification if it is destroyed.
  const std::unique_ptr<WindowTracker, BrowserThread::DeleteOnUIThread>
      tracker_;

  DISALLOW_COPY_AND_ASSIGN(AuraWindowVideoCaptureDevice);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CAPTURE_AURA_WINDOW_VIDEO_CAPTURE_DEVICE_H_
