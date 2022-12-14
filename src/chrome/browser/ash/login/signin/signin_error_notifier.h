// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_LOGIN_SIGNIN_SIGNIN_ERROR_NOTIFIER_H_
#define CHROME_BROWSER_ASH_LOGIN_SIGNIN_SIGNIN_ERROR_NOTIFIER_H_

#include <string>
#include <vector>

#include "base/auto_reset.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ash/login/signin/token_handle_util.h"
#include "components/account_id/account_id.h"
#include "components/account_manager_core/account.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/signin/core/browser/signin_error_controller.h"

class Profile;
class PrefRegistrySimple;

namespace signin {
class IdentityManager;
}  // namespace signin.

namespace ash {
class AccountManager;

// Shows signin-related errors as notifications in Ash.
class SigninErrorNotifier : public SigninErrorController::Observer,
                            public KeyedService {
 public:
  SigninErrorNotifier(SigninErrorController* controller, Profile* profile);
  ~SigninErrorNotifier() override;

  static std::unique_ptr<base::AutoReset<bool>> IgnoreSyncErrorsForTesting();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  // KeyedService:
  void Shutdown() override;

  // SigninErrorController::Observer:
  void OnErrorChanged() override;

 private:
  // Handles errors for the Device Account.
  // Displays a notification asking the user to Sign Out.
  void HandleDeviceAccountError();

  // Handles errors for Secondary Accounts.
  // Displays a notification that allows users to open crOS Account Manager UI.
  // `account_id` is the account identifier (used by the Token Service chain)
  // for the Secondary Account which received an error.
  void HandleSecondaryAccountError(const CoreAccountId& account_id);

  // `account_manager::AccountManager::CheckDummyGaiaTokenForAllAccounts`
  // callback handler.
  void OnCheckDummyGaiaTokenForAllAccounts(
      const std::vector<std::pair<account_manager::Account, bool>>&
          account_dummy_token_list);

  void OnTokenHandleCheck(const AccountId& account_id,
                          TokenHandleUtil::TokenHandleStatus status);

  // Handles clicks on the Secondary Account reauth notification. See
  // `message_center::HandleNotificationClickDelegate`.
  void HandleSecondaryAccountReauthNotificationClick(
      absl::optional<int> button_index);

  std::u16string GetMessageBody(bool is_secondary_account_error) const;

  // The error controller to query for error details.
  SigninErrorController* error_controller_;

  std::unique_ptr<TokenHandleUtil> token_handle_util_;

  // The Profile this service belongs to.
  Profile* const profile_;

  // A non-owning pointer to IdentityManager.
  signin::IdentityManager* const identity_manager_;

  // A non-owning pointer.
  account_manager::AccountManager* const account_manager_;

  // Used to keep track of the message center notifications.
  std::string device_account_notification_id_;
  std::string secondary_account_notification_id_;

  base::WeakPtrFactory<SigninErrorNotifier> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(SigninErrorNotifier);
};

}  // namespace ash

// TODO(https://crbug.com/1164001): remove after the //chrome/browser/chromeos
// source migration is finished.
namespace chromeos {
using ::ash::SigninErrorNotifier;
}

#endif  // CHROME_BROWSER_ASH_LOGIN_SIGNIN_SIGNIN_ERROR_NOTIFIER_H_
