// Copyright 2015 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MEDIA_NEVA_WEB_MEDIA_PLAYER_MSE_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MEDIA_NEVA_WEB_MEDIA_PLAYER_MSE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "media/base/media_log.h"
#include "media/neva/media_platform_api.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/platform/media/neva/video_frame_provider_impl.h"
#include "third_party/blink/public/platform/media/neva/web_media_player_params_neva.h"
#include "third_party/blink/public/platform/media/web_media_player_impl.h"
#include "third_party/blink/public/platform/web_media_player_source.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/platform_window/neva/mojom/video_window.mojom.h"

#if defined(USE_VIDEO_TEXTURE)
#include "ui/gl/neva/video_texture.h"
#endif

namespace blink {

class MediaPlatformAPI;
class WebMediaPlayerImpl;
class VideoHoleGeometryUpdateHelper;

// The canonical implementation of WebMediaPlayer that's backed by
// Pipeline. Handles normal resource loading, Media Source, and
// Encrypted Media.
class BLINK_PLATFORM_EXPORT WebMediaPlayerMSE
    : public ui::mojom::VideoWindowClient,
      public WebMediaPlayerImpl {
 public:
  // Constructs a WebMediaPlayer implementation using Chromium's media stack.
  // |delegate| may be null. |renderer_factory_selector| must not be null.
  WebMediaPlayerMSE(
      WebLocalFrame* frame,
      WebMediaPlayerClient* client,
      WebMediaPlayerEncryptedMediaClient* encrypted_client,
      WebMediaPlayerDelegate* delegate,
      std::unique_ptr<media::RendererFactorySelector> renderer_factory_selector,
      UrlIndex* url_index,
      std::unique_ptr<VideoFrameCompositor> compositor,
      const StreamTextureFactoryCreateCB& stream_texture_factory_create_cb,
      std::unique_ptr<WebMediaPlayerParams> params,
      std::unique_ptr<WebMediaPlayerParamsNeva> params_neva);
  WebMediaPlayerMSE(const WebMediaPlayerMSE&) = delete;
  WebMediaPlayerMSE& operator=(const WebMediaPlayerMSE&) = delete;
  ~WebMediaPlayerMSE() override;

  static bool IsAvailable();

  WebMediaPlayer::LoadTiming Load(LoadType load_type,
                                  const WebMediaPlayerSource& source,
                                  CorsMode cors_mode,
                                  bool is_cache_disabled) override;

  void Play() override;

  void Pause() override;
  void Seek(double seconds) override;
  void SetRate(double rate) override;

  void SetVolume(double volume) override;

  void SetVolumeMultiplier(double multiplier) override;

  // WebMediaPlayerDelegate::Observer interface stubs
  // TODO(neva): Below two methods changed to similar function name.
  //             Need to verify.
  void OnFrameHidden() override {}
  void OnFrameShown() override {}
  void OnIdleTimeout() override {}

  double TimelineOffset() const override;
  bool UsesIntrinsicSize() const override;
  void SetRenderMode(WebMediaPlayer::RenderMode mode) override;
  void SetDisableAudio(bool disable) override;
  void Suspend() override;
  void OnMediaActivationPermitted() override;
  void OnMediaPlayerObserverConnectionEstablished() override;

  void Resume();
  void OnLoadPermitted();

  bool RenderTexture() const {
    return render_mode_ == WebMediaPlayer::RenderModeTexture;
  }

  scoped_refptr<media::VideoFrame> GetCurrentFrameFromCompositor()
      const override;

 private:
  enum StatusOnSuspended {
    UnknownStatus = 0,
    PlayingStatus,
    PausedStatus,
  };

  void ProcessPendingRequests();
  void OnResumed();
  void OnSuspended();
  void OnVideoSizeChanged(const gfx::Size& coded_size,
                          const gfx::Size& natural_size);
  void OnError(media::PipelineStatus status) override;

  void OnMetadata(const media::PipelineMetadata& metadata) override;

  // Implements ui::mojom::VideoWindowClient
  void OnVideoWindowCreated(const ui::VideoWindowInfo& info) override;
  void OnVideoWindowDestroyed() override;
  void OnVideoWindowGeometryChanged(const gfx::Rect& rect) override;
  void OnVideoWindowVisibilityChanged(bool visibility) override;
  // End of ui::mojom::VideoWindowClient

  bool EnsureVideoWindowCreated();
  void ContinuePlayerWithWindowId();

  std::unique_ptr<VideoFrameProviderImpl> video_frame_provider_;
  std::string app_id_;
  bool is_suspended_ = false;
  StatusOnSuspended status_on_suspended_ = UnknownStatus;

  scoped_refptr<media::MediaPlatformAPI> media_platform_api_;

  // This value is updated by using value from media platform api.
  gfx::Size coded_size_;
  gfx::Size natural_size_;

  bool is_loading_ = false;
  LoadType pending_load_type_ = WebMediaPlayer::kLoadTypeMediaSource;
  WebMediaPlayerSource pending_source_;
  CorsMode pending_cors_mode_ = WebMediaPlayer::kCorsModeUnspecified;
  bool pending_is_cache_disabled_ = false;

  WebMediaPlayer::RenderMode render_mode_ = WebMediaPlayer::RenderModeNone;

  bool has_activation_permit_ = false;
  bool require_media_resource_ = true;

  PendingRequest pending_request_;

  WebMediaPlayerParamsNeva::CreateVideoWindowCB create_video_window_cb_;
  absl::optional<ui::VideoWindowInfo> video_window_info_ = absl::nullopt;
  mojo::Remote<ui::mojom::VideoWindow> video_window_remote_;
  mojo::Receiver<ui::mojom::VideoWindowClient> video_window_client_receiver_{
      this};

  base::WeakPtr<WebMediaPlayerMSE> weak_this_for_mse_;
  base::WeakPtrFactory<WebMediaPlayerMSE> weak_factory_for_mse_{this};
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MEDIA_NEVA_WEB_MEDIA_PLAYER_MSE_H_
