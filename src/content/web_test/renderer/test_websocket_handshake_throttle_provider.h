// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_WEB_TEST_RENDERER_TEST_WEBSOCKET_HANDSHAKE_THROTTLE_PROVIDER_H_
#define CONTENT_WEB_TEST_RENDERER_TEST_WEBSOCKET_HANDSHAKE_THROTTLE_PROVIDER_H_

#include <memory>
#include "base/macros.h"
#include "third_party/blink/public/platform/websocket_handshake_throttle.h"
#include "third_party/blink/public/platform/websocket_handshake_throttle_provider.h"

namespace content {

class TestWebSocketHandshakeThrottleProvider
    : public blink::WebSocketHandshakeThrottleProvider {
 public:
  TestWebSocketHandshakeThrottleProvider() = default;
  ~TestWebSocketHandshakeThrottleProvider() override = default;

  std::unique_ptr<blink::WebSocketHandshakeThrottleProvider> Clone(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner) override;
  std::unique_ptr<blink::WebSocketHandshakeThrottle> CreateThrottle(
      int render_frame_id,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestWebSocketHandshakeThrottleProvider);
};

}  // namespace content

#endif  // CONTENT_WEB_TEST_RENDERER_TEST_WEBSOCKET_HANDSHAKE_THROTTLE_PROVIDER_H_
