// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_REMOTE_OPEN_URL_CONSTANTS_H_
#define REMOTING_HOST_REMOTE_OPEN_URL_CONSTANTS_H_

#include "mojo/public/cpp/platform/named_platform_channel.h"

namespace remoting {

extern const char kRemoteOpenUrlDataChannelName[];

const mojo::NamedPlatformChannel::ServerName& GetRemoteOpenUrlIpcChannelName();

}  // namespace remoting

#endif  // REMOTING_HOST_REMOTE_OPEN_URL_CONSTANTS_H_
