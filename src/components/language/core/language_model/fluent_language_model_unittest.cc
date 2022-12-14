// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/core/language_model/fluent_language_model.h"

#include <cmath>

#include "base/strings/string_split.h"
#include "base/values.h"
#include "components/language/core/browser/language_prefs.h"
#include "components/language/core/browser/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"

#include "components/translate/core/browser/translate_pref_names.h"
#include "components/translate/core/browser/translate_prefs.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace language {

using testing::ElementsAre;
using Ld = LanguageModel::LanguageDetails;

constexpr static float kFloatEps = 0.00001f;
#if BUILDFLAG(IS_CHROMEOS_ASH)
const std::string& key = language::prefs::kPreferredLanguages;
#else
const std::string& key = language::prefs::kAcceptLanguages;
#endif

struct PrefRegistration {
  explicit PrefRegistration(user_prefs::PrefRegistrySyncable* registry) {
    language::LanguagePrefs::RegisterProfilePrefs(registry);
    translate::TranslatePrefs::RegisterProfilePrefs(registry);
  }
};

class FluentLanguageModelTest : public testing::Test {
 protected:
  FluentLanguageModelTest()
      : prefs_(new sync_preferences::TestingPrefServiceSyncable()),
        prefs_registration_(prefs_->registry()) {}

  std::unique_ptr<sync_preferences::TestingPrefServiceSyncable> prefs_;
  PrefRegistration prefs_registration_;
};

// Compares LanguageDetails.
MATCHER_P(EqualsLd, lang_details, "") {
  return arg.lang_code == lang_details.lang_code &&
         std::abs(arg.score - lang_details.score) < kFloatEps;
}

TEST_F(FluentLanguageModelTest, Defaults) {
  // By default, accept languages should all be fluent.
  const std::vector<std::string> accept_langs = base::SplitString(
      prefs_->GetString(key), ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  FluentLanguageModel model(prefs_.get());
  std::vector<Ld> languages = model.GetLanguages();

  EXPECT_EQ(accept_langs.size(), languages.size());
  for (size_t i = 0; i < accept_langs.size(); ++i)
    EXPECT_THAT(languages[i], EqualsLd(Ld(accept_langs[i], 1.0 / (1 + i))));
}

TEST_F(FluentLanguageModelTest, AllFluent) {
  prefs_->SetString(key, "ja,fr");
  base::Value fluent_languages(base::Value::Type::LIST);
  fluent_languages.Append("fr");
  fluent_languages.Append("ja");
  prefs_->Set(translate::prefs::kBlockedLanguages, fluent_languages);

  FluentLanguageModel model(prefs_.get());
  EXPECT_THAT(model.GetLanguages(), ElementsAre(EqualsLd(Ld("ja", 1.0f / 1)),
                                                EqualsLd(Ld("fr", 1.0f / 2))));
}

TEST_F(FluentLanguageModelTest, OneNonFluent) {
  prefs_->SetString(key, "ja,en,fr");
  base::Value fluent_languages(base::Value::Type::LIST);
  fluent_languages.Append("fr");
  fluent_languages.Append("ja");
  prefs_->Set(translate::prefs::kBlockedLanguages, fluent_languages);

  FluentLanguageModel model(prefs_.get());
  EXPECT_THAT(model.GetLanguages(), ElementsAre(EqualsLd(Ld("ja", 1.0f / 1)),
                                                EqualsLd(Ld("fr", 1.0f / 2))));
}

TEST_F(FluentLanguageModelTest, OneFluent) {
  prefs_->SetString(key, "ja,en,fr");
  base::Value fluent_languages(base::Value::Type::LIST);
  fluent_languages.Append("ja");
  prefs_->Set(translate::prefs::kBlockedLanguages, fluent_languages);

  FluentLanguageModel model(prefs_.get());
  EXPECT_THAT(model.GetLanguages(), ElementsAre(EqualsLd(Ld("ja", 1.0f / 1))));
}

}  // namespace language
