// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/full_restore/full_restore_service.h"

#include "ash/constants/ash_switches.h"
#include "ash/public/cpp/notification_utils.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/string_util.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ash/full_restore/full_restore_app_launch_handler.h"
#include "chrome/browser/ash/full_restore/full_restore_data_handler.h"
#include "chrome/browser/ash/full_restore/full_restore_prefs.h"
#include "chrome/browser/ash/full_restore/full_restore_service_factory.h"
#include "chrome/browser/ash/full_restore/new_user_restore_pref_handler.h"
#include "chrome/browser/ash/profiles/profile_helper.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/settings_window_manager_chromeos.h"
#include "chrome/browser/ui/webui/settings/chromeos/constants/routes.mojom.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/account_id/account_id.h"
#include "components/full_restore/full_restore_info.h"
#include "components/full_restore/full_restore_save_handler.h"
#include "components/full_restore/full_restore_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/chromeos/devicetype_utils.h"
#include "ui/message_center/public/cpp/notification.h"

namespace ash {
namespace full_restore {

bool g_restore_for_testing = true;

const char kRestoreForCrashNotificationId[] = "restore_for_crash_notification";
const char kRestoreNotificationId[] = "restore_notification";

const char kRestoreNotificationHistogramName[] = "Apps.RestoreNotification";
const char kRestoreForCrashNotificationHistogramName[] =
    "Apps.RestoreForCrashNotification";
const char kRestoreSettingHistogramName[] = "Apps.RestoreSetting";
const char kRestoreInitSettingHistogramName[] = "Apps.RestoreInitSetting";

constexpr char kWindowCountHistogramPrefix[] = "Apps.WindowCount.";
constexpr char kRestoreHistogramSuffix[] = "Restore";
constexpr char kNotRestoreHistogramSuffix[] = "NotRestore";
constexpr char kCloseByUserHistogramSuffix[] = "CloseByUser";
constexpr char kCloseNotByUserHistogramSuffix[] = "CloseNotByUser";

// static
FullRestoreService* FullRestoreService::GetForProfile(Profile* profile) {
  return static_cast<FullRestoreService*>(
      FullRestoreServiceFactory::GetInstance()->GetForProfile(profile));
}

// static
void FullRestoreService::MaybeCloseNotification(Profile* profile) {
  auto* full_restore_service =
      ash::full_restore::FullRestoreService::GetForProfile(profile);
  if (full_restore_service)
    full_restore_service->MaybeCloseNotification();
}

FullRestoreService::FullRestoreService(Profile* profile)
    : profile_(profile),
      app_launch_handler_(std::make_unique<FullRestoreAppLaunchHandler>(
          profile_,
          /*should_init_service=*/true)),
      restore_data_handler_(
          std::make_unique<FullRestoreDataHandler>(profile_)) {
  notification_registrar_.Add(this, chrome::NOTIFICATION_APP_TERMINATING,
                              content::NotificationService::AllSources());

  PrefService* prefs = profile_->GetPrefs();
  DCHECK(prefs);

  pref_change_registrar_.Init(prefs);
  pref_change_registrar_.Add(
      kRestoreAppsAndPagesPrefName,
      base::BindRepeating(&FullRestoreService::OnPreferenceChanged,
                          weak_ptr_factory_.GetWeakPtr()));

  const user_manager::User* user =
      ProfileHelper::Get()->GetUserByProfile(profile_);
  if (user) {
    ::full_restore::FullRestoreInfo::GetInstance()->SetRestorePref(
        user->GetAccountId(), CanPerformRestore(prefs));
  }

  // Set profile path before init the restore process to create
  // FullRestoreSaveHandler to observe restore windows.
  if (ProfileHelper::Get()->GetUserByProfile(profile_) ==
      user_manager::UserManager::Get()->GetPrimaryUser()) {
    ::full_restore::FullRestoreSaveHandler::GetInstance()
        ->SetPrimaryProfilePath(profile_->GetPath());

    // In Multi-Profile mode, only set for the primary user. For other users,
    // active profile path is set when switch users.
    ::full_restore::SetActiveProfilePath(profile_->GetPath());

    can_be_inited_ = CanBeInited();
  }

  if (!HasRestorePref(prefs) && HasSessionStartupPref(prefs)) {
    // If there is no full restore pref, but there is a session restore setting,
    // set the first run flag to maintain the previous behavior for the first
    // time running the full restore feature when migrate to the full restore
    // release. Restore browsers and web apps by the browser session restore.
    first_run_full_restore_ = true;
    SetDefaultRestorePrefIfNecessary(prefs);
    ::full_restore::FullRestoreSaveHandler::GetInstance()->AllowSave();
    VLOG(1) << "No restore pref! First time to run full restore."
            << profile_->GetPath();
  }
}

FullRestoreService::~FullRestoreService() = default;

void FullRestoreService::Init() {
  // If it is the first time to migrate to the full restore release, we don't
  // have other restore data, so we don't need to consider restoration.
  if (first_run_full_restore_)
    return;

  // If the user of `profile_` is not the primary user, and hasn't been the
  // active user yet, we don't need to consider restoration to prevent the
  // restored windows are written to the active user's profile path.
  if (!can_be_inited_)
    return;

  // If the restore ddata has not been loaded, wait for it.
  if (!app_launch_handler_->IsRestoreDataLoaded())
    return;

  if (is_shut_down_)
    return;

  PrefService* prefs = profile_->GetPrefs();
  DCHECK(prefs);

  // If the system crashed before reboot, show the restore notification.
  if (profile_->GetLastSessionExitType() == Profile::EXIT_CRASHED) {
    if (!HasRestorePref(prefs))
      SetDefaultRestorePrefIfNecessary(prefs);

    MaybeShowRestoreNotification(kRestoreForCrashNotificationId);
    return;
  }

  // If either OS pref setting nor Chrome pref setting exist, that means we
  // don't have restore data, so we don't need to consider restoration, and call
  // NewUserRestorePrefHandler to set OS pref setting.
  if (!HasRestorePref(prefs) && !HasSessionStartupPref(prefs)) {
    new_user_pref_handler_ =
        std::make_unique<NewUserRestorePrefHandler>(profile_);
    ::full_restore::FullRestoreSaveHandler::GetInstance()->AllowSave();
    return;
  }

  RestoreOption restore_pref = static_cast<RestoreOption>(
      prefs->GetInteger(kRestoreAppsAndPagesPrefName));
  base::UmaHistogramEnumeration(kRestoreInitSettingHistogramName, restore_pref);
  switch (restore_pref) {
    case RestoreOption::kAlways:
      Restore();
      break;
    case RestoreOption::kAskEveryTime:
      MaybeShowRestoreNotification(kRestoreNotificationId);
      break;
    case RestoreOption::kDoNotRestore:
      ::full_restore::FullRestoreSaveHandler::GetInstance()->AllowSave();
      return;
  }
}

void FullRestoreService::OnTransitionedToNewActiveUser(Profile* profile) {
  const bool already_initialized = can_be_inited_;
  if (profile_ != profile || already_initialized)
    return;

  can_be_inited_ = true;
  Init();
}

void FullRestoreService::LaunchBrowserWhenReady() {
  if (!g_restore_for_testing || !app_launch_handler_)
    return;

  app_launch_handler_->LaunchBrowserWhenReady(first_run_full_restore_);
}

void FullRestoreService::MaybeCloseNotification(bool allow_save) {
  close_notification_ = true;
  VLOG(1) << "The full restore notification is closed for "
          << profile_->GetPath();

  if (notification_ != nullptr && !is_shut_down_) {
    NotificationDisplayService::GetForProfile(profile_)->Close(
        NotificationHandler::Type::TRANSIENT, notification_->id());
    accelerator_controller_observer_.Reset();
  }

  if (allow_save) {
    // If the user launches an app or clicks the cancel button, start the save
    // timer.
    ::full_restore::FullRestoreSaveHandler::GetInstance()->AllowSave();
  }
}

void FullRestoreService::Close(bool by_user) {
  if (!skip_notification_histogram_) {
    RecordRestoreAction(
        notification_->id(),
        by_user ? RestoreAction::kCloseByUser : RestoreAction::kCloseNotByUser);
    RecordWindowCount(by_user ? kCloseByUserHistogramSuffix
                              : kCloseNotByUserHistogramSuffix);
  }
  notification_ = nullptr;

  if (by_user) {
    // If the user closes the notification, start the save timer. If it is not
    // closed by the user, the restore button might be clicked, then we need to
    // wait for the restore finish to start the save timer.
    ::full_restore::FullRestoreSaveHandler::GetInstance()->AllowSave();
  }
}

void FullRestoreService::Click(const absl::optional<int>& button_index,
                               const absl::optional<std::u16string>& reply) {
  DCHECK(notification_);
  skip_notification_histogram_ = true;

  if (!button_index.has_value() ||
      button_index.value() ==
          static_cast<int>(RestoreNotificationButtonIndex::kRestore)) {
    VLOG(1) << "The restore notification is clicked for "
            << profile_->GetPath();

    // Restore if the user clicks the notification body.
    RecordRestoreAction(notification_->id(), RestoreAction::kRestore);
    RecordWindowCount(kRestoreHistogramSuffix);
    Restore();

    // If the user selects restore, don't start the save timer. Wait for the
    // restore finish.
    MaybeCloseNotification(/*allow_save=*/false);
    return;
  }

  if (notification_->id() == kRestoreNotificationId) {
    // Show the 'On Startup' OS setting page if the user clicks the settings
    // button of the restore notification.
    chrome::SettingsWindowManager::GetInstance()->ShowOSSettings(
        profile_, chromeos::settings::mojom::kAppsSectionPath);
    return;
  }

  VLOG(1) << "The crash restore notification is canceled for "
          << profile_->GetPath();

  // Close the crash notification if the user clicks the cancel button of the
  // crash notification.
  RecordRestoreAction(notification_->id(), RestoreAction::kCancel);
  RecordWindowCount(kNotRestoreHistogramSuffix);
  MaybeCloseNotification();
}

void FullRestoreService::Observe(int type,
                                 const content::NotificationSource& source,
                                 const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_APP_TERMINATING, type);
  app_launch_handler_.reset();
  ::full_restore::FullRestoreSaveHandler::GetInstance()->SetShutDown();
}

void FullRestoreService::OnActionPerformed(AcceleratorAction action) {
  switch (action) {
    case NEW_INCOGNITO_WINDOW:
    case NEW_TAB:
    case NEW_WINDOW:
    case OPEN_CROSH:
    case OPEN_DIAGNOSTICS:
    case RESTORE_TAB:
      MaybeCloseNotification();
      return;
    default:
      return;
  }
}

void FullRestoreService::OnAcceleratorControllerWillBeDestroyed(
    AcceleratorController* controller) {
  accelerator_controller_observer_.Reset();
}

void FullRestoreService::SetAppLaunchHanlderForTesting(
    std::unique_ptr<FullRestoreAppLaunchHandler> app_launch_handler) {
  app_launch_handler_ = std::move(app_launch_handler);
}

void FullRestoreService::Shutdown() {
  is_shut_down_ = true;
}

bool FullRestoreService::CanBeInited() {
  auto* user_manager = user_manager::UserManager::Get();
  DCHECK(user_manager);
  DCHECK(user_manager->GetActiveUser());

  // For non-primary user, wait for `OnTransitionedToNewActiveUser`.
  auto* user = ProfileHelper::Get()->GetUserByProfile(profile_);
  if (user != user_manager->GetPrimaryUser()) {
    VLOG(1) << "Can't init full restore service for non_primary user."
            << profile_->GetPath();
    return false;
  }

  // Check the command line to decide whether this is the restart case.
  // `kLoginManager` means starting Chrome with login/oobe screen, not the
  // restart process. For the restart process, `kLoginUser` should be in the
  // command line.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  DCHECK(command_line);
  if (command_line->HasSwitch(switches::kLoginManager) ||
      !command_line->HasSwitch(switches::kLoginUser)) {
    return true;
  }

  // When the system restarts, and the active user in the previous session is
  // not the primary user, don't init, but wait for the transition to the last
  // active user.
  const auto& last_session_active_account_id =
      user_manager->GetLastSessionActiveAccountId();
  if (last_session_active_account_id.is_valid() &&
      AccountId::FromUserEmail(user->GetAccountId().GetUserEmail()) !=
          last_session_active_account_id) {
    VLOG(1) << "Can't init full restore service for non-active primary user."
            << profile_->GetPath();
    return false;
  }

  return true;
}

void FullRestoreService::MaybeShowRestoreNotification(const std::string& id) {
  if (!ShouldShowNotification())
    return;

  auto* accelerator_controller = ash::AcceleratorController::Get();
  if (accelerator_controller) {
    DCHECK(!accelerator_controller_observer_.IsObserving());
    accelerator_controller_observer_.Observe(accelerator_controller);
  }

  message_center::RichNotificationData notification_data;

  message_center::ButtonInfo restore_button(
      l10n_util::GetStringUTF16(IDS_RESTORE_NOTIFICATION_RESTORE_BUTTON));
  notification_data.buttons.push_back(restore_button);

  int button_id;
  if (id == kRestoreForCrashNotificationId)
    button_id = IDS_RESTORE_NOTIFICATION_CANCEL_BUTTON;
  else
    button_id = IDS_RESTORE_NOTIFICATION_SETTINGS_BUTTON;
  message_center::ButtonInfo cancel_button(
      l10n_util::GetStringUTF16(button_id));
  notification_data.buttons.push_back(cancel_button);

  std::u16string title;
  if (id == kRestoreForCrashNotificationId) {
    title = l10n_util::GetStringFUTF16(IDS_RESTORE_CRASH_NOTIFICATION_TITLE,
                                       ui::GetChromeOSDeviceName());
    VLOG(1) << "Show the restore notification for crash for "
            << profile_->GetPath();
  } else {
    title = l10n_util::GetStringUTF16(IDS_RESTORE_NOTIFICATION_TITLE);
    VLOG(1) << "Show the restore notification for the normal startup for "
            << profile_->GetPath();
  }

  int message_id;
  if (id == kRestoreForCrashNotificationId)
    message_id = IDS_RESTORE_CRASH_NOTIFICATION_MESSAGE;
  else
    message_id = IDS_RESTORE_NOTIFICATION_MESSAGE;

  notification_ = ash::CreateSystemNotification(
      message_center::NOTIFICATION_TYPE_SIMPLE, id, title,
      l10n_util::GetStringUTF16(message_id),
      l10n_util::GetStringUTF16(IDS_RESTORE_NOTIFICATION_DISPLAY_SOURCE),
      GURL(),
      message_center::NotifierId(message_center::NotifierType::SYSTEM_COMPONENT,
                                 id),
      notification_data,
      base::MakeRefCounted<message_center::ThunkNotificationDelegate>(
          weak_ptr_factory_.GetWeakPtr()),
      kFullRestoreNotificationIcon,
      message_center::SystemNotificationWarningLevel::NORMAL);
  notification_->set_priority(message_center::SYSTEM_PRIORITY);

  auto* notification_display_service =
      NotificationDisplayService::GetForProfile(profile_);
  DCHECK(notification_display_service);
  notification_display_service->Display(NotificationHandler::Type::TRANSIENT,
                                        *notification_,
                                        /*metadata=*/nullptr);
}

void FullRestoreService::Restore() {
  const user_manager::User* user =
      ProfileHelper::Get()->GetUserByProfile(profile_);
  if (user) {
    ::full_restore::FullRestoreInfo::GetInstance()->SetRestoreFlag(
        user->GetAccountId(), true);
  }

  if (app_launch_handler_)
    app_launch_handler_->SetShouldRestore();
}

void FullRestoreService::RecordRestoreAction(const std::string& notification_id,
                                             RestoreAction restore_action) {
  base::UmaHistogramEnumeration(notification_id == kRestoreNotificationId
                                    ? kRestoreNotificationHistogramName
                                    : kRestoreForCrashNotificationHistogramName,
                                restore_action);
}

void FullRestoreService::OnPreferenceChanged(const std::string& pref_name) {
  DCHECK_EQ(pref_name, kRestoreAppsAndPagesPrefName);

  RestoreOption restore_option = static_cast<RestoreOption>(
      profile_->GetPrefs()->GetInteger(kRestoreAppsAndPagesPrefName));
  base::UmaHistogramEnumeration(kRestoreSettingHistogramName, restore_option);

  const user_manager::User* user =
      ProfileHelper::Get()->GetUserByProfile(profile_);
  if (user) {
    ::full_restore::FullRestoreInfo::GetInstance()->SetRestorePref(
        user->GetAccountId(), CanPerformRestore(profile_->GetPrefs()));
  }
}

bool FullRestoreService::ShouldShowNotification() {
  return app_launch_handler_ && app_launch_handler_->HasRestoreData() &&
         !::first_run::IsChromeFirstRun() && !close_notification_;
}

void FullRestoreService::RecordWindowCount(const std::string& restore_action) {
  base::UmaHistogramCounts100(
      kWindowCountHistogramPrefix + restore_action,
      ::full_restore::FullRestoreSaveHandler::GetInstance()->window_count());
}

ScopedRestoreForTesting::ScopedRestoreForTesting() {
  g_restore_for_testing = false;
}

ScopedRestoreForTesting::~ScopedRestoreForTesting() {
  g_restore_for_testing = true;
}

}  // namespace full_restore
}  // namespace ash
