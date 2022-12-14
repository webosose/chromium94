// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browser_switcher/browser_switcher_sitelist.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/values.h"
#include "chrome/browser/browser_switcher/browser_switcher_prefs.h"
#include "chrome/browser/browser_switcher/ieem_sitelist_parser.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace browser_switcher {

namespace {

class TestBrowserSwitcherPrefs : public BrowserSwitcherPrefs {
 public:
  explicit TestBrowserSwitcherPrefs(PrefService* prefs)
      : BrowserSwitcherPrefs(prefs, nullptr) {}
};

std::unique_ptr<base::Value> StringArrayToValue(
    const std::vector<const char*>& strings) {
  std::vector<base::Value> values(strings.size());
  std::transform(strings.begin(), strings.end(), values.begin(),
                 [](const char* s) { return base::Value(s); });
  return std::make_unique<base::Value>(values);
}

}  // namespace

class BrowserSwitcherSitelistTest : public testing::TestWithParam<ParsingMode> {
 public:
  BrowserSwitcherSitelistTest() { parsing_mode_ = GetParam(); }

  void Initialize(const std::vector<const char*>& url_list,
                  const std::vector<const char*>& url_greylist,
                  bool enabled = true) {
    BrowserSwitcherPrefs::RegisterProfilePrefs(prefs_backend_.registry());
    prefs_backend_.SetManagedPref(prefs::kEnabled,
                                  std::make_unique<base::Value>(enabled));
    prefs_backend_.SetManagedPref(prefs::kUrlList,
                                  StringArrayToValue(url_list));
    prefs_backend_.SetManagedPref(prefs::kUrlGreylist,
                                  StringArrayToValue(url_greylist));
    prefs_backend_.SetManagedPref(
        prefs::kParsingMode,
        std::make_unique<base::Value>(static_cast<int>(parsing_mode())));
    prefs_ = std::make_unique<TestBrowserSwitcherPrefs>(&prefs_backend_);
    sitelist_ = std::make_unique<BrowserSwitcherSitelistImpl>(prefs_.get());
  }

  bool ShouldSwitch(const GURL& url) { return sitelist_->ShouldSwitch(url); }
  Decision GetDecision(const GURL& url) { return sitelist_->GetDecision(url); }

  sync_preferences::TestingPrefServiceSyncable* prefs_backend() {
    return &prefs_backend_;
  }
  BrowserSwitcherSitelist* sitelist() { return sitelist_.get(); }
  ParsingMode parsing_mode() { return parsing_mode_; }

 private:
  ParsingMode parsing_mode_;
  sync_preferences::TestingPrefServiceSyncable prefs_backend_;
  std::unique_ptr<BrowserSwitcherPrefs> prefs_;
  std::unique_ptr<BrowserSwitcherSitelist> sitelist_;
};

TEST_P(BrowserSwitcherSitelistTest, CanonicalizeRule) {
  std::string rule = "Example.Com";
  CanonicalizeRule(&rule);
  EXPECT_EQ("example.com", rule);

  rule = "Example.Com/";
  CanonicalizeRule(&rule);
  EXPECT_EQ("//example.com/", rule);

  rule = "!Example.Com/Abc";
  CanonicalizeRule(&rule);
  EXPECT_EQ("!//example.com/Abc", rule);

  rule = "/Example.Com";
  CanonicalizeRule(&rule);
  EXPECT_EQ("/Example.Com", rule);

  rule = "//Example.Com";
  CanonicalizeRule(&rule);
  EXPECT_EQ("//example.com/", rule);

  rule = "!//Example.Com";
  CanonicalizeRule(&rule);
  EXPECT_EQ("!//example.com/", rule);

  rule = "HTTP://EXAMPLE.COM";
  CanonicalizeRule(&rule);
  EXPECT_EQ("http://example.com/", rule);

  rule = "HTTP://EXAMPLE.COM/ABC";
  CanonicalizeRule(&rule);
  EXPECT_EQ("http://example.com/ABC", rule);

  rule = "User@Example.Com:8080/Test";
  CanonicalizeRule(&rule);
  EXPECT_EQ("//User@example.com:8080/Test", rule);
}

TEST_P(BrowserSwitcherSitelistTest, ShouldRedirectWildcard) {
  // A "*" by itself means everything matches.
  Initialize({"*"}, {});
  EXPECT_TRUE(ShouldSwitch(GURL("http://example.com/")));
  EXPECT_TRUE(ShouldSwitch(GURL("https://example.com/foobar/")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://example.com/foobar/")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://google.com/")));
}

TEST_P(BrowserSwitcherSitelistTest, ShouldRedirectHost) {
  // A string without slashes means compare the URL's host (case-insensitive).
  Initialize({"example.com"}, {});
  EXPECT_TRUE(ShouldSwitch(GURL("http://example.com/")));
  EXPECT_TRUE(ShouldSwitch(GURL("https://example.com/")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://subdomain.example.com/")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://example.com/foobar/")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://google.com/")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://example.ca/")));

  // For backwards compatibility, this should also match, even if it's not the
  // same host.
  EXPECT_EQ(parsing_mode() != ParsingMode::kIESiteListMode,
            ShouldSwitch(GURL("https://notexample.com/")));
}

TEST_P(BrowserSwitcherSitelistTest, ShouldRedirectHostNotLowerCase) {
  // Host is not in lowercase form, but we compare ignoring case.
  Initialize({"eXaMpLe.CoM"}, {});
  EXPECT_TRUE(ShouldSwitch(GURL("http://example.com/")));
}

TEST_P(BrowserSwitcherSitelistTest, ShouldRedirectWrongScheme) {
  Initialize({"example.com"}, {});
  // Scheme is not one of 'http', 'https' or 'file'.
  EXPECT_FALSE(ShouldSwitch(GURL("ftp://example.com/")));
}

TEST_P(BrowserSwitcherSitelistTest, ShouldRedirectPrefix) {
  // A string with slashes means check if it's a prefix (case-sensitive).
  Initialize({"http://example.com/foobar"}, {});
  EXPECT_TRUE(ShouldSwitch(GURL("http://example.com/foobar")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://example.com/foobar/subroute/")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://example.com/foobar#fragment")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://example.com/foobar?query=param")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://example.com/")));
  EXPECT_FALSE(ShouldSwitch(GURL("https://example.com/foobar")));
  EXPECT_EQ(parsing_mode() == ParsingMode::kIESiteListMode,
            ShouldSwitch(GURL("HTTP://EXAMPLE.COM/FOOBAR")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://subdomain.example.com/")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://google.com/")));
}

TEST_P(BrowserSwitcherSitelistTest, ShouldRedirectInvertedMatch) {
  // The most specific (i.e., longest string) rule should have priority.
  Initialize({"!subdomain.example.com", "example.com"}, {});
  EXPECT_TRUE(ShouldSwitch(GURL("http://example.com/")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://subdomain.example.com/")));
}

TEST_P(BrowserSwitcherSitelistTest, ShouldRedirectGreylist) {
  // The most specific (i.e., longest string) rule should have priority.
  Initialize({"example.com"}, {"http://example.com/login/"});
  EXPECT_TRUE(ShouldSwitch(GURL("http://example.com/")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://example.com/login/")));
}

TEST_P(BrowserSwitcherSitelistTest, ShouldRedirectGreylistWildcard) {
  Initialize({"*"}, {"*"});
  // If both are wildcards, prefer the greylist.
  EXPECT_FALSE(ShouldSwitch(GURL("http://example.com/")));
}

TEST_P(BrowserSwitcherSitelistTest, ShouldMatchAnySchema) {
  // URLs formatted like these don't include a schema, so should match both HTTP
  // and HTTPS.
  Initialize({"//example.com", "reddit.com/r/funny"}, {});
  EXPECT_TRUE(ShouldSwitch(GURL("http://example.com/something")));
  EXPECT_TRUE(ShouldSwitch(GURL("https://example.com/something")));
  EXPECT_TRUE(ShouldSwitch(GURL("file://example.com/foobar/")));
  EXPECT_FALSE(ShouldSwitch(GURL("https://foo.example.com/")));
  EXPECT_FALSE(ShouldSwitch(GURL("mailto://example.com")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://bad.com/example.com/")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://bad.com//example.com/")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://bad.com/hackme.html?example.com")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://reddit.com/r/funny")));
  EXPECT_TRUE(ShouldSwitch(GURL("https://reddit.com/r/funny")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://reddit.com/r/pics")));
  EXPECT_FALSE(ShouldSwitch(GURL("https://reddit.com/r/pics")));
}

TEST_P(BrowserSwitcherSitelistTest, ShouldRedirectPort) {
  Initialize(
      {"//example.com", "//test.com:3000", "lol.com:3000", "trololo.com"}, {});
  EXPECT_TRUE(ShouldSwitch(GURL("http://example.com:2000/something")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://test.com:3000/something")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://lol.com:3000/something")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://trololo.com/something")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://trololo.com:3000/something")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://test.com:2000/something")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://test.com:2000/something:3000")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://test.com/something:3000")));
}

// crbug.com/1209124
TEST_P(BrowserSwitcherSitelistTest, ShouldRedirectHostnamePrefix) {
  // A hostname rule (no "/") can match at the beginning of the hostname, not
  // just at the end.
  Initialize({"10.", "subdomain"}, {});
  EXPECT_EQ(parsing_mode() == ParsingMode::kIESiteListMode
                ? Decision(kStay, kDefault, "")
                : Decision(kGo, kSitelist, "10."),
            GetDecision(GURL("http://10.0.0.1/")));
  EXPECT_EQ(parsing_mode() == ParsingMode::kIESiteListMode
                ? Decision(kStay, kDefault, "")
                : Decision(kGo, kSitelist, "subdomain"),
            GetDecision(GURL("http://subdomain.example.com/")));
}

TEST_P(BrowserSwitcherSitelistTest, ShouldPickUpPrefChanges) {
  Initialize({}, {});
  prefs_backend()->SetManagedPref(prefs::kUrlList,
                                  StringArrayToValue({"example.com"}));
  prefs_backend()->SetManagedPref(prefs::kUrlGreylist,
                                  StringArrayToValue({"foo.example.com"}));
  EXPECT_TRUE(ShouldSwitch(GURL("http://example.com/")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://bar.example.com/")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://foo.example.com/")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://google.com/")));
}

TEST_P(BrowserSwitcherSitelistTest, ShouldIgnoreNonManagedPrefs) {
  Initialize({}, {});

  prefs_backend()->Set(prefs::kUrlList, *StringArrayToValue({"example.com"}));
  EXPECT_FALSE(ShouldSwitch(GURL("http://example.com/")));

  prefs_backend()->SetManagedPref(prefs::kUrlList,
                                  StringArrayToValue({"example.com"}));
  prefs_backend()->Set(prefs::kUrlGreylist,
                       *StringArrayToValue({"morespecific.example.com"}));
  EXPECT_TRUE(ShouldSwitch(GURL("http://morespecific.example.com/")));
}

TEST_P(BrowserSwitcherSitelistTest, SetIeemSitelist) {
  Initialize({}, {});
  ParsedXml ieem;
  ieem.rules = {"example.com"};
  sitelist()->SetIeemSitelist(std::move(ieem));
  EXPECT_TRUE(ShouldSwitch(GURL("http://example.com/")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://bar.example.com/")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://google.com/")));
}

TEST_P(BrowserSwitcherSitelistTest, SetExternalSitelist) {
  Initialize({}, {});
  ParsedXml external;
  external.rules = {"example.com"};
  sitelist()->SetExternalSitelist(std::move(external));
  EXPECT_TRUE(ShouldSwitch(GURL("http://example.com/")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://bar.example.com/")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://google.com/")));
}

TEST_P(BrowserSwitcherSitelistTest, SetExternalGreylist) {
  Initialize({"example.com"}, {});
  ParsedXml external;
  external.rules = {"foo.example.com"};
  sitelist()->SetExternalGreylist(std::move(external));
  EXPECT_TRUE(ShouldSwitch(GURL("http://example.com/")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://bar.example.com/")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://foo.example.com/")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://google.com/")));
}

TEST_P(BrowserSwitcherSitelistTest, All3Sources) {
  Initialize({"google.com"}, {"mail.google.com"});
  ParsedXml ieem;
  ieem.rules = {"example.com"};
  sitelist()->SetIeemSitelist(std::move(ieem));
  ParsedXml external;
  external.rules = {"yahoo.com"};
  sitelist()->SetExternalSitelist(std::move(external));
  EXPECT_TRUE(ShouldSwitch(GURL("http://google.com/")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://drive.google.com/")));
  EXPECT_FALSE(ShouldSwitch(GURL("http://mail.google.com/")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://example.com/")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://bar.example.com/")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://yahoo.com/")));
  EXPECT_TRUE(ShouldSwitch(GURL("http://news.yahoo.com/")));
}

TEST_P(BrowserSwitcherSitelistTest, BrowserSwitcherDisabled) {
  Initialize({"example.com"}, {}, false);
  EXPECT_FALSE(ShouldSwitch(GURL("http://example.com/")));
  EXPECT_EQ(Decision(kStay, kDisabled, ""),
            GetDecision(GURL("http://example.com/")));
}

TEST_P(BrowserSwitcherSitelistTest, CheckReason) {
  Initialize({"foo.invalid.com", "!example.com"},
             {"//foo.invalid.com/foobar", "invalid.com"});
  EXPECT_EQ(Decision(kStay, kProtocol, ""),
            GetDecision(GURL("ftp://example.com/")));
  EXPECT_EQ(Decision(kStay, kDefault, ""),
            GetDecision(GURL("http://google.com/")));
  EXPECT_EQ(Decision(kStay, kDefault, ""),
            GetDecision(GURL("http://bar.invalid.com/")));
  EXPECT_EQ(Decision(kStay, kSitelist, "!example.com"),
            GetDecision(GURL("http://example.com/")));
  EXPECT_EQ(Decision(kGo, kSitelist, "foo.invalid.com"),
            GetDecision(GURL("http://foo.invalid.com/")));
  EXPECT_EQ(Decision(kStay, kGreylist, "//foo.invalid.com/foobar"),
            GetDecision(GURL("http://foo.invalid.com/foobar")));
}

INSTANTIATE_TEST_SUITE_P(ParsingMode,
                         BrowserSwitcherSitelistTest,
                         testing::Values(ParsingMode::kDefault,
                                         ParsingMode::kIESiteListMode,
                                         // 999 should behave like kDefault
                                         static_cast<ParsingMode>(999)));

}  // namespace browser_switcher
