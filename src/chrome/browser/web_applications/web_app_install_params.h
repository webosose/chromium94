// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_INSTALL_PARAMS_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_INSTALL_PARAMS_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "chrome/browser/web_applications/components/web_app_constants.h"
#include "chrome/browser/web_applications/components/web_app_id.h"
#include "chrome/browser/web_applications/components/web_app_install_utils.h"
#include "chrome/browser/web_applications/system_web_apps/system_web_app_types.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

struct WebApplicationInfo;

namespace content {
class WebContents;
}  // namespace content

namespace web_app {

// |app_id| may be empty on failure.
using OnceInstallCallback =
    base::OnceCallback<void(const AppId& app_id, InstallResultCode code)>;
using OnceUninstallCallback =
    base::OnceCallback<void(const AppId& app_id, bool uninstalled)>;

// Callback used to indicate whether a user has accepted the installation of a
// web app.
using WebAppInstallationAcceptanceCallback =
    base::OnceCallback<void(bool user_accepted,
                            std::unique_ptr<WebApplicationInfo>)>;

// Callback to show the WebApp installation confirmation bubble in UI.
// |web_app_info| is the WebApplicationInfo to be installed.
using WebAppInstallDialogCallback = base::OnceCallback<void(
    content::WebContents* initiator_web_contents,
    std::unique_ptr<WebApplicationInfo> web_app_info,
    ForInstallableSite for_installable_site,
    WebAppInstallationAcceptanceCallback acceptance_callback)>;

enum class InstallableCheckResult {
  kNotInstallable,
  kInstallable,
  kAlreadyInstalled,
};
// Callback with the result of manifest check.
// |web_contents| owns the WebContents that was used to check for a manifest.
// |app_id| will be present iff already installed.
using WebAppManifestCheckCallback =
    base::OnceCallback<void(std::unique_ptr<content::WebContents> web_contents,
                            InstallableCheckResult result,
                            absl::optional<AppId> app_id)>;

// See related ExternalInstallOptions struct and
// ConvertExternalInstallOptionsToParams function.
struct WebAppInstallParams {
  WebAppInstallParams();
  ~WebAppInstallParams();
  WebAppInstallParams(const WebAppInstallParams&);

  DisplayMode user_display_mode = DisplayMode::kUndefined;

  // URL to be used as start_url if manifest is unavailable.
  GURL fallback_start_url;

  // Setting this field will force the webapp to have a manifest id, which
  // will result in a different AppId than if it isn't set. Currently here
  // to support forwards compatibility with future sync entities..
  absl::optional<std::string> override_manifest_id;

  // App name to be used if manifest is unavailable.
  absl::optional<std::u16string> fallback_app_name;

  bool locally_installed = true;
  // These OS shortcut fields can't be true if |locally_installed| is false.
  bool add_to_applications_menu = true;
  bool add_to_desktop = true;
  bool add_to_quick_launch_bar = true;
  bool run_on_os_login = false;

  // These have no effect outside of Chrome OS.
  bool add_to_search = true;
  bool add_to_management = true;
  bool is_disabled = false;

  bool bypass_service_worker_check = false;
  bool require_manifest = false;

  std::vector<std::string> additional_search_terms;

  absl::optional<std::string> launch_query_params;
  absl::optional<SystemAppType> system_app_type;

  bool oem_installed = false;
};

}  // namespace web_app

#endif  // CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_INSTALL_PARAMS_H_
