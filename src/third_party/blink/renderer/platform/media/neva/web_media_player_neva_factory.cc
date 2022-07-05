// Copyright 2018-2019 LG Electronics, Inc.
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

#include "third_party/blink/public/platform/media/neva/web_media_player_neva_factory.h"

#include "third_party/blink/public/platform/media/neva/web_media_player_mse.h"
#include "third_party/blink/public/platform/media/neva/web_media_player_neva.h"

namespace blink {

bool WebMediaPlayerNevaFactory::Playable(const WebMediaPlayerClient* client) {
  int load_type = client->LoadType();
  switch (load_type) {
    case WebMediaPlayer::kLoadTypeMediaSource:
    case WebMediaPlayer::kLoadTypeBlobURL:
    case WebMediaPlayer::kLoadTypeDataURL:
      return WebMediaPlayerMSE::IsAvailable();
    case WebMediaPlayer::kLoadTypeURL:
      if (WebMediaPlayerNeva::CanSupportMediaType(
              client->ContentMIMEType().Latin1()))
        return true;
      break;
  }

  return false;
}

WebMediaPlayer* WebMediaPlayerNevaFactory::CreateWebMediaPlayerNeva(
    WebLocalFrame* frame,
    WebMediaPlayerClient* client,
    WebMediaPlayerEncryptedMediaClient* encrypted_client,
    WebMediaPlayerDelegate* delegate,
    std::unique_ptr<media::RendererFactorySelector> renderer_factory_selector,
    UrlIndex* url_index,
    std::unique_ptr<VideoFrameCompositor> compositor,
    const StreamTextureFactoryCreateCB& stream_texture_factory_create_cb,
    std::unique_ptr<WebMediaPlayerParams> params,
    std::unique_ptr<WebMediaPlayerParamsNeva> params_neva) {
  switch (client->LoadType()) {
    case WebMediaPlayer::kLoadTypeMediaSource:
    case WebMediaPlayer::kLoadTypeBlobURL:
    case WebMediaPlayer::kLoadTypeDataURL:
      return new WebMediaPlayerMSE(frame, client, encrypted_client, delegate,
                                   std::move(renderer_factory_selector),
                                   url_index, std::move(compositor),
                                   stream_texture_factory_create_cb,
                                   std::move(params), std::move(params_neva));
    case WebMediaPlayer::kLoadTypeURL:
      return WebMediaPlayerNeva::Create(
          frame, client, delegate, stream_texture_factory_create_cb,
          std::move(params), std::move(params_neva));
    default:
      NOTREACHED();
      return nullptr;
  }

  NOTREACHED();
  return nullptr;
}

}  // namespace blink
