// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UKM_APP_SOURCE_URL_RECORDER_H_
#define COMPONENTS_UKM_APP_SOURCE_URL_RECORDER_H_

#include "services/metrics/public/cpp/ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

#include "base/feature_list.h"

#include <string>

class GURL;

namespace apps {
class AppPlatformMetrics;
}

namespace app_list {
class AppLaunchEventLogger;
}  // namespace app_list

namespace badging {
class BadgeManager;
}  // namespace badging
namespace ukm {

const base::Feature kUkmAppLogging{"UkmAppLogging",
                                   base::FEATURE_ENABLED_BY_DEFAULT};

class AppSourceUrlRecorder {
 private:
  friend class apps::AppPlatformMetrics;

  friend class AppSourceUrlRecorderTest;

  friend class app_list::AppLaunchEventLogger;

  friend class badging::BadgeManager;

  // Get a UKM SourceId with the prefix "app://" for a Chrome app with `app_id`,
  // a unique hash string to identify the app. For example,
  // "mgndgikekgjfcpckkfioiadnlibdjbkf" is the `app_id` for Chrome browser, and
  // the output SourceId is "app://mgndgikekgjfcpckkfioiadnlibdjbkf".
  static SourceId GetSourceIdForChromeApp(const std::string& app_id);

  // Get a UKM SourceId with the prefix "chrome-extension://" for a Chrome
  // extension. For example, for `id`, "mhjfbmdgcfjbbpaeojofohoefgiehjai", the
  // output SourceId is "chrome-extension://mhjfbmdgcfjbbpaeojofohoefgiehjai".
  static SourceId GetSourceIdForChromeExtension(const std::string& id);

  // Get a UKM SourceId with the prefix "app://" for an Arc app with
  // `package_name`. For example, for `package_name`, "com.google.play", the
  // output SourceId is "app://com.google.play".
  static SourceId GetSourceIdForArcPackageName(const std::string& package_name);

  // Get a UKM SourceId with the prefix "app://" for an Arc app with a hash
  // string for `package_name`. For example, for `package_name`,
  // "com.google.play", the output SourceId is
  // "app://play/pjhgmeephkiehhlkfcoginnkbphkdang".
  static SourceId GetSourceIdForArc(const std::string& package_name);

  // Get a UKM SourceId for a PWA or bookmark app.
  static SourceId GetSourceIdForPWA(const GURL& url);

  // For internal use only.
  static SourceId GetSourceIdForUrl(const GURL& url, const AppType);

  // Informs UKM service that the source_id is no longer needed nor used by the
  // end of the current reporting cycle, and thus can be deleted later.
  static void MarkSourceForDeletion(SourceId source_id);
};

}  // namespace ukm

#endif  // COMPONENTS_UKM_APP_SOURCE_URL_RECORDER_H_
