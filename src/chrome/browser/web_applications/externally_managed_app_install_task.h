// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEB_APPLICATIONS_EXTERNALLY_MANAGED_APP_INSTALL_TASK_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_EXTERNALLY_MANAGED_APP_INSTALL_TASK_H_

#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/web_applications/components/external_install_options.h"
#include "chrome/browser/web_applications/components/externally_installed_web_app_prefs.h"
#include "chrome/browser/web_applications/components/web_app_id.h"
#include "chrome/browser/web_applications/components/web_app_install_utils.h"
#include "chrome/browser/web_applications/components/web_app_url_loader.h"
#include "chrome/browser/web_applications/components/web_application_info.h"
#include "chrome/browser/web_applications/externally_managed_app_manager.h"
#include "chrome/browser/web_applications/os_integration_manager.h"
#include "chrome/browser/web_applications/web_app_registrar.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class Profile;

namespace content {
class WebContents;
}

namespace web_app {

class WebAppUrlLoader;
class OsIntegrationManager;
class WebAppInstallFinalizer;
class WebAppInstallManager;
class WebAppUiManager;
enum class InstallResultCode;

// Class to install WebApp from a WebContents. A queue of such tasks is owned by
// ExternallyManagedAppManager. Can only be called from the UI thread.
class ExternallyManagedAppInstallTask {
 public:
  using ResultCallback = base::OnceCallback<void(
      absl::optional<AppId> app_id,
      ExternallyManagedAppManager::InstallResult result)>;

  // Ensures the tab helpers necessary for installing an app are present.
  static void CreateTabHelpers(content::WebContents* web_contents);

  // Constructs a task that will install a BookmarkApp-based Shortcut or Web App
  // for |profile|. |install_options| will be used to decide some of the
  // properties of the installed app e.g. open in a tab vs. window, installed by
  // policy, etc.
  explicit ExternallyManagedAppInstallTask(
      Profile* profile,
      WebAppUrlLoader* url_loader,
      WebAppRegistrar* registrar,
      OsIntegrationManager* os_integration_manager,
      WebAppUiManager* ui_manager,
      WebAppInstallFinalizer* install_finalizer,
      WebAppInstallManager* install_manager,
      ExternalInstallOptions install_options);

  ExternallyManagedAppInstallTask(const ExternallyManagedAppInstallTask&) =
      delete;
  ExternallyManagedAppInstallTask& operator=(
      const ExternallyManagedAppInstallTask&) = delete;

  virtual ~ExternallyManagedAppInstallTask();

  // Temporarily takes a |load_url_result| to decide if a placeholder app should
  // be installed.
  // TODO(ortuno): Remove once loading is done inside the task.
  virtual void Install(content::WebContents* web_contents,
                       ResultCallback result_callback);

  const ExternalInstallOptions& install_options() { return install_options_; }

 private:
  // Install directly from a fully specified WebApplicationInfo struct. Used
  // by system apps.
  void InstallFromInfo(ResultCallback result_callback);

  void OnWebContentsReady(content::WebContents* web_contents,
                          ResultCallback result_callback,
                          WebAppUrlLoader::Result prepare_for_load_result);
  void OnUrlLoaded(content::WebContents* web_contents,
                   ResultCallback result_callback,
                   WebAppUrlLoader::Result load_url_result);

  void InstallPlaceholder(ResultCallback result_callback);

  void UninstallPlaceholderApp(content::WebContents* web_contents,
                               ResultCallback result_callback);
  void OnPlaceholderUninstalled(content::WebContents* web_contents,
                                ResultCallback result_callback,
                                bool uninstalled);
  void ContinueWebAppInstall(content::WebContents* web_contents,
                             ResultCallback result_callback);
  void OnWebAppInstalled(bool is_placeholder,
                         bool offline_install,
                         ResultCallback result_callback,
                         const AppId& app_id,
                         InstallResultCode code);
  void TryAppInfoFactoryOnFailure(
      ResultCallback result_callback,
      absl::optional<AppId> app_id,
      ExternallyManagedAppManager::InstallResult result);
  void OnOsHooksCreated(const AppId& app_id,
                        base::ScopedClosureRunner scoped_closure,
                        const OsHooksResults os_hooks_results);

  Profile* const profile_;
  WebAppUrlLoader* const url_loader_;
  WebAppRegistrar* const registrar_;
  OsIntegrationManager* const os_integration_manager_;
  WebAppInstallFinalizer* const install_finalizer_;
  WebAppInstallManager* const install_manager_;
  WebAppUiManager* const ui_manager_;

  ExternallyInstalledWebAppPrefs externally_installed_app_prefs_;

  const ExternalInstallOptions install_options_;

  base::WeakPtrFactory<ExternallyManagedAppInstallTask> weak_ptr_factory_{this};
};

}  // namespace web_app

#endif  // CHROME_BROWSER_WEB_APPLICATIONS_EXTERNALLY_MANAGED_APP_INSTALL_TASK_H_
