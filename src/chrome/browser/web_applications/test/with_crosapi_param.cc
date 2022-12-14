// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/web_applications/test/with_crosapi_param.h"

#include "ash/constants/ash_features.h"
#include "chrome/common/chrome_features.h"

namespace web_app {
namespace test {

WithCrosapiParam::WithCrosapiParam() {
  if (GetParam() == CrosapiParam::kEnabled) {
    scoped_feature_list_.InitWithFeatures(
        {chromeos::features::kLacrosSupport, features::kWebAppsCrosapi}, {});
  } else {
    scoped_feature_list_.InitWithFeatures({}, {features::kWebAppsCrosapi});
  }
}

WithCrosapiParam::~WithCrosapiParam() = default;

// static
std::string WithCrosapiParam::ParamToString(
    testing::TestParamInfo<CrosapiParam> param) {
  switch (param.param) {
    case CrosapiParam::kDisabled:
      return "WebAppsCrosapiDisabled";
    case CrosapiParam::kEnabled:
      return "WebAppsCrosapiEnabled";
  }
}

}  // namespace test
}  // namespace web_app
