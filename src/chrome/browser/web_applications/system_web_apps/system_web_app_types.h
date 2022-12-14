// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEB_APPLICATIONS_SYSTEM_WEB_APPS_SYSTEM_WEB_APP_TYPES_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_SYSTEM_WEB_APPS_SYSTEM_WEB_APP_TYPES_H_

namespace web_app {

// An enum that lists the different System Apps that exist. Can be used to
// retrieve the App ID from the underlying Web App system.
//
// These values are persisted to the web_app database. Entries should not be
// renumbered and numeric values should never be reused.
//
// When deprecating, comment out the entry so that it's not accidentally
// re-used.
enum class SystemAppType {
  FILE_MANAGER = 1,
  TELEMETRY = 2,

  // A sample System Web App to illustrate SWA development best practices, and
  // various SWA platform features.
  //
  // This App is only enabled on non-official builds. You can find a brief SWA
  // platform introduction (Google internal) at: http://go/system-web-apps.
  //
  // Source: //ash/webui/sample_system_web_app_ui/
  // Contact: dominicshulz@google.com, ortuno@chromium.org
  SAMPLE = 3,

  SETTINGS = 4,
  CAMERA = 5,
  TERMINAL = 6,
  MEDIA = 7,
  HELP = 8,
  PRINT_MANAGEMENT = 9,
  SCANNING = 10,
  DIAGNOSTICS = 11,
  CONNECTIVITY_DIAGNOSTICS = 12,
  ECHE = 13,
  CROSH = 14,
  PERSONALIZATION = 15,
  SHORTCUT_CUSTOMIZATION = 16,

  // SHIMLESS RMA Flow is SWA that provides step by step guides for the
  // repair/RMA process.
  //
  // You can find information about this SWA at: http://go/shimless-ux.
  //
  // Source: //ash/webui/shimless_rma/
  // Contact: cros-peripherals@google.com
  SHIMLESS_RMA = 17,

  // A System Web App that launches on Demo Mode startup, to display animated
  // content that highlights various features of ChromeOS
  //
  // Currently this SWA is only enabled in unofficial builds while still under
  // development. Prefer to file bugs to the internal Demo Mode component:
  // b/components/812312
  //
  // Source: //chromeos/components/demo_mode_app_ui/
  // Contact: jacksontadie@google.com, drcrash@chromium.org
  DEMO_MODE = 18,

  // OS FEEDBACK is a SWA that provides step by step guides to submit a
  // feedback report on Chrome OS.
  //
  // Source: //ash/webui/os_feedback_ui
  // contact: cros-telemetry@google.com
  OS_FEEDBACK = 19,

  // Projector (go/projector-player-dd) aims to make it simple for teachers and
  // students to record and share instructional videos on a Chromebook. This app
  // enables teachers to create a library of custom-tailored instructional
  // content that students can search and view at home.
  //
  // Source: //chromeos/components/projector_app/
  // Contact: cros-projector@google.com
  // Buganizer component: b/components/1080013
  // This app is only included in Chrome-branded builds. Non-official builds
  // will have a mock page.
  PROJECTOR = 20,

  // When adding a new System App, remember to:
  //
  // 1. Add a corresponding histogram suffix in WebAppSystemAppInternalName
  //    (histograms.xml). The suffix name should match the App's
  //    |internal_name|. This is for reporting per-app install results.
  //
  // 2. Add a corresponding proto enum entry (with the same numerical value) to
  //    SystemWebAppDataProto in system_web_app_data.proto. This is for
  //    identifying system apps during Chrome start-up (i.e. when
  //    SystemWebAppManager hasn't finished synchronizing all apps).
  //
  // 3. Add a comment above the enum entry in this file. It should include a
  //    description (what it does in one sentence), at least one email contact,
  //    source location (if it's in chromium source tree), and other relevant
  //    information.
  //
  //    Other relevant information should come in separate paragraphs after the
  //    description. This can be anything useful for triaging or routing bugs.
  //    For example, your team doesn't use chromium's bug tracker, the App is
  //    only available on certain devices.
  //
  //    Source location should point to where the App's WebUIController is
  //    defined. It doesn't have to include the complete source repository (e.g.
  //    if the App is hosted in internal repositories).
  //
  // 4. Put a blank line after each enum (before next enum's comment).
  //
  // 5. Use web_app::LaunchSystemWebAppAsync to launch your SWA (with the type
  //    added above). This provides extra safety in edge cases (e.g. when in
  //    incognito or guest sessions).
  //
  // 6. Update kMaxValue.
  //
  // 7. Have one of System Web App Platform owners review the CL.
  //    See: //chromeos/components/system_apps/PLATFORM_OWNERS
  kMaxValue = PROJECTOR
};

}  // namespace web_app

#endif  // CHROME_BROWSER_WEB_APPLICATIONS_SYSTEM_WEB_APPS_SYSTEM_WEB_APP_TYPES_H_
