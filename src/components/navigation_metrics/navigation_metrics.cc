// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/navigation_metrics/navigation_metrics.h"

#include "base/cxx17_backports.h"
#include "base/i18n/rtl.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "components/dom_distiller/core/url_constants.h"
#include "components/profile_metrics/browser_profile_type.h"
#include "components/url_formatter/url_formatter.h"
#include "url/gurl.h"

namespace navigation_metrics {

const char kMainFrameScheme[] = "Navigation.MainFrameScheme2";
const char kMainFrameSchemeDifferentPage[] =
    "Navigation.MainFrameSchemeDifferentPage2";
const char kMainFrameSchemeOTR[] = "Navigation.MainFrameSchemeOTR2";
const char kMainFrameSchemeDifferentPageOTR[] =
    "Navigation.MainFrameSchemeDifferentPageOTR2";
const char kMainFrameHasRTLDomain[] = "Navigation.MainFrameHasRTLDomain2";
const char kMainFrameHasRTLDomainDifferentPage[] =
    "Navigation.MainFrameHasRTLDomainDifferentPage2";
const char kMainFrameProfileType[] = "Navigation.MainFrameProfileType2";

namespace {

const char* const kSchemeNames[] = {
    "unknown",
    url::kHttpScheme,
    url::kHttpsScheme,
    url::kFileScheme,
    url::kFtpScheme,
    url::kDataScheme,
    url::kJavaScriptScheme,
    url::kAboutScheme,
    "chrome",
    url::kBlobScheme,
    url::kFileSystemScheme,
    "chrome-native",
    "chrome-search",
    dom_distiller::kDomDistillerScheme,
    "devtools",
    "chrome-extension",
    "view-source",
    "externalfile",
};

static_assert(base::size(kSchemeNames) == static_cast<int>(Scheme::COUNT),
              "kSchemeNames should have Scheme::COUNT elements");

}  // namespace

Scheme GetScheme(const GURL& url) {
  for (int i = static_cast<int>(Scheme::HTTP);
       i < static_cast<int>(Scheme::COUNT); ++i) {
    if (url.SchemeIs(kSchemeNames[i]))
      return static_cast<Scheme>(i);
  }
  return Scheme::UNKNOWN;
}

void RecordMainFrameNavigation(
    const GURL& url,
    bool is_same_document,
    bool is_off_the_record,
    profile_metrics::BrowserProfileType profile_type) {
  Scheme scheme = GetScheme(url);
  UMA_HISTOGRAM_ENUMERATION("Navigation.MainFrameScheme2", scheme,
                            Scheme::COUNT);
  if (!is_same_document) {
    UMA_HISTOGRAM_ENUMERATION("Navigation.MainFrameSchemeDifferentPage2",
                              scheme, Scheme::COUNT);
    UMA_HISTOGRAM_BOOLEAN("Navigation.MainFrameHasRTLDomainDifferentPage2",
                          base::i18n::StringContainsStrongRTLChars(
                              url_formatter::IDNToUnicode(url.host())));
  }

  UMA_HISTOGRAM_BOOLEAN("Navigation.MainFrameHasRTLDomain2",
                        base::i18n::StringContainsStrongRTLChars(
                            url_formatter::IDNToUnicode(url.host())));

  if (is_off_the_record) {
    UMA_HISTOGRAM_ENUMERATION("Navigation.MainFrameSchemeOTR2", scheme,
                              Scheme::COUNT);
    if (!is_same_document) {
      UMA_HISTOGRAM_ENUMERATION("Navigation.MainFrameSchemeDifferentPageOTR2",
                                scheme, Scheme::COUNT);
    }
  }
  UMA_HISTOGRAM_ENUMERATION("Navigation.MainFrameProfileType2", profile_type);
}

void RecordOmniboxURLNavigation(const GURL& url) {
  UMA_HISTOGRAM_ENUMERATION("Omnibox.URLNavigationScheme", GetScheme(url),
                            Scheme::COUNT);
}

}  // namespace navigation_metrics
