// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_LOGIN_USER_BOARD_VIEW_MOJO_H_
#define CHROME_BROWSER_ASH_LOGIN_USER_BOARD_VIEW_MOJO_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ash/login/ui/views/user_board_view.h"

namespace chromeos {

// UserBoardView implementation that forwards calls to ash via mojo.
class UserBoardViewMojo : public UserBoardView {
 public:
  UserBoardViewMojo();
  ~UserBoardViewMojo() override;

  // UserBoardView:
  void SetPublicSessionDisplayName(const AccountId& account_id,
                                   const std::string& display_name) override;
  void SetPublicSessionLocales(const AccountId& account_id,
                               std::unique_ptr<base::ListValue> locales,
                               const std::string& default_locale,
                               bool multiple_recommended_locales) override;
  void SetPublicSessionShowFullManagementDisclosure(
      bool show_full_management_disclosure) override;
  void ShowBannerMessage(const std::u16string& message,
                         bool is_warning) override;
  void ShowUserPodCustomIcon(
      const AccountId& account_id,
      const proximity_auth::ScreenlockBridge::UserPodCustomIconInfo& icon_info)
      override;
  void HideUserPodCustomIcon(const AccountId& account_id) override;
  void SetAuthType(const AccountId& account_id,
                   proximity_auth::mojom::AuthType auth_type,
                   const std::u16string& initial_value) override;
  void SetTpmLockedState(const AccountId& account_id,
                         bool is_locked,
                         base::TimeDelta time_left) override;
  void Bind(UserSelectionScreen* screen) override {}
  void Unbind() override {}
  base::WeakPtr<UserBoardView> GetWeakPtr() override;

 private:
  base::WeakPtrFactory<UserBoardViewMojo> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(UserBoardViewMojo);
};

}  // namespace chromeos

// TODO(https://crbug.com/1164001): remove after the //chrome/browser/chromeos
// source migration is finished.
namespace ash {
using ::chromeos::UserBoardViewMojo;
}

#endif  // CHROME_BROWSER_ASH_LOGIN_USER_BOARD_VIEW_MOJO_H_
