// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/page_info/chrome_page_info_ui_delegate.h"

#include "build/build_config.h"
#include "chrome/browser/content_settings/chrome_content_settings_utils.h"
#include "chrome/browser/permissions/permission_manager_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/permission_manager.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/cpp/is_potentially_trustworthy.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

#if !defined(OS_ANDROID)
#include "chrome/browser/extensions/window_controller_list.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#endif

ChromePageInfoUiDelegate::ChromePageInfoUiDelegate(Profile* profile,
                                                   const GURL& site_url)
    : profile_(profile), site_url_(site_url) {}

bool ChromePageInfoUiDelegate::ShouldShowAllow(ContentSettingsType type) {
  switch (type) {
    // Notifications and idle detection do not support CONTENT_SETTING_ALLOW in
    // incognito.
    case ContentSettingsType::NOTIFICATIONS:
    case ContentSettingsType::IDLE_DETECTION:
      return !profile_->IsOffTheRecord();
    // Media only supports CONTENT_SETTING_ALLOW for secure origins.
    case ContentSettingsType::MEDIASTREAM_MIC:
    case ContentSettingsType::MEDIASTREAM_CAMERA:
      return network::IsUrlPotentiallyTrustworthy(site_url_);
    // Chooser permissions do not support CONTENT_SETTING_ALLOW.
    case ContentSettingsType::SERIAL_GUARD:
    case ContentSettingsType::USB_GUARD:
    case ContentSettingsType::BLUETOOTH_GUARD:
    case ContentSettingsType::HID_GUARD:
    // Bluetooth scanning does not support CONTENT_SETTING_ALLOW.
    case ContentSettingsType::BLUETOOTH_SCANNING:
    // File system write does not support CONTENT_SETTING_ALLOW.
    case ContentSettingsType::FILE_SYSTEM_WRITE_GUARD:
      return false;
    default:
      return true;
  }
}

std::u16string ChromePageInfoUiDelegate::GetAutomaticallyBlockedReason(
    ContentSettingsType type) {
  switch (type) {
    // Notifications and idle detection do not support CONTENT_SETTING_ALLOW in
    // incognito.
    case ContentSettingsType::NOTIFICATIONS:
    case ContentSettingsType::IDLE_DETECTION: {
      if (profile_->IsOffTheRecord()) {
        return l10n_util::GetStringUTF16(
            IDS_PAGE_INFO_STATE_TEXT_NOT_ALLOWED_IN_INCOGNITO);
      }
      break;
    }
    // Media only supports CONTENT_SETTING_ALLOW for secure origins.
    // TODO(crbug.com/1227679): This string can probably be removed.
    case ContentSettingsType::MEDIASTREAM_MIC:
    case ContentSettingsType::MEDIASTREAM_CAMERA: {
      if (!network::IsUrlPotentiallyTrustworthy(site_url_)) {
        return l10n_util::GetStringUTF16(
            IDS_PAGE_INFO_STATE_TEXT_NOT_ALLOWED_INSECURE);
      }
      break;
    }
    default:
      break;
  }

  return std::u16string();
}

bool ChromePageInfoUiDelegate::ShouldShowAsk(ContentSettingsType type) {
  return permissions::PermissionUtil::IsGuardContentSetting(type);
}

#if !defined(OS_ANDROID)
bool ChromePageInfoUiDelegate::ShouldShowSiteSettings() {
  return !profile_->IsGuestSession();
}

// TODO(crbug.com/1227074): Reconcile with LastTabStandingTracker.
bool ChromePageInfoUiDelegate::IsMultipleTabsOpen() {
  const extensions::WindowControllerList::ControllerList& windows =
      extensions::WindowControllerList::GetInstance()->windows();
  int count = 0;
  auto site_origin = site_url_.GetOrigin();
  for (auto* window : windows) {
    const Browser* const browser = window->GetBrowser();
    if (!browser)
      continue;
    const TabStripModel* const tabs = browser->tab_strip_model();
    DCHECK(tabs);
    for (int i = 0; i < tabs->count(); ++i) {
      content::WebContents* const web_contents = tabs->GetWebContentsAt(i);
      if (web_contents->GetURL().GetOrigin() == site_origin) {
        count++;
      }
    }
  }
  return count > 1;
}

std::u16string ChromePageInfoUiDelegate::GetPermissionDetail(
    ContentSettingsType type) {
  switch (type) {
    // TODO(crbug.com/1228243): Reconcile with SiteDetailsPermissionElement.
    case ContentSettingsType::ADS:
      return l10n_util::GetStringUTF16(IDS_PAGE_INFO_PERMISSION_ADS_SUBTITLE);
    default:
      return content_settings::GetPermissionDetailString(profile_, type,
                                                         site_url_);
  }
}

bool ChromePageInfoUiDelegate::IsBlockAutoPlayEnabled() {
  return profile_->GetPrefs()->GetBoolean(prefs::kBlockAutoplayEnabled);
}
#endif

permissions::PermissionResult ChromePageInfoUiDelegate::GetPermissionStatus(
    ContentSettingsType type) {
  return PermissionManagerFactory::GetForProfile(profile_)->GetPermissionStatus(
      type, site_url_, site_url_);
}
