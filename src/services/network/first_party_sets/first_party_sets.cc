// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/first_party_sets/first_party_sets.h"

#include <initializer_list>
#include <set>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/logging.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_split.h"
#include "net/base/schemeful_site.h"
#include "net/cookies/cookie_constants.h"
#include "net/cookies/same_party_context.h"
#include "services/network/first_party_sets/first_party_set_parser.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace network {

namespace {

absl::optional<
    std::pair<net::SchemefulSite, base::flat_set<net::SchemefulSite>>>
CanonicalizeSet(const std::vector<std::string>& origins) {
  if (origins.empty())
    return absl::nullopt;

  const absl::optional<net::SchemefulSite> maybe_owner =
      FirstPartySetParser::CanonicalizeRegisteredDomain(origins[0],
                                                        true /* emit_errors */);
  if (!maybe_owner.has_value()) {
    LOG(ERROR) << "First-Party Set owner is not valid; aborting.";
    return absl::nullopt;
  }

  const net::SchemefulSite& owner = *maybe_owner;
  base::flat_set<net::SchemefulSite> members;
  for (auto it = origins.begin() + 1; it != origins.end(); ++it) {
    const absl::optional<net::SchemefulSite> maybe_member =
        FirstPartySetParser::CanonicalizeRegisteredDomain(
            *it, true /* emit_errors */);
    if (maybe_member.has_value() && maybe_member != owner)
      members.emplace(std::move(*maybe_member));
  }

  if (members.empty()) {
    LOG(ERROR) << "No valid First-Party Set members were specified; aborting.";
    return absl::nullopt;
  }

  return absl::make_optional(
      std::make_pair(std::move(owner), std::move(members)));
}

net::SamePartyContext::Type ContextTypeFromBool(bool is_same_party) {
  return is_same_party ? net::SamePartyContext::Type::kSameParty
                       : net::SamePartyContext::Type::kCrossParty;
}

}  // namespace

FirstPartySets::FirstPartySets() = default;

FirstPartySets::~FirstPartySets() = default;

void FirstPartySets::SetManuallySpecifiedSet(const std::string& flag_value) {
  manually_specified_set_ = CanonicalizeSet(base::SplitString(
      flag_value, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY));

  ApplyManuallySpecifiedSet();
}

base::flat_map<net::SchemefulSite, net::SchemefulSite>*
FirstPartySets::ParseAndSet(base::StringPiece raw_sets) {
  sets_ = FirstPartySetParser::ParseSetsFromComponentUpdater(raw_sets);
  ApplyManuallySpecifiedSet();
  return &sets_;
}

bool FirstPartySets::IsContextSamePartyWithSite(
    const net::SchemefulSite& site,
    const net::SchemefulSite* top_frame_site,
    const std::set<net::SchemefulSite>& party_context,
    bool infer_singleton_sets) const {
  const net::SchemefulSite* site_owner = FindOwner(site, infer_singleton_sets);
  if (!site_owner)
    return false;

  const auto is_owned_by_site_owner =
      [this, site_owner,
       infer_singleton_sets](const net::SchemefulSite& context_site) -> bool {
    const net::SchemefulSite* context_owner =
        FindOwner(context_site, infer_singleton_sets);
    return context_owner && *context_owner == *site_owner;
  };

  if (top_frame_site && !is_owned_by_site_owner(*top_frame_site))
    return false;

  return base::ranges::all_of(party_context, is_owned_by_site_owner);
}

net::SamePartyContext FirstPartySets::ComputeContext(
    const net::SchemefulSite& site,
    const net::SchemefulSite* top_frame_site,
    const std::set<net::SchemefulSite>& party_context) const {
  net::SamePartyContext::Type context_type = ContextTypeFromBool(
      IsContextSamePartyWithSite(site, top_frame_site, party_context,
                                 false /* infer_singleton_sets */));
  net::SamePartyContext::Type ancestors = ContextTypeFromBool(
      IsContextSamePartyWithSite(site, top_frame_site, party_context,
                                 true /* infer_singleton_sets */));
  net::SamePartyContext::Type top_resource =
      ContextTypeFromBool(IsContextSamePartyWithSite(
          site, top_frame_site, {}, true /* infer_singleton_sets */));

  return net::SamePartyContext(context_type, ancestors, top_resource);
}

net::FirstPartySetsContextType FirstPartySets::ComputeContextType(
    const net::SchemefulSite& site,
    const absl::optional<net::SchemefulSite>& top_frame_site,
    const std::set<net::SchemefulSite>& party_context) const {
  constexpr bool infer_singleton_sets = true;
  const net::SchemefulSite* site_owner = FindOwner(site, infer_singleton_sets);
  // Note: the `party_context` consists of the intermediate frames (for frame
  // requests) or intermediate frames and current frame for subresource
  // requests.
  const bool is_homogeneous = base::ranges::all_of(
      party_context, [&](const net::SchemefulSite& middle_site) {
        return *FindOwner(middle_site, infer_singleton_sets) == *site_owner;
      });
  if (!top_frame_site.has_value()) {
    return is_homogeneous
               ? net::FirstPartySetsContextType::kTopFrameIgnoredHomogeneous
               : net::FirstPartySetsContextType::kTopFrameIgnoredMixed;
  }
  if (*FindOwner(*top_frame_site, infer_singleton_sets) != *site_owner)
    return net::FirstPartySetsContextType::kTopResourceMismatch;

  return is_homogeneous
             ? net::FirstPartySetsContextType::kHomogeneous
             : net::FirstPartySetsContextType::kTopResourceMatchMixed;
}

const net::SchemefulSite* FirstPartySets::FindOwner(
    const net::SchemefulSite& site,
    bool infer_singleton_sets) const {
  const auto it = sets_.find(site);
  if (it == sets_.end())
    return infer_singleton_sets ? &site : nullptr;
  return &it->second;
}

bool FirstPartySets::IsInNontrivialFirstPartySet(
    const net::SchemefulSite& site) const {
  return base::Contains(sets_, site);
}

base::flat_map<net::SchemefulSite, std::set<net::SchemefulSite>>
FirstPartySets::Sets() const {
  base::flat_map<net::SchemefulSite, std::set<net::SchemefulSite>> sets;

  for (const auto& pair : sets_) {
    const net::SchemefulSite& member = pair.first;
    const net::SchemefulSite& owner = pair.second;
    auto set = sets.find(owner);
    if (set == sets.end()) {
      sets.emplace(owner, std::initializer_list<net::SchemefulSite>{member});
    } else {
      set->second.insert(member);
    }
  }

  return sets;
}

void FirstPartySets::ApplyManuallySpecifiedSet() {
  if (!manually_specified_set_)
    return;

  const net::SchemefulSite& manual_owner = manually_specified_set_->first;
  const base::flat_set<net::SchemefulSite>& manual_members =
      manually_specified_set_->second;

  const auto was_manually_provided =
      [&manual_members, &manual_owner](const net::SchemefulSite& site) {
        return site == manual_owner || manual_members.contains(site);
      };

  // Erase the intersection between the manually-specified set and the
  // CU-supplied set, and any members whose owner was in the intersection.
  base::EraseIf(sets_, [&was_manually_provided](const auto& p) {
    return was_manually_provided(p.first) || was_manually_provided(p.second);
  });

  // Now remove singleton sets. We already removed any sites that were part
  // of the intersection, or whose owner was part of the intersection. This
  // leaves sites that *are* owners, which no longer have any (other)
  // members.
  std::set<net::SchemefulSite> owners_with_members;
  for (const auto& it : sets_) {
    if (it.first != it.second)
      owners_with_members.insert(it.second);
  }
  base::EraseIf(sets_, [&owners_with_members](const auto& p) {
    return p.first == p.second && !base::Contains(owners_with_members, p.first);
  });

  // Next, we must add the manually-added set to the parsed value.
  for (const net::SchemefulSite& member : manual_members) {
    sets_.emplace(member, manual_owner);
  }
  sets_.emplace(manual_owner, manual_owner);
}

}  // namespace network
