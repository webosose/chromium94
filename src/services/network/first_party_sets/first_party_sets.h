// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_FIRST_PARTY_SETS_FIRST_PARTY_SETS_H_
#define SERVICES_NETWORK_FIRST_PARTY_SETS_FIRST_PARTY_SETS_H_

#include <map>
#include <memory>
#include <set>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "net/base/schemeful_site.h"
#include "net/cookies/cookie_constants.h"
#include "net/cookies/same_party_context.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace network {

// Class FirstPartySets is a pseudo-singleton owned by NetworkService; it stores
// all known information about First-Party Sets state. This information is
// updated by the component updater via |ParseAndSet|.
class FirstPartySets {
 public:
  FirstPartySets();
  ~FirstPartySets();

  FirstPartySets(const FirstPartySets&) = delete;
  FirstPartySets& operator=(const FirstPartySets&) = delete;

  void SetManuallySpecifiedSet(const std::string& flag_value);

  // Overwrites the current members-to-owners map with the values in |raw_sets|,
  // which should be the JSON-encoded string representation of a collection of
  // set declarations according to the format specified in this document:
  // https://github.com/privacycg/first-party-sets. Returns a pointer to the
  // mapping, for testing.
  //
  // In case of invalid input, clears the current members-to-owners map, but
  // keeps any manually-specified set (i.e. a set provided on the command line).
  base::flat_map<net::SchemefulSite, net::SchemefulSite>* ParseAndSet(
      base::StringPiece raw_sets);

  // Returns whether the `site` is same-party with the `party_context`, and
  // `top_frame_site` (if it is not nullptr). That is, is the `site`'s owner the
  // same as the owners of every member of `party_context` and of
  // `top_frame_site`? Note: if `site` is not a member of a First-Party Set
  // (with more than one member), then this returns false. If `top_frame_site`
  // is nullptr, then it is ignored.
  bool IsContextSamePartyWithSite(
      const net::SchemefulSite& site,
      const net::SchemefulSite* top_frame_site,
      const std::set<net::SchemefulSite>& party_context,
      bool infer_singleton_sets) const;

  // Computes the SameParty context, indicating whether `site` is same-party
  // with `top_frame_site` (if not nullptr) and `party_context`. The context
  // includes the real context type, plus some additional "hypothetical" context
  // types for metrics.
  net::SamePartyContext ComputeContext(
      const net::SchemefulSite& site,
      const net::SchemefulSite* top_frame_site,
      const std::set<net::SchemefulSite>& party_context) const;

  // Computes the "type" of the context. I.e., categorizes contexts based on
  // whether the top frame site and resource URL are same-party; whether the top
  // frame site was ignored; whether the `party_context` is same-party with
  // everything else; etc.
  //
  // Since this metric may be used to inform decisions based on actual usage
  // patterns of sites on the web, this infers singleton sets. That is, it
  // treats sites that do not belong to a First-Party Set as belonging to an
  // implictly-declared singleton First-Party Set.
  net::FirstPartySetsContextType ComputeContextType(
      const net::SchemefulSite& site,
      const absl::optional<net::SchemefulSite>& top_frame_site,
      const std::set<net::SchemefulSite>& party_context) const;

  // Returns whether the `site` is a member of a non-trivial (i.e.
  // non-singleton) First-Party Set.
  bool IsInNontrivialFirstPartySet(const net::SchemefulSite& site) const;

  int64_t size() const { return sets_.size(); }

  // Returns a mapping from owner to set members. For convenience of iteration,
  // the members of the set includes the owner.
  base::flat_map<net::SchemefulSite, std::set<net::SchemefulSite>> Sets() const;

 private:
  // Returns a pointer to `site`'s owner (optionally inferring a singleton set
  // if necessary), or `nullptr` if `site` has no owner. Must not return
  // `nullptr` if `infer_singleton_sets` is true.
  const net::SchemefulSite* FindOwner(const net::SchemefulSite& site,
                                      bool infer_singleton_sets) const;

  // We must ensure there's no intersection between the manually-specified set
  // and the sets that came from Component Updater. (When reconciling the
  // manually-specified set and `sets_`, entries in the manually-specified set
  // always win.) We must also ensure that `sets_` includes the set described by
  // `manually_specified_set_`.
  void ApplyManuallySpecifiedSet();

  // Represents the mapping of site -> site, where keys are members of sets, and
  // values are owners of the sets. Owners are explicitly represented as members
  // of the set.
  base::flat_map<net::SchemefulSite, net::SchemefulSite> sets_;
  absl::optional<
      std::pair<net::SchemefulSite, base::flat_set<net::SchemefulSite>>>
      manually_specified_set_;
};

}  // namespace network

#endif  // SERVICES_NETWORK_FIRST_PARTY_SETS_FIRST_PARTY_SETS_H_
