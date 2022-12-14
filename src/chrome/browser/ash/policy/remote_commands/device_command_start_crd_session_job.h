// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_POLICY_REMOTE_COMMANDS_DEVICE_COMMAND_START_CRD_SESSION_JOB_H_
#define CHROME_BROWSER_ASH_POLICY_REMOTE_COMMANDS_DEVICE_COMMAND_START_CRD_SESSION_JOB_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "components/policy/core/common/remote_commands/remote_command_job.h"

class DeviceOAuth2TokenService;

namespace policy {

class CrdLockoutStrategy;

// Remote command that would start Chrome Remote Desktop host and return auth
// code. This command is usable only for devices running Kiosk sessions.
class DeviceCommandStartCRDSessionJob : public RemoteCommandJob {
 public:
  enum ResultCode {
    // Successfully obtained access code.
    SUCCESS = 0,

    // Failed as required services are not launched on the device.
    FAILURE_SERVICES_NOT_READY = 1,

    // Failure as the current user type does not support remotely starting CRD.
    FAILURE_UNSUPPORTED_USER_TYPE = 2,

    // Failed as device is currently in use and no interruptUser flag is set.
    FAILURE_NOT_IDLE = 3,

    // Failed as we could not get OAuth token for whatever reason.
    FAILURE_NO_OAUTH_TOKEN = 4,

    // Failed as we could not get ICE configuration for whatever reason.
    // deprecated FAILURE_NO_ICE_CONFIG = 5,

    // Failure during attempt to start CRD host and obtain CRD token.
    FAILURE_CRD_HOST_ERROR = 6,

    // Failed as the host user declined the connection too many times in a row.
    // The |CrdLockoutStrategy| controls when this happens.
    FAILURE_TOO_MANY_REJECTED_ATTEMPTS = 7,
  };

  using OAuthTokenCallback = base::OnceCallback<void(const std::string&)>;
  using AccessCodeCallback = base::OnceCallback<void(const std::string&)>;
  using ErrorCallback =
      base::OnceCallback<void(ResultCode, const std::string&)>;

  // Delegate that will start a session with the CRD native host.
  class Delegate {
   public:
    // Session parameters used to start the CRD host.
    struct SessionParameters {
      std::string oauth_token = "";
      std::string user_name = "";
      bool terminate_upon_input = false;
      bool show_confirmation_dialog = false;
    };

    virtual ~Delegate() = default;

    // Check if there exists an active CRD session.
    virtual bool HasActiveSession() const = 0;

    // Run |callback| once active CRD session is terminated.
    virtual void TerminateSession(base::OnceClosure callback) = 0;

    // Attempts to start CRD host and get Auth Code.
    virtual void StartCRDHostAndGetCode(const SessionParameters& parameters,
                                        AccessCodeCallback success_callback,
                                        ErrorCallback error_callback) = 0;
  };

  DeviceCommandStartCRDSessionJob(Delegate* crd_host_delegate,
                                  CrdLockoutStrategy* lockout_strategy);
  ~DeviceCommandStartCRDSessionJob() override;

  // RemoteCommandJob:
  enterprise_management::RemoteCommand_Type GetType() const override;

  // Set a Fake OAuth token that will be used once the next time we need to
  // fetch an oauth token.
  void SetOAuthTokenForTest(const std::string& token);

 protected:
  // RemoteCommandJob:
  bool ParseCommandPayload(const std::string& command_payload) override;
  void RunImpl(CallbackWithResult succeeded_callback,
               CallbackWithResult failed_callback) override;
  void TerminateImpl() override;

 private:
  class OAuthTokenFetcher;
  class ResultPayload;

  enum class UserType {
    kAutoLaunchedKiosk,
    kNonAutoLaunchedKiosk,
    kNoUser,
    kAffiliatedUser,
    kManagedGuestSession,
    kOther,
  };
  const char* UserTypeToString(UserType value) const;

  // Check if all required system services (singletons) are ready.
  bool AreServicesReady() const;
  bool UserTypeSupportsCRD() const;
  UserType GetUserType() const;
  bool IsRunningAutoLaunchedKiosk() const;
  bool IsDeviceIdle() const;
  base::TimeDelta GetDeviceIdlenessPeriod() const;

  void FetchOAuthTokenASync(OAuthTokenCallback on_success,
                            ErrorCallback on_error);

  // Finishes command with error code and optional message.
  void FinishWithError(ResultCode result_code, const std::string& message);
  void FinishWithNotIdleError();

  void OnOAuthTokenReceived(const std::string& token);
  void OnAccessCodeReceived(const std::string& access_code);

  std::string GetRobotAccountUserName() const;

  bool ShouldShowConfirmationDialog() const;
  bool ShouldUseEnterpriseUserDialog() const;

  DeviceOAuth2TokenService* oauth_service() const;

  std::unique_ptr<OAuthTokenFetcher> oauth_token_fetcher_;

  // The callback that will be called when the access code was successfully
  // obtained.
  CallbackWithResult succeeded_callback_;

  // The callback that will be called when this command failed.
  CallbackWithResult failed_callback_;

  // -- Command parameters --

  // Defines whether connection attempt to active user should succeed or fail.
  base::TimeDelta idleness_cutoff_;

  // Defines if CRD session should be terminated upon any input event from local
  // user.
  bool terminate_upon_input_ = false;

  // Fake OAuth token that will be used once the next time we need to fetch an
  // oauth token.
  absl::optional<std::string> oauth_token_for_test_;

  // The Delegate is used to interact with chrome services and CRD host.
  // Owned by DeviceCommandsFactoryAsh.
  Delegate* const delegate_;
  // Owned by DeviceCommandsFactoryChromeOS.
  CrdLockoutStrategy* const lockout_strategy_;

  bool terminate_session_attempted_ = false;

  base::WeakPtrFactory<DeviceCommandStartCRDSessionJob> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(DeviceCommandStartCRDSessionJob);
};

}  // namespace policy

#endif  // CHROME_BROWSER_ASH_POLICY_REMOTE_COMMANDS_DEVICE_COMMAND_START_CRD_SESSION_JOB_H_
