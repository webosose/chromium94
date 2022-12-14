// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CAST_STREAMING_PUBLIC_CAST_STREAMING_URL_H_
#define COMPONENTS_CAST_STREAMING_PUBLIC_CAST_STREAMING_URL_H_

class GURL;

namespace cast_streaming {

// Returns true if |url| is the Cast Streaming media source URL.
bool IsCastStreamingMediaSourceUrl(const GURL& url);

}  // namespace cast_streaming

#endif  // COMPONENTS_CAST_STREAMING_PUBLIC_CAST_STREAMING_URL_H_
