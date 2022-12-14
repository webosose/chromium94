// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEB_APPLICATIONS_EXTERNALLY_MANAGED_APP_REGISTRATION_TASK_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_EXTERNALLY_MANAGED_APP_REGISTRATION_TASK_H_

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list_types.h"
#include "base/timer/timer.h"
#include "chrome/browser/web_applications/components/web_app_url_loader.h"
#include "content/public/browser/service_worker_context_observer.h"

class GURL;

namespace content {
enum class ServiceWorkerCapability;
class ServiceWorkerContext;
class WebContents;
}  // namespace content

namespace web_app {

enum class RegistrationResultCode;
class WebAppUrlLoader;

class ExternallyManagedAppRegistrationTaskBase
    : public content::ServiceWorkerContextObserver {
 public:
  ~ExternallyManagedAppRegistrationTaskBase() override;

  const GURL& install_url() const { return install_url_; }

 protected:
  explicit ExternallyManagedAppRegistrationTaskBase(const GURL& install_url);

 private:
  const GURL install_url_;
};

class ExternallyManagedAppRegistrationTask
    : public ExternallyManagedAppRegistrationTaskBase {
 public:
  using RegistrationCallback = base::OnceCallback<void(RegistrationResultCode)>;

  ExternallyManagedAppRegistrationTask(const GURL& install_url,
                                       WebAppUrlLoader* url_loader,
                                       content::WebContents* web_contents,
                                       RegistrationCallback callback);
  ExternallyManagedAppRegistrationTask(
      const ExternallyManagedAppRegistrationTask&) = delete;
  ExternallyManagedAppRegistrationTask& operator=(
      const ExternallyManagedAppRegistrationTask&) = delete;
  ~ExternallyManagedAppRegistrationTask() override;

  // ServiceWorkerContextObserver:
  void OnRegistrationCompleted(const GURL& scope) override;
  void OnDestruct(content::ServiceWorkerContext* context) override;

  static void SetTimeoutForTesting(int registration_timeout_in_seconds);

 private:
  void OnDidCheckHasServiceWorker(content::ServiceWorkerCapability capability);

  void OnWebContentsReady(WebAppUrlLoader::Result result);

  void OnRegistrationTimeout();

  WebAppUrlLoader* const url_loader_;
  content::WebContents* const web_contents_;
  RegistrationCallback callback_;
  content::ServiceWorkerContext* service_worker_context_;

  base::OneShotTimer registration_timer_;

  base::WeakPtrFactory<ExternallyManagedAppRegistrationTask> weak_ptr_factory_{
      this};

  static int registration_timeout_in_seconds_;
};

}  // namespace web_app

#endif  // CHROME_BROWSER_WEB_APPLICATIONS_EXTERNALLY_MANAGED_APP_REGISTRATION_TASK_H_
