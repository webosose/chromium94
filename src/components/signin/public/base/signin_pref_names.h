// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_PUBLIC_BASE_SIGNIN_PREF_NAMES_H_
#define COMPONENTS_SIGNIN_PUBLIC_BASE_SIGNIN_PREF_NAMES_H_

#include "build/chromeos_buildflags.h"

namespace prefs {

#if BUILDFLAG(IS_CHROMEOS_ASH)
extern const char kForceLogoutUnauthenticatedUserEnabled[];
#endif
extern const char kAccountIdMigrationState[];
extern const char kAccountInfo[];
extern const char kAutologinEnabled[];
extern const char kGaiaCookieHash[];
extern const char kGaiaCookieChangedTime[];
extern const char kGaiaCookiePeriodicReportTime[];
extern const char kGoogleServicesAccountId[];
extern const char kGoogleServicesConsentedToSync[];
extern const char kGoogleServicesLastAccountId[];
extern const char kGoogleServicesLastUsername[];
extern const char kGoogleServicesSigninScopedDeviceId[];
extern const char kGoogleServicesUsernamePattern[];
extern const char kRestrictAccountsToPatterns[];
extern const char kReverseAutologinRejectedEmailList[];
extern const char kSignedInWithCredentialProvider[];
extern const char kSigninAllowed[];
extern const char kSigninAllowedByPolicy[];
extern const char kTokenServiceDiceCompatible[];
extern const char kGaiaCookieLastListAccountsData[];

}  // namespace prefs

#endif  // COMPONENTS_SIGNIN_PUBLIC_BASE_SIGNIN_PREF_NAMES_H_
