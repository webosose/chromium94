// Copyright 2018-2020 LG Electronics, Inc.
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

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MEDIA_NEVA_WEB_MEDIA_PLAYER_NEVA_FACTORY_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MEDIA_NEVA_WEB_MEDIA_PLAYER_NEVA_FACTORY_H_

#include "media/base/renderer_factory_selector.h"
#include "third_party/blink/public/platform/media/neva/stream_texture_interface.h"
#include "third_party/blink/public/platform/media/neva/web_media_player_params_neva.h"
#include "third_party/blink/public/platform/media/url_index.h"
#include "third_party/blink/public/platform/media/video_frame_compositor.h"
#include "third_party/blink/public/platform/media/web_media_player_params.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_media_player.h"
#include "third_party/blink/public/platform/web_media_player_client.h"
#include "third_party/blink/public/platform/web_media_player_encrypted_media_client.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace blink {
class WebMediaPlayerDelegate;

class BLINK_PLATFORM_EXPORT WebMediaPlayerNevaFactory {
 public:
  // True if WebMediaPlayerNevaFactory has a WebMediaPlayerNeva that
  // can play given client.
  static bool Playable(const WebMediaPlayerClient* client);

  static WebMediaPlayer* CreateWebMediaPlayerNeva(
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
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MEDIA_NEVA_WEB_MEDIA_PLAYER_NEVA_FACTORY_H_
