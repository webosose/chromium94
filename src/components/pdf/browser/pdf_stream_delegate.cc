// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/pdf/browser/pdf_stream_delegate.h"

#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace pdf {

PdfStreamDelegate::PdfStreamDelegate() = default;
PdfStreamDelegate::~PdfStreamDelegate() = default;

absl::optional<PdfStreamDelegate::StreamInfo> PdfStreamDelegate::GetStreamInfo(
    content::WebContents* contents) {
  return absl::nullopt;
}

}  // namespace pdf
