// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PASSWORD_MANAGER_ANDROID_PASSWORD_INFOBAR_UTILS_H_
#define CHROME_BROWSER_PASSWORD_MANAGER_ANDROID_PASSWORD_INFOBAR_UTILS_H_

#include "components/signin/public/identity_manager/account_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class Profile;

namespace password_manager {

absl::optional<AccountInfo> GetAccountInfoForPasswordInfobars(Profile* profile,
                                                              bool is_syncing);

absl::optional<AccountInfo> GetAccountInfoForPasswordMessages(Profile* profile,
                                                              bool is_syncing);

}  // namespace password_manager

#endif  // CHROME_BROWSER_PASSWORD_MANAGER_ANDROID_PASSWORD_INFOBAR_UTILS_H_
