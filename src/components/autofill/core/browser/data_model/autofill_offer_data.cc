// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/data_model/autofill_offer_data.h"

#include <algorithm>

#include "base/ranges/algorithm.h"
#include "components/autofill/core/common/autofill_clock.h"

namespace autofill {

AutofillOfferData::AutofillOfferData() = default;

AutofillOfferData::~AutofillOfferData() = default;

AutofillOfferData::AutofillOfferData(const AutofillOfferData&) = default;

AutofillOfferData& AutofillOfferData::operator=(const AutofillOfferData&) =
    default;

bool AutofillOfferData::operator==(
    const AutofillOfferData& other_offer_data) const {
  return Compare(other_offer_data) == 0;
}

bool AutofillOfferData::operator!=(
    const AutofillOfferData& other_offer_data) const {
  return Compare(other_offer_data) != 0;
}

int AutofillOfferData::Compare(
    const AutofillOfferData& other_offer_data) const {
  int comparison = offer_id - other_offer_data.offer_id;
  if (comparison != 0)
    return comparison;

  comparison =
      offer_reward_amount.compare(other_offer_data.offer_reward_amount);
  if (comparison != 0)
    return comparison;

  if (expiry < other_offer_data.expiry)
    return -1;
  if (expiry > other_offer_data.expiry)
    return 1;

  comparison = offer_details_url.spec().compare(
      other_offer_data.offer_details_url.spec());
  if (comparison != 0)
    return comparison;

  std::vector<GURL> merchant_origins_copy = merchant_origins;
  std::vector<GURL> other_merchant_origins_copy =
      other_offer_data.merchant_origins;
  std::sort(merchant_origins_copy.begin(), merchant_origins_copy.end());
  std::sort(other_merchant_origins_copy.begin(),
            other_merchant_origins_copy.end());
  if (merchant_origins_copy < other_merchant_origins_copy)
    return -1;
  if (merchant_origins_copy > other_merchant_origins_copy)
    return 1;

  std::vector<int64_t> eligible_instrument_id_copy = eligible_instrument_id;
  std::vector<int64_t> other_eligible_instrument_id_copy =
      other_offer_data.eligible_instrument_id;
  std::sort(eligible_instrument_id_copy.begin(),
            eligible_instrument_id_copy.end());
  std::sort(other_eligible_instrument_id_copy.begin(),
            other_eligible_instrument_id_copy.end());
  if (eligible_instrument_id_copy < other_eligible_instrument_id_copy)
    return -1;
  if (eligible_instrument_id_copy > other_eligible_instrument_id_copy)
    return 1;

  comparison = promo_code.compare(other_offer_data.promo_code);
  if (comparison != 0)
    return comparison;

  comparison = display_strings.value_prop_text.compare(
      other_offer_data.display_strings.value_prop_text);
  if (comparison != 0)
    return comparison;

  comparison = display_strings.see_details_text.compare(
      other_offer_data.display_strings.see_details_text);
  if (comparison != 0)
    return comparison;

  comparison = display_strings.usage_instructions_text.compare(
      other_offer_data.display_strings.usage_instructions_text);
  if (comparison != 0)
    return comparison;

  return 0;
}

bool AutofillOfferData::IsCardLinkedOffer() const {
  // Card-linked offers have at least one |eligible_instrument_id|.
  return !eligible_instrument_id.empty();
}

bool AutofillOfferData::IsPromoCodeOffer() const {
  // Promo code offers have the promo code field populated.
  return !promo_code.empty();
}

bool AutofillOfferData::IsActiveAndEligibleForOrigin(const GURL& origin) const {
  return expiry > AutofillClock::Now() &&
         base::ranges::count(merchant_origins, origin) > 0;
}

}  // namespace autofill
