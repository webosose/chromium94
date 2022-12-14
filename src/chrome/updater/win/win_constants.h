// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_UPDATER_WIN_WIN_CONSTANTS_H_
#define CHROME_UPDATER_WIN_WIN_CONSTANTS_H_

#include <windows.h>

#include "chrome/updater/updater_branding.h"

namespace updater {

// The prefix to use for global names in WIN32 API's. The prefix is necessary
// to avoid collision on kernel object names.
extern const wchar_t kGlobalPrefix[];

// Serializes access to prefs.
extern const wchar_t kPrefsAccessMutex[];

// Registry keys and value names.
#define COMPANY_KEY "Software\\" COMPANY_SHORTNAME_STRING "\\"

// Use |Update| instead of PRODUCT_FULLNAME_STRING for the registry key name
// to be backward compatible with Google Update / Omaha.
#define UPDATER_KEY COMPANY_KEY "Update\\"
#define CLIENTS_KEY UPDATER_KEY "Clients\\"
#define CLIENT_STATE_KEY UPDATER_KEY "ClientState\\"

#define COMPANY_POLICIES_KEY \
  L"Software\\Policies\\" COMPANY_SHORTNAME_STRING L"\\"
#define UPDATER_POLICIES_KEY COMPANY_POLICIES_KEY L"Update\\"

#define USER_REG_VISTA_LOW_INTEGRITY_HKCU     \
  L"Software\\Microsoft\\Internet Explorer\\" \
  L"InternetRegistry\\REGISTRY\\USER"

extern const wchar_t kRegValuePV[];
extern const wchar_t kRegValueName[];

// Installer API registry names.
extern const wchar_t kRegValueInstallerError[];
extern const wchar_t kRegValueInstallerExtraCode1[];
extern const wchar_t kRegValueInstallerProgress[];
extern const wchar_t kRegValueInstallerResult[];
extern const wchar_t kRegValueInstallerResultUIString[];
extern const wchar_t kRegValueInstallerSuccessLaunchCmdLine[];

// Device management.
//
// Registry for enrollment token.
extern const wchar_t kRegKeyCompanyCloudManagement[];
extern const wchar_t kRegValueEnrollmentToken[];

// Registry for DM token.
extern const wchar_t kRegKeyCompanyEnrollment[];
extern const wchar_t kRegValueDmToken[];

extern const wchar_t kWindowsServiceName[];
extern const wchar_t kWindowsInternalServiceName[];

}  // namespace updater

#endif  // CHROME_UPDATER_WIN_WIN_CONSTANTS_H_
