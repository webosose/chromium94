// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CDM_CDM_PREFERENCE_DATA_H_
#define MEDIA_CDM_CDM_PREFERENCE_DATA_H_

#include <vector>

#include "base/unguessable_token.h"
#include "media/base/media_export.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace media {
struct MEDIA_EXPORT CdmPreferenceData {
  CdmPreferenceData();
  CdmPreferenceData(base::UnguessableToken origin_id,
                    absl::optional<std::vector<uint8_t>> client_token);

  CdmPreferenceData(const CdmPreferenceData& other) = delete;
  CdmPreferenceData& operator=(const CdmPreferenceData& other) = delete;

  ~CdmPreferenceData();

  base::UnguessableToken origin_id;
  absl::optional<std::vector<uint8_t>> client_token;
};
}  // namespace media

#endif  // MEDIA_CDM_CDM_PREFERENCE_DATA_H_
