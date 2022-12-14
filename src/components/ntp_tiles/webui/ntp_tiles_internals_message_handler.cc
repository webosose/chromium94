// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_tiles/webui/ntp_tiles_internals_message_handler.h"

#include <array>
#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/check_op.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/task_runner_util.h"
#include "base/values.h"
#include "components/favicon/core/favicon_service.h"
#include "components/ntp_tiles/constants.h"
#include "components/ntp_tiles/most_visited_sites.h"
#include "components/ntp_tiles/pref_names.h"
#include "components/ntp_tiles/webui/ntp_tiles_internals_message_handler_client.h"
#include "components/prefs/pref_service.h"
#include "components/url_formatter/url_fixer.h"
#include "url/gurl.h"

namespace ntp_tiles {

namespace {

using FaviconResultMap = std::map<std::pair<GURL, favicon_base::IconType>,
                                  favicon_base::FaviconRawBitmapResult>;

struct IconTypeAndName {
  favicon_base::IconType type_enum;
  const char* type_name;
};

constexpr std::array<IconTypeAndName, 4> kIconTypesAndNames{{
    {favicon_base::IconType::kFavicon, "kFavicon"},
    {favicon_base::IconType::kTouchIcon, "kTouchIcon"},
    {favicon_base::IconType::kTouchPrecomposedIcon, "kTouchPrecomposedIcon"},
    {favicon_base::IconType::kWebManifestIcon, "kWebManifestIcon"},
}};

std::string FormatJson(const base::Value& value) {
  std::string pretty_printed;
  bool ok = base::JSONWriter::WriteWithOptions(
      value, base::JSONWriter::OPTIONS_PRETTY_PRINT, &pretty_printed);
  DCHECK(ok);
  return pretty_printed;
}

}  // namespace

NTPTilesInternalsMessageHandler::NTPTilesInternalsMessageHandler(
    favicon::FaviconService* favicon_service)
    : favicon_service_(favicon_service),
      client_(nullptr),
      site_count_(ntp_tiles::kMaxNumMostVisited) {}

NTPTilesInternalsMessageHandler::~NTPTilesInternalsMessageHandler() = default;

void NTPTilesInternalsMessageHandler::RegisterMessages(
    NTPTilesInternalsMessageHandlerClient* client) {
  client_ = client;

  client_->RegisterMessageCallback(
      "registerForEvents",
      base::BindRepeating(
          &NTPTilesInternalsMessageHandler::HandleRegisterForEvents,
          base::Unretained(this)));

  client_->RegisterMessageCallback(
      "update",
      base::BindRepeating(&NTPTilesInternalsMessageHandler::HandleUpdate,
                          base::Unretained(this)));

  client_->RegisterMessageCallback(
      "viewPopularSitesJson",
      base::BindRepeating(
          &NTPTilesInternalsMessageHandler::HandleViewPopularSitesJson,
          base::Unretained(this)));
}

void NTPTilesInternalsMessageHandler::HandleRegisterForEvents(
    const base::ListValue* args) {
  if (!client_->SupportsNTPTiles()) {
    base::Value disabled(base::Value::Type::DICTIONARY);
    disabled.SetBoolKey("topSites", false);
    disabled.SetBoolKey("popular", false);
    disabled.SetBoolKey("customLinks", false);
    disabled.SetBoolKey("allowlist", false);
    client_->CallJavascriptFunction("cr.webUIListenerCallback",
                                    base::Value("receive-source-info"),
                                    disabled);
    SendTiles(NTPTilesVector(), FaviconResultMap());
    return;
  }
  DCHECK_EQ(0u, args->GetSize());

  popular_sites_json_.clear();
  most_visited_sites_ = client_->MakeMostVisitedSites();
  most_visited_sites_->AddMostVisitedURLsObserver(this, site_count_);
  SendSourceInfo();
}

void NTPTilesInternalsMessageHandler::HandleUpdate(
    const base::ListValue* args) {
  if (!client_->SupportsNTPTiles()) {
    return;
  }

  const base::Value* dict = nullptr;
  DCHECK_EQ(1u, args->GetSize());
  args->Get(0, &dict);
  DCHECK(dict && dict->is_dict());

  PrefService* prefs = client_->GetPrefs();

  if (most_visited_sites_ &&
      most_visited_sites_->DoesSourceExist(ntp_tiles::TileSource::POPULAR)) {
    popular_sites_json_.clear();

    const std::string* url = dict->FindStringPath("popular.overrideURL");
    if (url->empty()) {
      prefs->ClearPref(ntp_tiles::prefs::kPopularSitesOverrideURL);
    } else {
      prefs->SetString(ntp_tiles::prefs::kPopularSitesOverrideURL,
                       url_formatter::FixupURL(*url, std::string()).spec());
    }

    const std::string* directory =
        dict->FindStringPath("popular.overrideDirectory");
    if (directory->empty()) {
      prefs->ClearPref(ntp_tiles::prefs::kPopularSitesOverrideDirectory);
    } else {
      prefs->SetString(ntp_tiles::prefs::kPopularSitesOverrideDirectory,
                       *directory);
    }

    const std::string* country =
        dict->FindStringPath("popular.overrideCountry");
    if (country->empty()) {
      prefs->ClearPref(ntp_tiles::prefs::kPopularSitesOverrideCountry);
    } else {
      prefs->SetString(ntp_tiles::prefs::kPopularSitesOverrideCountry,
                       *country);
    }

    const std::string* version =
        dict->FindStringPath("popular.overrideVersion");
    if (version->empty()) {
      prefs->ClearPref(ntp_tiles::prefs::kPopularSitesOverrideVersion);
    } else {
      prefs->SetString(ntp_tiles::prefs::kPopularSitesOverrideVersion,
                       *version);
    }
  }

  // Recreate to pick up new values.
  // TODO(sfiera): refresh MostVisitedSites without re-creating it, as soon as
  // that will pick up changes to the Popular Sites overrides.
  most_visited_sites_ = client_->MakeMostVisitedSites();
  most_visited_sites_->AddMostVisitedURLsObserver(this, site_count_);
  SendSourceInfo();
}

void NTPTilesInternalsMessageHandler::HandleViewPopularSitesJson(
    const base::ListValue* args) {
  DCHECK_EQ(0u, args->GetSize());
  if (!most_visited_sites_ ||
      !most_visited_sites_->DoesSourceExist(ntp_tiles::TileSource::POPULAR)) {
    return;
  }

  popular_sites_json_ =
      FormatJson(*most_visited_sites_->popular_sites()->GetCachedJson());
  SendSourceInfo();
}

void NTPTilesInternalsMessageHandler::SendSourceInfo() {
  PrefService* prefs = client_->GetPrefs();
  base::Value value(base::Value::Type::DICTIONARY);

  value.SetBoolKey("topSites",
                   most_visited_sites_->DoesSourceExist(TileSource::TOP_SITES));
  value.SetBoolKey("customLinks", most_visited_sites_->DoesSourceExist(
                                      TileSource::CUSTOM_LINKS));
  value.SetBoolKey("allowlist",
                   most_visited_sites_->DoesSourceExist(TileSource::ALLOWLIST));

  if (most_visited_sites_->DoesSourceExist(TileSource::POPULAR)) {
    auto* popular_sites = most_visited_sites_->popular_sites();
    value.SetStringKey("popular.url", popular_sites->GetURLToFetch().spec());
    value.SetStringKey("popular.directory",
                       popular_sites->GetDirectoryToFetch());
    value.SetStringKey("popular.country", popular_sites->GetCountryToFetch());
    value.SetStringKey("popular.version", popular_sites->GetVersionToFetch());

    value.SetStringKey(
        "popular.overrideURL",
        prefs->GetString(ntp_tiles::prefs::kPopularSitesOverrideURL));
    value.SetStringKey(
        "popular.overrideDirectory",
        prefs->GetString(ntp_tiles::prefs::kPopularSitesOverrideDirectory));
    value.SetStringKey(
        "popular.overrideCountry",
        prefs->GetString(ntp_tiles::prefs::kPopularSitesOverrideCountry));
    value.SetStringKey(
        "popular.overrideVersion",
        prefs->GetString(ntp_tiles::prefs::kPopularSitesOverrideVersion));

    value.SetStringKey("popular.json", popular_sites_json_);
  } else {
    value.SetBoolKey("popular", false);
  }

  client_->CallJavascriptFunction("cr.webUIListenerCallback",
                                  base::Value("receive-source-info"), value);
}

void NTPTilesInternalsMessageHandler::SendTiles(
    const NTPTilesVector& tiles,
    const FaviconResultMap& result_map) {
  base::Value sites_list(base::Value::Type::LIST);
  for (const NTPTile& tile : tiles) {
    base::Value entry(base::Value::Type::DICTIONARY);
    entry.SetStringKey("title", tile.title);
    entry.SetStringKey("url", tile.url.spec());
    entry.SetIntKey("source", static_cast<int>(tile.source));
    entry.SetStringKey("allowlistIconPath",
                       tile.allowlist_icon_path.LossyDisplayName());
    if (tile.source == TileSource::CUSTOM_LINKS) {
      entry.SetBoolKey("fromMostVisited", tile.from_most_visited);
    }

    base::Value icon_list(base::Value::Type::LIST);
    for (const auto& entry : kIconTypesAndNames) {
      auto it = result_map.find(
          FaviconResultMap::key_type(tile.url, entry.type_enum));

      if (it != result_map.end()) {
        const favicon_base::FaviconRawBitmapResult& result = it->second;
        base::Value icon(base::Value::Type::DICTIONARY);
        icon.SetStringKey("url", result.icon_url.spec());
        icon.SetStringKey("type", entry.type_name);
        icon.SetBoolKey("onDemand", !result.fetched_because_of_page_visit);
        icon.SetIntKey("width", result.pixel_size.width());
        icon.SetIntKey("height", result.pixel_size.height());
        icon_list.Append(std::move(icon));
      }
    }
    entry.SetKey("icons", std::move(icon_list));

    sites_list.Append(std::move(entry));
  }

  base::Value result(base::Value::Type::DICTIONARY);
  result.SetKey("sites", std::move(sites_list));
  client_->CallJavascriptFunction("cr.webUIListenerCallback",
                                  base::Value("receive-sites"), result);
}

void NTPTilesInternalsMessageHandler::OnURLsAvailable(
    const std::map<SectionType, NTPTilesVector>& sections) {
  cancelable_task_tracker_.TryCancelAll();

  // TODO(fhorschig): Handle non-personalized tiles - https://crbug.com/753852.
  const NTPTilesVector& tiles = sections.at(SectionType::PERSONALIZED);
  if (tiles.empty()) {
    SendTiles(tiles, FaviconResultMap());
    return;
  }

  auto on_lookup_done = base::BindRepeating(
      &NTPTilesInternalsMessageHandler::OnFaviconLookupDone,
      // Unretained(this) is safe because of |cancelable_task_tracker_|.
      base::Unretained(this), tiles, base::Owned(new FaviconResultMap()),
      base::Owned(new size_t(tiles.size() * kIconTypesAndNames.size())));

  for (const NTPTile& tile : tiles) {
    for (const auto& entry : kIconTypesAndNames) {
      favicon_service_->GetLargestRawFaviconForPageURL(
          tile.url, std::vector<favicon_base::IconTypeSet>({{entry.type_enum}}),
          /*minimum_size_in_pixels=*/0,
          base::BindOnce(on_lookup_done, tile.url), &cancelable_task_tracker_);
    }
  }
}

void NTPTilesInternalsMessageHandler::OnIconMadeAvailable(
    const GURL& site_url) {}

void NTPTilesInternalsMessageHandler::OnFaviconLookupDone(
    const NTPTilesVector& tiles,
    FaviconResultMap* result_map,
    size_t* num_pending_lookups,
    const GURL& page_url,
    const favicon_base::FaviconRawBitmapResult& result) {
  DCHECK_NE(0u, *num_pending_lookups);

  result_map->emplace(
      std::pair<GURL, favicon_base::IconType>(page_url, result.icon_type),
      result);

  --*num_pending_lookups;
  if (*num_pending_lookups == 0)
    SendTiles(tiles, *result_map);
}

}  // namespace ntp_tiles
