// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/privacy_budget/field_trial_param_conversions.h"

#include <algorithm>
#include <utility>
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "third_party/blink/public/common/privacy_budget/identifiable_surface.h"

namespace privacy_budget_internal {

bool DecodeIdentifiabilityType(const base::StringPiece s,
                               blink::IdentifiableSurface* out) {
  uint64_t hash = 0;
  if (!base::StringToUint64(s, &hash))
    return false;
  *out = blink::IdentifiableSurface::FromMetricHash(hash);
  return true;
}

bool DecodeIdentifiabilityType(const base::StringPiece s,
                               blink::IdentifiableSurface::Type* out) {
  uint64_t hash = 0;
  if (!base::StringToUint64(s, &hash))
    return false;
  if ((hash & blink::IdentifiableSurface::kTypeMask) != hash)
    return false;
  *out = static_cast<blink::IdentifiableSurface::Type>(hash);
  return true;
}

bool DecodeIdentifiabilityType(const base::StringPiece s, int* out) {
  return base::StringToInt(s, out);
}

bool DecodeIdentifiabilityType(const base::StringPiece s, unsigned int* out) {
  return base::StringToUint(s, out);
}

bool DecodeIdentifiabilityType(const base::StringPiece s, double* out) {
  return base::StringToDouble(s, out);
}

bool DecodeIdentifiabilityType(const base::StringPiece s,
                               std::vector<blink::IdentifiableSurface>* out) {
  *out = DecodeIdentifiabilityFieldTrialParam<
      std::vector<blink::IdentifiableSurface>, ';'>(s);
  return !out->empty();
}

std::string EncodeIdentifiabilityType(const blink::IdentifiableSurface& s) {
  return base::NumberToString(s.ToUkmMetricHash());
}

std::string EncodeIdentifiabilityType(
    const blink::IdentifiableSurface::Type& t) {
  return base::NumberToString(static_cast<uint64_t>(t));
}

std::string EncodeIdentifiabilityType(
    const std::pair<const blink::IdentifiableSurface, int>& v) {
  return base::StrCat({EncodeIdentifiabilityType(v.first), ";",
                       base::NumberToString(v.second)});
}

std::string EncodeIdentifiabilityType(const unsigned int& v) {
  return base::NumberToString(v);
}

std::string EncodeIdentifiabilityType(
    const std::vector<blink::IdentifiableSurface>& value) {
  std::vector<std::string> parts;
  base::ranges::transform(
      value, std::back_inserter(parts),
      [](const blink::IdentifiableSurface v) -> std::string {
        return EncodeIdentifiabilityType(v);
      });
  return base::JoinString(parts, ";");
}

}  // namespace privacy_budget_internal
