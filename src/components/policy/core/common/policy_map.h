// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_COMMON_POLICY_MAP_H_
#define COMPONENTS_POLICY_CORE_COMMON_POLICY_MAP_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/containers/flat_set.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/values.h"
#include "components/policy/core/common/external_data_fetcher.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_export.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace policy {

class PolicyMerger;

class PolicyMapTest;
FORWARD_DECLARE_TEST(PolicyMapTest, BlockedEntry);
FORWARD_DECLARE_TEST(PolicyMapTest, InvalidEntry);
FORWARD_DECLARE_TEST(PolicyMapTest, MergeFrom);

// A mapping of policy names to policy values for a given policy namespace.
class POLICY_EXPORT PolicyMap {
 public:
  // Types of messages that can be associated with policies. New types must be
  // added here in order to appear in the policy table.
  enum class MessageType { kInfo, kWarning, kError };

  // Types of conflicts that can be associated with policies. New conflict types
  // must be added here in order to appear in the policy table.
  enum class ConflictType { None, Override, Supersede };

  // Forward declare class so that it can be used in Entry.
  class EntryConflict;

  // Each policy maps to an Entry which keeps the policy value as well as other
  // relevant data about the policy.
  class POLICY_EXPORT Entry {
   public:
    PolicyLevel level = POLICY_LEVEL_RECOMMENDED;
    PolicyScope scope = POLICY_SCOPE_USER;
    // For debugging and displaying only. Set by provider delivering the policy.
    PolicySource source = POLICY_SOURCE_ENTERPRISE_DEFAULT;
    std::unique_ptr<ExternalDataFetcher> external_data_fetcher;
    std::vector<EntryConflict> conflicts;

    Entry();
    Entry(PolicyLevel level,
          PolicyScope scope,
          PolicySource source,
          absl::optional<base::Value> value,
          std::unique_ptr<ExternalDataFetcher> external_data_fetcher);
    ~Entry();

    Entry(Entry&&) noexcept;
    Entry& operator=(Entry&&) noexcept;

    // Returns a copy of |this|.
    Entry DeepCopy() const;

    base::Value* value();
    const base::Value* value() const;

    void set_value(absl::optional<base::Value> val);

    // Returns true if |this| has higher priority than |other|. The priority of
    // the fields are |level| > |scope| > |source|.
    bool has_higher_priority_than(const Entry& other) const;

    // Returns true if |this| equals |other|.
    bool Equals(const Entry& other) const;

    // Add a localized message given its l10n message ID.
    void AddMessage(MessageType type, int message_id);

    // Add a localized message given its l10n message ID and placeholder
    // args.
    void AddMessage(MessageType type,
                    int message_id,
                    std::vector<std::u16string>&& message_args);

    // Clear a message of a specific type given its l10n message ID.
    void ClearMessage(MessageType type, int message_id);

    // Adds a conflicting policy.
    void AddConflictingPolicy(Entry&& conflict);

    // Removes all the conflicts.
    void ClearConflicts();

    // Getter for |ignored_|.
    bool ignored() const;
    // Sets |ignored_| to true.
    void SetIgnored();

    // Marks the policy as blocked because it is not supported in the current
    // environment.
    void SetBlocked();

    // Marks the policy as invalid because it failed to validate against the
    // current schema.
    void SetInvalid();

    // Marks the policy as ignored because it does not share the priority of
    // its policy atomic group.
    void SetIgnoredByPolicyAtomicGroup();
    bool IsIgnoredByAtomicGroup() const;

    // Sets that the policy's value is a default value set by the policy
    // provider.
    void SetIsDefaultValue();
    bool IsDefaultValue() const;

    // Callback used to look up a localized string given its l10n message ID. It
    // should return a UTF-16 string.
    typedef base::RepeatingCallback<std::u16string(int message_id)>
        L10nLookupFunction;

    // Returns true if there is any message for |type|.
    bool HasMessage(MessageType type) const;

    // Returns localized messages as UTF-16 separated with LF characters. The
    // messages are organized according to message types (Warning, Error, etc).
    std::u16string GetLocalizedMessages(MessageType type,
                                        L10nLookupFunction lookup) const;

   private:
    absl::optional<base::Value> value_;
    bool ignored_ = false;
    bool is_default_value_ = false;

    // Stores all message IDs separated by message types.
    std::map<MessageType,
             std::map<int, absl::optional<std::vector<std::u16string>>>>
        message_ids_;
  };

  // Associates an Entry with a ConflictType.
  class POLICY_EXPORT EntryConflict {
   public:
    EntryConflict();
    EntryConflict(ConflictType type, Entry&& entry);
    ~EntryConflict();

    EntryConflict(EntryConflict&&) noexcept;
    EntryConflict& operator=(EntryConflict&&) noexcept;

    // Accessor methods for conflict type.
    void SetConflictType(ConflictType type);
    ConflictType conflict_type() const;

    // Accessor method for entry.
    const Entry& entry() const;

   private:
    ConflictType conflict_type_;
    Entry entry_;
  };

  typedef std::map<std::string, Entry> PolicyMapType;
  typedef PolicyMapType::const_iterator const_iterator;
  typedef PolicyMapType::iterator iterator;

  PolicyMap();
  PolicyMap(const PolicyMap&) = delete;
  PolicyMap& operator=(const PolicyMap&) = delete;
  PolicyMap(PolicyMap&& other) noexcept;
  PolicyMap& operator=(PolicyMap&& other) noexcept;
  ~PolicyMap();

  // Returns a weak reference to the entry currently stored for key |policy|,
  // or NULL if untrusted or not found. Ownership is retained by the PolicyMap.
  const Entry* Get(const std::string& policy) const;
  Entry* GetMutable(const std::string& policy);

  // Returns a weak reference to the value currently stored for key
  // |policy|, or NULL if not found. Ownership is retained by the PolicyMap.
  // This is equivalent to Get(policy)->value, when it doesn't return NULL.
  const base::Value* GetValue(const std::string& policy) const;
  base::Value* GetMutableValue(const std::string& policy);

  // Overwrites any existing information stored in the map for the key |policy|.
  // Resets the error for that policy to the empty string.
  void Set(const std::string& policy,
           PolicyLevel level,
           PolicyScope scope,
           PolicySource source,
           absl::optional<base::Value> value,
           std::unique_ptr<ExternalDataFetcher> external_data_fetcher);

  void Set(const std::string& policy, Entry entry);

  // Adds a localized message with |message_id| to the map for the key |policy|
  // that should be shown to the user alongisde the value in the policy UI. This
  // should only be called for policies that are already stored in the map.
  void AddMessage(const std::string& policy, MessageType type, int message_id);

  // Adds a localized message with |message_id| and placeholder arguments
  // |message_args| to the map for the key |policy| that should be shown to the
  // user alongisde the value in the policy UI. The number of placeholders in
  // the policy string corresponding to |message_id| must be equal to the number
  // of arguments in |message_args|. This should only be called for policies
  // that are already stored in the map.
  void AddMessage(const std::string& policy,
                  MessageType type,
                  int message_id,
                  std::vector<std::u16string>&& message_args);

  // Return True if the policy is set but its value is ignored because it does
  // not share the highest priority from its atomic group. Returns False if the
  // policy is active or not set.
  bool IsPolicyIgnoredByAtomicGroup(const std::string& policy) const;

  // For all policies, overwrite the PolicySource with |source|.
  void SetSourceForAll(PolicySource source);

  // For all policies, mark them as invalid, e.g. when a required schema failed
  // to load.
  void SetAllInvalid();

  // Erase the given |policy|, if it exists in this map.
  void Erase(const std::string& policy);

  // Erase the given iterator |it|. Returns the iterator following |it| (which
  // could be `map_.end()`).
  iterator EraseIt(const_iterator it);

  // Erase all entries for which |filter| returns true.
  void EraseMatching(
      const base::RepeatingCallback<bool(const const_iterator)>& filter);

  // Erase all entries for which |filter| returns false.
  void EraseNonmatching(
      const base::RepeatingCallback<bool(const const_iterator)>& filter);

  // Swaps the internal representation of |this| with |other|.
  void Swap(PolicyMap* other);

  // Returns a copy of |this|.
  PolicyMap Clone() const;

  // Merges policies from |other| into |this|. Existing policies are only
  // overridden by those in |other| if they have a higher priority, as defined
  // by Entry::has_higher_priority_than(). If a policy is contained in both
  // maps with the same priority, the current value in |this| is preserved.
  void MergeFrom(const PolicyMap& other);

  // Merge the policy values that are coming from different sources.
  void MergeValues(const std::vector<PolicyMerger*>& mergers);

  // Loads the values in |policies| into this PolicyMap. All policies loaded
  // will have |level|, |scope| and |source| in their entries. Existing entries
  // are replaced.
  void LoadFrom(const base::DictionaryValue* policies,
                PolicyLevel level,
                PolicyScope scope,
                PolicySource source);

  // Returns True if at least one shared ID is found in the user and device
  // affiliation ID sets.
  bool IsUserAffiliated() const;

  // Populates the set containing user affiliation ID strings.
  void SetUserAffiliationIds(const base::flat_set<std::string>& user_ids);

  // Returns the set containing user affiliation ID strings.
  const base::flat_set<std::string>& GetUserAffiliationIds() const;

  // Populates the set containing device affiliation ID strings.
  void SetDeviceAffiliationIds(const base::flat_set<std::string>& device_ids);

  // Returns the set containing device affiliation ID strings.
  const base::flat_set<std::string>& GetDeviceAffiliationIds() const;

  bool Equals(const PolicyMap& other) const;
  bool empty() const;
  size_t size() const;

  const_iterator begin() const;
  const_iterator end() const;
  iterator begin();
  iterator end();
  void Clear();

 private:
  FRIEND_TEST_ALL_PREFIXES(PolicyMapTest, BlockedEntry);
  FRIEND_TEST_ALL_PREFIXES(PolicyMapTest, InvalidEntry);
  FRIEND_TEST_ALL_PREFIXES(PolicyMapTest, MergeFrom);

  // Returns a weak reference to the entry currently stored for key |policy|,
  // or NULL if not found. Ownership is retained by the PolicyMap.
  const Entry* GetUntrusted(const std::string& policy) const;
  Entry* GetMutableUntrusted(const std::string& policy);

  // Helper function for Equals().
  static bool MapEntryEquals(const PolicyMapType::value_type& a,
                             const PolicyMapType::value_type& b);

  // Erase all entries for which |filter| returns |deletion_value|.
  void FilterErase(
      const base::RepeatingCallback<bool(const const_iterator)>& filter,
      bool deletion_value);

  PolicyMapType map_;

  // Affiliation
  bool is_user_affiliated_ = false;
  base::flat_set<std::string> user_affiliation_ids_;
  base::flat_set<std::string> device_affiliation_ids_;
};
}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_COMMON_POLICY_MAP_H_
