// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/cart/cart_discount_fetcher.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/sequence_checker.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/cart/cart_db.h"
#include "chrome/browser/cart/cart_db_content.pb.h"
#include "chrome/browser/cart/cart_discount_metric_collector.h"
#include "chrome/browser/endpoint_fetcher/endpoint_fetcher.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/grit/generated_resources.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/cross_thread_pending_shared_url_loader_factory.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/base/l10n/l10n_util.h"

namespace {
const char kPostMethod[] = "POST";
const char kContentType[] = "application/json; charset=UTF-8";

const char kFetchDiscountsEndpoint[] =
    "https://memex-pa.googleapis.com/v1/shopping/cart/discounts";
const int64_t kTimeoutMs = 30000;

struct DiscountInfo {
  std::vector<cart_db::DiscountInfoProto> discount_list;
  int highest_amount_off;
  int highest_percent_off;

  DiscountInfo(std::vector<cart_db::DiscountInfoProto> discount_list,
               int highest_amount_off,
               int highest_percent_off)
      : discount_list(std::move(discount_list)),
        highest_amount_off(highest_amount_off),
        highest_percent_off(highest_percent_off) {}

  ~DiscountInfo() = default;
  DiscountInfo(const DiscountInfo& other) = delete;
  DiscountInfo& operator=(const DiscountInfo& other) = delete;
  DiscountInfo(DiscountInfo&& other) = default;
  DiscountInfo& operator=(DiscountInfo&& other) = default;
};

// TODO(crbug.com/1207197): Consolidate to one util method to get string.
std::string GetMerchantUrl(const base::Value* merchant_identifier) {
  DCHECK(merchant_identifier->is_dict());

  // TODO(crbug.com/1207197): Use a constant instead.
  const base::Value* value = merchant_identifier->FindKey("cartUrl");
  if (!value || !value->is_string()) {
    NOTREACHED() << "Missing cart_url or it is not a string";
    return "";
  }

  return value->GetString();
}

std::string GetMerchantId(const base::Value* merchant_identifier) {
  DCHECK(merchant_identifier->is_dict());

  const base::Value* value = merchant_identifier->FindKey("merchantId");
  if (!value || !value->is_string()) {
    NOTREACHED() << "Missing merchant_id or it is not a string";
    return "";
  }

  return value->GetString();
}

DiscountInfo CovertToDiscountInfo(const base::Value* rule_discount_list) {
  std::vector<cart_db::DiscountInfoProto> cart_discounts;

  if (!rule_discount_list || !rule_discount_list->is_list()) {
    NOTREACHED() << "Missing rule_Discounts or it is not a list";
    return DiscountInfo(cart_discounts, 0, 0);
  }

  cart_discounts.reserve(rule_discount_list->GetList().size());

  int highest_percent_off = 0;
  int64_t highest_amount_off = 0;
  for (const auto& rule_discount : rule_discount_list->GetList()) {
    cart_db::DiscountInfoProto discount_proto;

    // Parse ruleId
    const base::Value* rule_id_value = rule_discount.FindKey("ruleId");
    if (!rule_id_value || !rule_id_value->is_string()) {
      NOTREACHED() << "Missing rule_id or it is not a string";
      continue;
    }
    discount_proto.set_rule_id(rule_id_value->GetString());

    // Parse merchantRuleId
    const base::Value* merchant_rule_id_value =
        rule_discount.FindKey("merchantRuleId");
    if (!merchant_rule_id_value || !merchant_rule_id_value->is_string()) {
      NOTREACHED() << "Missing merchant_rule_id or it is not a string";
      continue;
    }
    discount_proto.set_merchant_rule_id(merchant_rule_id_value->GetString());

    // Parse rawMerchantOfferId
    const base::Value* raw_merchant_offer_id_value =
        rule_discount.FindKey("rawMerchantOfferId");
    if (!raw_merchant_offer_id_value) {
      VLOG(1) << "raw_merchant_offer_id is empty";
    } else if (!raw_merchant_offer_id_value->is_string()) {
      NOTREACHED() << "raw_merchant_offer_id is not a string";
      continue;
    } else {
      discount_proto.set_raw_merchant_offer_id(
          raw_merchant_offer_id_value->GetString());
    }

    // Parse discount
    const base::Value* discount_value = rule_discount.FindKey("discount");
    if (!discount_value || !discount_value->is_dict()) {
      NOTREACHED() << "discount is missing or it is not a dictionary";
      continue;
    }

    if (discount_value->FindKey("percentOff")) {
      const base::Value* percent_off_value =
          discount_value->FindKey("percentOff");
      if (!percent_off_value->is_int()) {
        NOTREACHED() << "percent_off is not a int";
        continue;
      }
      int percent_off = percent_off_value->GetInt();
      discount_proto.set_percent_off(percent_off);
      highest_percent_off = std::max(highest_percent_off, percent_off);
    } else {
      const base::Value* amount_off_value =
          discount_value->FindKey("amountOff");
      if (!amount_off_value || !amount_off_value->is_dict()) {
        NOTREACHED() << "amount_off is not a dictionary";
        continue;
      }

      auto* money = discount_proto.mutable_amount_off();
      // Parse currencyCode
      const base::Value* currency_code_value =
          amount_off_value->FindKey("currencyCode");
      if (!currency_code_value || !currency_code_value->is_string()) {
        NOTREACHED() << "Missing currency_code or it is not a string";
        continue;
      }
      money->set_currency_code(currency_code_value->GetString());

      // Parse units
      const base::Value* units_value = amount_off_value->FindKey("units");
      if (!units_value || !units_value->is_string()) {
        NOTREACHED() << "Missing units or it is not a string, it is a "
                     << units_value->type();
        continue;
      }
      std::string units_string = units_value->GetString();
      money->set_units(units_string);
      int64_t units;
      base::StringToInt64(units_string, &units);
      highest_amount_off = std::max(highest_amount_off, units);

      // Parse nanos
      const base::Value* nanos_value = amount_off_value->FindKey("nanos");
      if (!nanos_value || !nanos_value->is_int()) {
        NOTREACHED() << "Missing nanos or it is not a int";
        continue;
      }
      money->set_nanos(nanos_value->GetInt());
    }

    cart_discounts.emplace_back(std::move(discount_proto));
  }

  return DiscountInfo(std::move(cart_discounts), highest_amount_off,
                      highest_percent_off);
}

bool ValidateResponse(const absl::optional<base::Value>& response) {
  if (!response) {
    NOTREACHED() << "Response is not valid";
    return false;
  }

  if (!response->is_dict()) {
    NOTREACHED()
        << "Wrong response format, response is not a dictionary. Response: "
        << response->DebugString();
    return false;
  }

  if (response->DictEmpty()) {
    VLOG(1) << "Response does not have value. Response: "
            << response->DebugString();
    return false;
  }
  return true;
}
}  // namespace

MerchantIdAndDiscounts::MerchantIdAndDiscounts(
    std::string merchant_id,
    std::vector<cart_db::DiscountInfoProto> discount_list,
    std::string discount_string)
    : merchant_id(std::move(merchant_id)),
      discount_list(std::move(discount_list)),
      highest_discount_string(std::move(discount_string)) {}

MerchantIdAndDiscounts::MerchantIdAndDiscounts(
    const MerchantIdAndDiscounts& other) = default;

MerchantIdAndDiscounts& MerchantIdAndDiscounts::operator=(
    const MerchantIdAndDiscounts& other) = default;

MerchantIdAndDiscounts::MerchantIdAndDiscounts(MerchantIdAndDiscounts&& other) =
    default;

MerchantIdAndDiscounts& MerchantIdAndDiscounts::operator=(
    MerchantIdAndDiscounts&& other) = default;

MerchantIdAndDiscounts::~MerchantIdAndDiscounts() = default;

std::unique_ptr<CartDiscountFetcher>
CartDiscountFetcherFactory::createFetcher() {
  return std::make_unique<CartDiscountFetcher>();
}

CartDiscountFetcherFactory::~CartDiscountFetcherFactory() = default;

CartDiscountFetcher::~CartDiscountFetcher() = default;

void CartDiscountFetcher::Fetch(
    std::unique_ptr<network::PendingSharedURLLoaderFactory> pending_factory,
    CartDiscountFetcherCallback callback,
    std::vector<CartDB::KeyAndValue> proto_pairs,
    bool is_oauth_fetch,
    const std::string access_token) {
  CartDiscountFetcher::FetchForDiscounts(
      std::move(pending_factory), std::move(callback), std::move(proto_pairs),
      is_oauth_fetch, std::move(access_token));
}

void CartDiscountFetcher::FetchForDiscounts(
    std::unique_ptr<network::PendingSharedURLLoaderFactory> pending_factory,
    CartDiscountFetcherCallback callback,
    std::vector<CartDB::KeyAndValue> proto_pairs,
    bool is_oauth_fetch,
    const std::string access_token) {
  auto fetcher = CreateEndpointFetcher(std::move(pending_factory),
                                       std::move(proto_pairs), is_oauth_fetch);

  auto* const fetcher_ptr = fetcher.get();
  fetcher_ptr->PerformRequest(
      base::BindOnce(&CartDiscountFetcher::OnDiscountsAvailable,
                     std::move(fetcher), std::move(callback)),
      access_token.c_str());
  CartDiscountMetricCollector::RecordFetchingForDiscounts();
}

std::unique_ptr<EndpointFetcher> CartDiscountFetcher::CreateEndpointFetcher(
    std::unique_ptr<network::PendingSharedURLLoaderFactory> pending_factory,
    std::vector<CartDB::KeyAndValue> proto_pairs,
    bool is_oauth_fetch) {
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("chrome_cart_discounts_lookup", R"(
        semantics {
          sender: "Chrome Cart"
          description:
            "Chrome looks up any discounts available to users' Chrome Shopping "
            "Carts. The Chrome Shopping Cart list is displayed on the New Tab "
            "Page, and it contains users' pending shopping Carts from merchant "
            "sites. Currently, this is a device based feature, Google does "
            "not save any data that is sent."
          trigger:
            "After user has given their consent and opt-in for the feature."
            "Afterwards, refreshes every 30 minutes."
          data:
            "The Chrome Cart data, includes the shopping site and products "
            "users have added to their shopping carts."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "You can enable or disable this feature via the Chrome NTP "
            "customized page in the bottom right corner of the NTP."
          policy_exception_justification: "No policy provided because this "
            "does not require user to sign in or sync, and they must given "
            "their consent before triggering this. And user can disable this "
            "feature."
        })");

  return std::make_unique<EndpointFetcher>(
      GURL(kFetchDiscountsEndpoint), kPostMethod, kContentType, kTimeoutMs,
      generatePostData(proto_pairs, base::Time::Now()), traffic_annotation,
      network::SharedURLLoaderFactory::Create(std::move(pending_factory)),
      is_oauth_fetch);
}

std::string CartDiscountFetcher::generatePostData(
    std::vector<CartDB::KeyAndValue> proto_pairs,
    base::Time current_time) {
  auto carts_list = std::make_unique<base::ListValue>();

  for (const CartDB::KeyAndValue& key_and_value : proto_pairs) {
    cart_db::ChromeCartContentProto cart_proto = key_and_value.second;

    auto cart_dict = std::make_unique<base::DictionaryValue>();
    // Set merchantIdentifier.
    auto merchant_dict = std::make_unique<base::DictionaryValue>();
    merchant_dict->SetString("cartUrl", cart_proto.merchant_cart_url());
    cart_dict->SetDictionary("merchantIdentifier", std::move(merchant_dict));

    // Set CartAbandonedTimeMinutes.
    int cart_abandoned_time_mintues =
        (current_time - base::Time::FromDoubleT(cart_proto.timestamp()))
            .InMinutes();
    cart_dict->SetInteger("cartAbandonedTimeMinutes",
                          cart_abandoned_time_mintues);

    // Set rawMerchantOffers.
    auto offer_list = std::make_unique<base::ListValue>();
    for (const auto& product_proto : cart_proto.product_infos()) {
      offer_list->Append(product_proto.product_id());
    }
    cart_dict->SetList("rawMerchantOffers", std::move(offer_list));

    // Add cart_dict to cart_list.
    carts_list->Append(std::move(cart_dict));
  }

  base::DictionaryValue request_dic;
  request_dic.SetList("carts", std::move(carts_list));

  std::string request_json;
  base::JSONWriter::Write(request_dic, &request_json);
  VLOG(2) << "Request body: " << request_json;
  return request_json;
}

void CartDiscountFetcher::OnDiscountsAvailable(
    std::unique_ptr<EndpointFetcher> endpoint_fetcher,
    CartDiscountFetcherCallback callback,
    std::unique_ptr<EndpointResponse> responses) {
  VLOG(2) << "Response: " << responses->response;
  CartDiscountMap cart_discount_map;
  absl::optional<base::Value> value =
      base::JSONReader::Read(responses->response);
  if (!ValidateResponse(value)) {
    std::move(callback).Run(std::move(cart_discount_map), false);
    return;
  }

  const base::Value* error_value = value->FindKey("error");
  if (error_value) {
    NOTREACHED() << "Error: " << responses->response;
    std::move(callback).Run(std::move(cart_discount_map), false);
    return;
  }

  const base::Value* discounts_list = value->FindKey("discounts");
  if (!discounts_list || !discounts_list->is_list()) {
    NOTREACHED() << "Missing discounts or it is not a list";
    std::move(callback).Run(std::move(cart_discount_map), false);
    return;
  }

  for (const auto& merchant_discount : discounts_list->GetList()) {
    // Parse merchant_identifier.
    const base::Value* merchant_identifier =
        merchant_discount.FindKey("merchantIdentifier");
    if (!merchant_identifier) {
      NOTREACHED() << "Missing merchant_identifier";
      continue;
    }
    std::string merchant_url = GetMerchantUrl(merchant_identifier);
    if (merchant_url.empty()) {
      continue;
    }

    std::string merchant_id = GetMerchantId(merchant_identifier);
    if (merchant_id.empty()) {
      continue;
    }

    // Parse rule discounts
    auto cart_discounts_info =
        CovertToDiscountInfo(merchant_discount.FindKey("ruleDiscounts"));
    if (!cart_discounts_info.discount_list.size()) {
      continue;
    }
    std::string discount_string_param;
    if (cart_discounts_info.highest_amount_off) {
      // TODO(meiliang): Use icu_formatter or
      // components/payments/core/currency_formatter to set the amount off.
      discount_string_param =
          "$" + base::NumberToString(cart_discounts_info.highest_amount_off);
    } else if (cart_discounts_info.highest_percent_off) {
      discount_string_param =
          base::NumberToString(cart_discounts_info.highest_percent_off) + "%";
    } else {
      NOTREACHED() << "Missing hightest discount info";
      continue;
    }

    std::string discount_string =
        cart_discounts_info.discount_list.size() > 1
            ? l10n_util::GetStringFUTF8(
                  IDS_NTP_MODULES_CART_DISCOUNT_CHIP_UP_TO_AMOUNT,
                  base::UTF8ToUTF16(discount_string_param))
            : l10n_util::GetStringFUTF8(
                  IDS_NTP_MODULES_CART_DISCOUNT_CHIP_AMOUNT,
                  base::UTF8ToUTF16(discount_string_param));

    MerchantIdAndDiscounts merchant_id_and_discounts(
        std::move(merchant_id), std::move(cart_discounts_info.discount_list),
        std::move(discount_string));
    cart_discount_map.emplace(merchant_url,
                              std::move(merchant_id_and_discounts));
  }

  bool is_tester = false;
  const base::Value* is_tester_value = value->FindKey("externalTester");
  if (is_tester_value && is_tester_value->is_bool()) {
    is_tester = is_tester_value->GetBool();
  }

  std::move(callback).Run(std::move(cart_discount_map), is_tester);
}
