// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/web_applications/components/web_app_install_utils.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "chrome/browser/banners/app_banner_manager_desktop.h"
#include "chrome/browser/web_applications/components/web_app_icon_generator.h"
#include "chrome/common/chrome_features.h"
#include "components/services/app_service/public/cpp/share_target.h"
#include "components/webapps/browser/banners/app_banner_manager.h"
#include "components/webapps/browser/banners/app_banner_settings_helper.h"
#include "components/webapps/browser/installable/installable_metrics.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/mojom/manifest/manifest.mojom.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace web_app {

namespace {

const char kChromeScheme[] = "chrome";

// We restrict the number of icons to limit disk usage per installed PWA. This
// value can change overtime as new features are added.
constexpr int kMaxIcons = 20;
constexpr SquareSizePx kMaxIconSize = 1024;

// Append non-empty square icons from |icons_map| onto the |square_icons| list.
void AddSquareIconsFromMap(std::vector<SkBitmap>* square_icons,
                           const IconsMap& icons_map) {
  for (const auto& url_icon : icons_map) {
    for (const SkBitmap& icon : url_icon.second) {
      if (!icon.empty() && icon.width() == icon.height())
        square_icons->push_back(icon);
    }
  }
}

// Append non-empty square icons from |icons_map| onto the |square_icons| list,
// if they are also in |icon_infos|.
void AddSquareIconsFromMapMatchingIconInfos(
    std::vector<SkBitmap>* square_icons,
    const std::vector<WebApplicationIconInfo>& icon_infos,
    const IconsMap& icons_map) {
  for (const auto& url_icon : icons_map) {
    for (const SkBitmap& icon : url_icon.second) {
      if (!icon.empty() && icon.width() == icon.height()) {
        for (const auto& info : icon_infos) {
          if (info.url == url_icon.first) {
            square_icons->push_back(icon);
          }
        }
      }
    }
  }
}

// Append non-empty square icons from |bitmaps| onto the |square_icons| list.
void AddSquareIconsFromBitmaps(
    std::vector<SkBitmap>* square_icons,
    const std::map<SquareSizePx, SkBitmap>& bitmaps) {
  for (const std::pair<const SquareSizePx, SkBitmap>& icon : bitmaps) {
    DCHECK_EQ(icon.first, icon.second.width());
    DCHECK_EQ(icon.first, icon.second.height());
    if (!icon.second.empty())
      square_icons->push_back(icon.second);
  }
}

// Populate |web_app_info|'s shortcuts_menu_item_infos vector using the
// blink::Manifest's shortcuts vector.
std::vector<WebApplicationShortcutsMenuItemInfo> ToWebAppShortcutsMenuItemInfos(
    const std::vector<blink::Manifest::ShortcutItem>& shortcuts) {
  std::vector<WebApplicationShortcutsMenuItemInfo> web_app_shortcut_infos;
  web_app_shortcut_infos.reserve(shortcuts.size());
  int num_shortcut_icons = 0;
  for (const auto& shortcut : shortcuts) {
    WebApplicationShortcutsMenuItemInfo shortcut_info;
    shortcut_info.name = shortcut.name;
    shortcut_info.url = shortcut.url;

    for (IconPurpose purpose : kIconPurposes) {
      std::vector<WebApplicationShortcutsMenuItemInfo::Icon> shortcut_icons;
      for (const auto& icon : shortcut.icons) {
        DCHECK(!icon.purpose.empty());
        if (!base::Contains(icon.purpose, purpose))
          continue;

        WebApplicationShortcutsMenuItemInfo::Icon info;

        // Filter out non-square or too large icons.
        auto valid_size_it = std::find_if(
            icon.sizes.begin(), icon.sizes.end(), [](const gfx::Size& size) {
              return size.width() == size.height() &&
                     size.width() <= kMaxIconSize;
            });
        if (valid_size_it == icon.sizes.end())
          continue;
        // TODO(https://crbug.com/1071308): Take the declared icon density and
        // sizes into account.
        info.square_size_px = valid_size_it->width();

        DCHECK_LE(num_shortcut_icons, kMaxIcons);
        if (num_shortcut_icons < kMaxIcons) {
          info.url = icon.src;
          shortcut_icons.push_back(std::move(info));
          ++num_shortcut_icons;
        }
        if (num_shortcut_icons == kMaxIcons)
          break;
      }

      // If any icons are specified in the manifest, they take precedence over
      // any we picked up from web_app_info.
      if (!shortcut_icons.empty())
        shortcut_info.SetShortcutIconInfosForPurpose(purpose,
                                                     std::move(shortcut_icons));
    }
    web_app_shortcut_infos.push_back(std::move(shortcut_info));
  }

  return web_app_shortcut_infos;
}

apps::ShareTarget::Method ToAppsShareTargetMethod(
    blink::mojom::ManifestShareTarget_Method method) {
  switch (method) {
    case blink::mojom::ManifestShareTarget_Method::kGet:
      return apps::ShareTarget::Method::kGet;
    case blink::mojom::ManifestShareTarget_Method::kPost:
      return apps::ShareTarget::Method::kPost;
  }
  NOTREACHED();
}

apps::ShareTarget::Enctype ToAppsShareTargetEnctype(
    blink::mojom::ManifestShareTarget_Enctype enctype) {
  switch (enctype) {
    case blink::mojom::ManifestShareTarget_Enctype::kFormUrlEncoded:
      return apps::ShareTarget::Enctype::kFormUrlEncoded;
    case blink::mojom::ManifestShareTarget_Enctype::kMultipartFormData:
      return apps::ShareTarget::Enctype::kMultipartFormData;
  }
  NOTREACHED();
}

absl::optional<apps::ShareTarget> ToWebAppShareTarget(
    const absl::optional<blink::Manifest::ShareTarget>& share_target) {
  if (!share_target) {
    return absl::nullopt;
  }
  apps::ShareTarget apps_share_target;
  apps_share_target.action = share_target->action;
  apps_share_target.method = ToAppsShareTargetMethod(share_target->method);
  apps_share_target.enctype = ToAppsShareTargetEnctype(share_target->enctype);

  if (share_target->params.title.has_value()) {
    apps_share_target.params.title =
        base::UTF16ToUTF8(*share_target->params.title);
  }
  if (share_target->params.text.has_value()) {
    apps_share_target.params.text =
        base::UTF16ToUTF8(*share_target->params.text);
  }
  if (share_target->params.url.has_value()) {
    apps_share_target.params.url = base::UTF16ToUTF8(*share_target->params.url);
  }

  for (const auto& file_filter : share_target->params.files) {
    apps::ShareTarget::Files apps_share_target_files;
    apps_share_target_files.name = base::UTF16ToUTF8(file_filter.name);

    for (const auto& file_type : file_filter.accept) {
      apps_share_target_files.accept.push_back(base::UTF16ToUTF8(file_type));
    }

    apps_share_target.params.files.push_back(
        std::move(apps_share_target_files));
  }

  return std::move(apps_share_target);
}

apps::UrlHandlers ToWebAppUrlHandlers(
    const std::vector<blink::mojom::ManifestUrlHandlerPtr>& url_handlers) {
  apps::UrlHandlers apps_url_handlers;
  for (const auto& url_handler : url_handlers) {
    DCHECK(url_handler);
    apps_url_handlers.emplace_back(url_handler->origin,
                                   url_handler->has_origin_wildcard);
  }
  return apps_url_handlers;
}

std::vector<apps::ProtocolHandlerInfo> ToWebAppProtocolHandlers(
    const std::vector<blink::mojom::ManifestProtocolHandlerPtr>&
        manifest_protocol_handlers) {
  std::vector<apps::ProtocolHandlerInfo> protocol_handlers;
  for (const auto& manifest_protocol_handler : manifest_protocol_handlers) {
    apps::ProtocolHandlerInfo protocol_handler;
    protocol_handler.protocol =
        base::UTF16ToUTF8(manifest_protocol_handler->protocol);
    protocol_handler.url = manifest_protocol_handler->url;
    protocol_handlers.push_back(std::move(protocol_handler));
  }
  return protocol_handlers;
}

}  // namespace

apps::FileHandlers CreateFileHandlersFromManifest(
    const std::vector<blink::mojom::ManifestFileHandlerPtr>&
        manifest_file_handlers,
    const GURL& app_scope) {
  apps::FileHandlers web_app_file_handlers;

  for (const auto& manifest_file_handler : manifest_file_handlers) {
    DCHECK(manifest_file_handler);
    apps::FileHandler web_app_file_handler;
    web_app_file_handler.action = manifest_file_handler->action;

    for (const auto& it : manifest_file_handler->accept) {
      apps::FileHandler::AcceptEntry web_app_accept_entry;
      web_app_accept_entry.mime_type = base::UTF16ToUTF8(it.first);
      for (const auto& manifest_file_extension : it.second) {
        web_app_accept_entry.file_extensions.insert(
            base::UTF16ToUTF8(manifest_file_extension));
      }
      web_app_file_handler.accept.push_back(std::move(web_app_accept_entry));
    }

    web_app_file_handlers.push_back(std::move(web_app_file_handler));

    if (web_app_file_handlers.size() == kMaxFileHandlers &&
        !app_scope.SchemeIs(kChromeScheme)) {
      break;
    }
  }

  DCHECK(web_app_file_handlers.size() <= kMaxFileHandlers ||
         app_scope.SchemeIs(kChromeScheme));

  return web_app_file_handlers;
}

void UpdateWebAppInfoFromManifest(const blink::mojom::Manifest& manifest,
                                  const GURL& manifest_url,
                                  WebApplicationInfo* web_app_info) {
  // Give the full length name priority if it's not empty.
  std::u16string name = manifest.name.value_or(std::u16string());
  if (!name.empty())
    web_app_info->title = name;
  else if (manifest.short_name)
    web_app_info->title = *manifest.short_name;

  if (manifest.id.has_value()) {
    web_app_info->manifest_id =
        absl::optional<std::string>(base::UTF16ToUTF8(manifest.id.value()));
  }

  // Set the url based on the manifest value, if any.
  if (manifest.start_url.is_valid())
    web_app_info->start_url = manifest.start_url;

  if (manifest.scope.is_valid())
    web_app_info->scope = manifest.scope;

  if (manifest.has_theme_color) {
    web_app_info->theme_color =
        SkColorSetA(SkColor(manifest.theme_color), SK_AlphaOPAQUE);
  }

  if (manifest.has_background_color) {
    web_app_info->background_color =
        SkColorSetA(SkColor(manifest.background_color), SK_AlphaOPAQUE);
  }

  if (manifest.display != DisplayMode::kUndefined)
    web_app_info->display_mode = manifest.display;

  if (!manifest.display_override.empty())
    web_app_info->display_override = manifest.display_override;

  // Create the WebApplicationInfo icons list *outside* of |web_app_info|, so
  // that we can decide later whether or not to replace the existing icons.
  std::vector<WebApplicationIconInfo> web_app_icons;
  for (const auto& icon : manifest.icons) {
    // An icon's purpose vector should never be empty (the manifest parser
    // should have added ANY if there was no purpose specified in the manifest).
    DCHECK(!icon.purpose.empty());

    for (IconPurpose purpose : icon.purpose) {
      WebApplicationIconInfo info;

      if (!icon.sizes.empty()) {
        // Filter out non-square or too large icons.
        auto valid_size = std::find_if(icon.sizes.begin(), icon.sizes.end(),
                                       [](const gfx::Size& size) {
                                         return size.width() == size.height() &&
                                                size.width() <= kMaxIconSize;
                                       });
        if (valid_size == icon.sizes.end())
          continue;
        // TODO(https://crbug.com/1071308): Take the declared icon density and
        // sizes into account.
        info.square_size_px = valid_size->width();
      }

      info.url = icon.src;
      info.purpose = purpose;
      web_app_icons.push_back(std::move(info));

      // Limit the number of icons we store on the user's machine.
      if (web_app_icons.size() == kMaxIcons)
        break;
    }
    // Limit the number of icons we store on the user's machine.
    if (web_app_icons.size() == kMaxIcons)
      break;
  }
  // If any icons are correctly specified in the manifest, they take precedence
  // over any we picked up from web page metadata.
  if (!web_app_icons.empty())
    web_app_info->icon_infos = std::move(web_app_icons);

  // TODO(crbug.com/1218210): Confirm incoming icons to write to web_app_info.
  web_app_info->file_handlers = CreateFileHandlersFromManifest(
      manifest.file_handlers, web_app_info->scope);

  web_app_info->share_target = ToWebAppShareTarget(manifest.share_target);

  web_app_info->protocol_handlers =
      ToWebAppProtocolHandlers(manifest.protocol_handlers);

  web_app_info->url_handlers = ToWebAppUrlHandlers(manifest.url_handlers);

  if (base::FeatureList::IsEnabled(blink::features::kWebAppNoteTaking) &&
      manifest.note_taking && manifest.note_taking->new_note_url.is_valid()) {
    web_app_info->note_taking_new_note_url = manifest.note_taking->new_note_url;
  }

  DCHECK(web_app_info->shortcuts_menu_item_infos.empty());
  web_app_info->shortcuts_menu_item_infos =
      ToWebAppShortcutsMenuItemInfos(manifest.shortcuts);

  web_app_info->capture_links = manifest.capture_links;

  if (manifest_url.is_valid())
    web_app_info->manifest_url = manifest_url;

  web_app_info->is_storage_isolated = manifest.isolated_storage;

  web_app_info->launch_handler = manifest.launch_handler;
}

std::vector<GURL> GetValidIconUrlsToDownload(
    const WebApplicationInfo& web_app_info) {
  std::vector<GURL> web_app_info_icon_urls;
  // App icons.
  for (const WebApplicationIconInfo& info : web_app_info.icon_infos) {
    if (!info.url.is_valid())
      continue;
    web_app_info_icon_urls.push_back(info.url);
  }

  // Shortcut icons.
  for (const auto& shortcut : web_app_info.shortcuts_menu_item_infos) {
    for (IconPurpose purpose : kIconPurposes) {
      for (const auto& icon :
           shortcut.GetShortcutIconInfosForPurpose(purpose)) {
        if (!icon.url.is_valid())
          continue;
        web_app_info_icon_urls.push_back(icon.url);
      }
    }
  }

  // TODO(crbug.com/1152661): Consider eliminating duplicates.
  return web_app_info_icon_urls;
}

void PopulateShortcutItemIcons(WebApplicationInfo* web_app_info,
                               const IconsMap& icons_map) {
  web_app_info->shortcuts_menu_icon_bitmaps.clear();
  for (auto& shortcut : web_app_info->shortcuts_menu_item_infos) {
    IconBitmaps shortcut_icon_bitmaps;

    for (IconPurpose purpose : kIconPurposes) {
      std::map<SquareSizePx, SkBitmap> bitmaps;
      for (const auto& icon :
           shortcut.GetShortcutIconInfosForPurpose(purpose)) {
        auto it = icons_map.find(icon.url);
        if (it != icons_map.end()) {
          std::set<SquareSizePx> sizes_to_generate;
          sizes_to_generate.emplace(icon.square_size_px);
          SizeToBitmap resized_bitmaps(
              ConstrainBitmapsToSizes(it->second, sizes_to_generate));

          // Don't overwrite as a shortcut item could have multiple icon urls.
          bitmaps.insert(resized_bitmaps.begin(), resized_bitmaps.end());
        }
      }
      shortcut_icon_bitmaps.SetBitmapsForPurpose(purpose, std::move(bitmaps));
    }

    web_app_info->shortcuts_menu_icon_bitmaps.emplace_back(
        std::move(shortcut_icon_bitmaps));
  }
}

void PopulateProductIcons(WebApplicationInfo* web_app_info,
                          const IconsMap* icons_map) {
  std::vector<WebApplicationIconInfo> icon_infos_any;
  std::vector<WebApplicationIconInfo> icon_infos_maskable;
  std::vector<WebApplicationIconInfo> icon_infos_monochrome;
  for (WebApplicationIconInfo& icon_info : web_app_info->icon_infos) {
    switch (icon_info.purpose) {
      case IconPurpose::ANY:
        icon_infos_any.push_back(icon_info);
        break;
      case IconPurpose::MASKABLE:
        icon_infos_maskable.push_back(icon_info);
        break;
      case IconPurpose::MONOCHROME:
        icon_infos_monochrome.push_back(icon_info);
        break;
    }
  }

  std::vector<SkBitmap> square_icons_any;
  std::vector<SkBitmap> square_icons_maskable;
  std::vector<SkBitmap> square_icons_monochrome;
  if (icons_map) {
    AddSquareIconsFromMapMatchingIconInfos(&square_icons_any, icon_infos_any,
                                           *icons_map);
    AddSquareIconsFromMapMatchingIconInfos(&square_icons_maskable,
                                           icon_infos_maskable, *icons_map);
    AddSquareIconsFromMapMatchingIconInfos(&square_icons_monochrome,
                                           icon_infos_monochrome, *icons_map);
    // Fall back to using all icons from |icons_map| if none match icon_infos.
    if (square_icons_any.empty())
      AddSquareIconsFromMap(&square_icons_any, *icons_map);
  }
  AddSquareIconsFromBitmaps(&square_icons_any, web_app_info->icon_bitmaps.any);

  for (SkBitmap& bitmap : square_icons_maskable) {
    // Retain any bitmaps provided as input to the installation.
    if (web_app_info->icon_bitmaps.maskable.count(bitmap.width()) == 0)
      web_app_info->icon_bitmaps.maskable[bitmap.width()] = std::move(bitmap);
  }

  for (SkBitmap& bitmap : square_icons_monochrome) {
    // Retain any bitmaps provided as input to the installation.
    if (web_app_info->icon_bitmaps.monochrome.count(bitmap.width()) == 0)
      web_app_info->icon_bitmaps.monochrome[bitmap.width()] = std::move(bitmap);
  }

  char16_t icon_letter =
      web_app_info->title.empty()
          ? GenerateIconLetterFromUrl(web_app_info->start_url)
          : GenerateIconLetterFromAppName(web_app_info->title);
  web_app_info->generated_icon_color = SK_ColorTRANSPARENT;
  // Ensure that all top-level icons that are in web_app_info with  Purpose::ANY
  // are present, by generating icons for any sizes that have failed to
  // download. This ensures that the created manifest for the web app does not
  // contain links to icons that are not actually created and linked on disk.
  // TODO(https://crbug.com/1029223): Don't resize before writing to disk, it's
  // not necessary and would simplify this code path to remove.
  SizeToBitmap size_to_icons = ResizeIconsAndGenerateMissing(
      square_icons_any, SizesToGenerate(), icon_letter,
      &web_app_info->generated_icon_color, &web_app_info->is_generated_icon);

  for (auto& item : size_to_icons) {
    // Retain any bitmaps provided as input to the installation.
    if (web_app_info->icon_bitmaps.any.count(item.first) == 0)
      web_app_info->icon_bitmaps.any[item.first] = std::move(item.second);
  }
}

void RecordAppBanner(content::WebContents* contents, const GURL& app_url) {
  webapps::AppBannerSettingsHelper::RecordBannerEvent(
      contents, app_url, app_url.spec(),
      webapps::AppBannerSettingsHelper::APP_BANNER_EVENT_DID_ADD_TO_HOMESCREEN,
      base::Time::Now());
}

webapps::WebappInstallSource ConvertExternalInstallSourceToInstallSource(
    ExternalInstallSource external_install_source) {
  webapps::WebappInstallSource install_source;
  switch (external_install_source) {
    case ExternalInstallSource::kInternalDefault:
      install_source = webapps::WebappInstallSource::INTERNAL_DEFAULT;
      break;
    case ExternalInstallSource::kExternalDefault:
      install_source = webapps::WebappInstallSource::EXTERNAL_DEFAULT;
      break;
    case ExternalInstallSource::kExternalPolicy:
      install_source = webapps::WebappInstallSource::EXTERNAL_POLICY;
      break;
    case ExternalInstallSource::kSystemInstalled:
      install_source = webapps::WebappInstallSource::SYSTEM_DEFAULT;
      break;
    case ExternalInstallSource::kArc:
      install_source = webapps::WebappInstallSource::ARC;
      break;
  }

  return install_source;
}

webapps::WebappUninstallSource ConvertExternalInstallSourceToUninstallSource(
    ExternalInstallSource external_install_source) {
  webapps::WebappUninstallSource uninstall_source;
  switch (external_install_source) {
    case ExternalInstallSource::kInternalDefault:
      uninstall_source = webapps::WebappUninstallSource::kInternalPreinstalled;
      break;
    case ExternalInstallSource::kExternalDefault:
      uninstall_source = webapps::WebappUninstallSource::kExternalPreinstalled;
      break;
    case ExternalInstallSource::kExternalPolicy:
      uninstall_source = webapps::WebappUninstallSource::kExternalPolicy;
      break;
    case ExternalInstallSource::kSystemInstalled:
      uninstall_source = webapps::WebappUninstallSource::kSystemPreinstalled;
      break;
    case ExternalInstallSource::kArc:
      uninstall_source = webapps::WebappUninstallSource::kArc;
      break;
  }

  return uninstall_source;
}

// TODO(loyso): Call sites should specify Source explicitly as a part of
// AppTraits parameter object.
Source::Type InferSourceFromMetricsInstallSource(
    webapps::WebappInstallSource install_source) {
  switch (install_source) {
    case webapps::WebappInstallSource::MENU_BROWSER_TAB:
    case webapps::WebappInstallSource::MENU_CUSTOM_TAB:
    case webapps::WebappInstallSource::AUTOMATIC_PROMPT_BROWSER_TAB:
    case webapps::WebappInstallSource::AUTOMATIC_PROMPT_CUSTOM_TAB:
    case webapps::WebappInstallSource::API_BROWSER_TAB:
    case webapps::WebappInstallSource::API_CUSTOM_TAB:
    case webapps::WebappInstallSource::DEVTOOLS:
    case webapps::WebappInstallSource::MANAGEMENT_API:
    case webapps::WebappInstallSource::AMBIENT_BADGE_BROWSER_TAB:
    case webapps::WebappInstallSource::AMBIENT_BADGE_CUSTOM_TAB:
    case webapps::WebappInstallSource::OMNIBOX_INSTALL_ICON:
    case webapps::WebappInstallSource::SYNC:
    case webapps::WebappInstallSource::MENU_CREATE_SHORTCUT:
      return Source::kSync;

    case webapps::WebappInstallSource::INTERNAL_DEFAULT:
    case webapps::WebappInstallSource::EXTERNAL_DEFAULT:
      return Source::kDefault;

    case webapps::WebappInstallSource::EXTERNAL_POLICY:
      return Source::kPolicy;

    case webapps::WebappInstallSource::SYSTEM_DEFAULT:
      return Source::kSystem;

    case webapps::WebappInstallSource::ARC:
      return Source::kWebAppStore;

    case webapps::WebappInstallSource::COUNT:
      NOTREACHED();
      return Source::kSync;
  }
}

}  // namespace web_app
