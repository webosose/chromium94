// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_APPS_APP_SERVICE_PUBLISHERS_STANDALONE_BROWSER_EXTENSION_APPS_H_
#define CHROME_BROWSER_APPS_APP_SERVICE_PUBLISHERS_STANDALONE_BROWSER_EXTENSION_APPS_H_

#include "base/memory/weak_ptr.h"
#include "chromeos/crosapi/mojom/app_service.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/services/app_service/public/cpp/publisher_base.h"
#include "components/services/app_service/public/mojom/app_service.mojom-forward.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"

class Profile;

namespace apps {

// An app publisher (in the App Service sense) for extension-based apps [e.g.
// packaged v2 apps] published by Lacros.
//
// The App Service is responsible for integrating various app platforms [ARC,
// Lacros, Crostini, etc.] with system UI: shelf, launcher, etc. The App Service
// expects publishers to implement a uniform API, roughly translating to:
// GetLoadedApps, LaunchApp, GetIconForApp, etc.
//
// This class implements this functionality for extension based apps running in
// the Lacros context. Aforementioned API is implemented using crosapi to
// communicate between this class and Lacros.
//
// The current implementation of this class relies on the assumption that Lacros
// is almost always running in the background. This is enforced via
// ScopedKeepAlive. We would like to eventually remove this assumption. This
// requires caching a copy of installed apps in this class.
class StandaloneBrowserExtensionApps : public KeyedService,
                                       public apps::PublisherBase,
                                       public crosapi::mojom::AppPublisher {
 public:
  explicit StandaloneBrowserExtensionApps(Profile* profile);
  ~StandaloneBrowserExtensionApps() override;

  StandaloneBrowserExtensionApps(const StandaloneBrowserExtensionApps&) =
      delete;
  StandaloneBrowserExtensionApps& operator=(
      const StandaloneBrowserExtensionApps&) = delete;

  // Register the chrome apps host from lacros-chrome to allow lacros-chrome
  // to publish chrome apps to the app service in ash-chrome.
  void RegisterChromeAppsCrosapiHost(
      mojo::PendingReceiver<crosapi::mojom::AppPublisher> receiver);

 private:
  // apps::PublisherBase:
  void Connect(mojo::PendingRemote<apps::mojom::Subscriber> subscriber_remote,
               apps::mojom::ConnectOptionsPtr opts) override;
  void LoadIcon(const std::string& app_id,
                apps::mojom::IconKeyPtr icon_key,
                apps::mojom::IconType icon_type,
                int32_t size_hint_in_dip,
                bool allow_placeholder_icon,
                LoadIconCallback callback) override;
  void Launch(const std::string& app_id,
              int32_t event_flags,
              apps::mojom::LaunchSource launch_source,
              apps::mojom::WindowInfoPtr window_info) override;
  void GetMenuModel(const std::string& app_id,
                    apps::mojom::MenuType menu_type,
                    int64_t display_id,
                    GetMenuModelCallback callback) override;

  // crosapi::mojom::AppPublisher overrides.
  void OnApps(std::vector<apps::mojom::AppPtr> deltas) override;
  void RegisterAppController(
      mojo::PendingRemote<crosapi::mojom::AppController> controller) override;
  void OnCapabilityAccesses(
      std::vector<apps::mojom::CapabilityAccessPtr> deltas) override;

  // Called when the crosapi termination is terminated [e.g. Lacros is closed].
  // The ordering of these two disconnect methods is non-deterministic. When
  // either is called we close both connections since this class requires both
  // to be functional.
  void OnReceiverDisconnected();
  void OnControllerDisconnected();

  mojo::RemoteSet<apps::mojom::Subscriber> subscribers_;

  // Receives chrome app publisher events from Lacros.
  mojo::Receiver<crosapi::mojom::AppPublisher> receiver_{this};

  // Used to send chrome app publisher actions to Lacros.
  mojo::Remote<crosapi::mojom::AppController> controller_;

  base::WeakPtrFactory<StandaloneBrowserExtensionApps> weak_factory_{this};
};

}  // namespace apps

#endif  // CHROME_BROWSER_APPS_APP_SERVICE_PUBLISHERS_STANDALONE_BROWSER_EXTENSION_APPS_H_
