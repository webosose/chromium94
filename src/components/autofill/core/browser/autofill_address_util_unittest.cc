// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/autofill_address_util.h"

#include "base/guid.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/resource/resource_bundle.h"

namespace autofill {

using ::testing::ElementsAre;

class GetEnvelopeStyleAddressTest : public ::testing::Test {
 public:
  void SetUp() override {
    orig_resource_bundle_ =
        ui::ResourceBundle::SwapSharedInstanceForTesting(nullptr);
    ui::ResourceBundle::InitSharedInstanceWithLocale(
        GetLocale(), /*delegate=*/nullptr,
        ui::ResourceBundle::DO_NOT_LOAD_COMMON_RESOURCES);
  }

  void TearDown() override {
    ui::ResourceBundle::CleanupSharedInstance();
    ui::ResourceBundle::SwapSharedInstanceForTesting(orig_resource_bundle_);
  }

  std::string GetLocale() { return "en-US"; }

 private:
  ui::ResourceBundle* orig_resource_bundle_;
};

TEST_F(GetEnvelopeStyleAddressTest, Sanity) {
  AutofillProfile profile = test::GetFullProfile();
  std::u16string address =
      GetEnvelopeStyleAddress(profile, GetLocale(), /*include_recipient=*/true,
                              /*include_country=*/true);

  // The exact format of the address depends on the format in the
  // libaddressinput library. Let's avoid testing the exact format, but test
  // some more highlevel conditions that are less probable to change.

  // The full name should be part of the envelope style address.
  EXPECT_NE(address.find(
                profile.GetInfo(NAME_FULL_WITH_HONORIFIC_PREFIX, GetLocale())),
            std::string::npos);

  // City should be part of the envelope style address.
  EXPECT_NE(address.find(profile.GetInfo(ADDRESS_HOME_CITY, GetLocale())),
            std::string::npos);

  // Check that some literals are properly returned.
  // The US envelope style address should contain a comma.
  EXPECT_NE(address.find(u","), std::string::npos);

  // The US envelope style address should contains at least one newline.
  EXPECT_NE(address.find(u"\n"), std::string::npos);

  // The country should be returned.
  EXPECT_NE(address.find(u"United States"), std::string::npos);

  // The country shouldn't be returned when include_country=false.
  EXPECT_EQ(
      GetEnvelopeStyleAddress(profile, GetLocale(), /*include_recipient=*/true,
                              /*include_country=*/false)
          .find(u"United States"),
      std::string::npos);

  // The recipient shouldn't be returned when include_recipient=false.
  EXPECT_EQ(
      GetEnvelopeStyleAddress(profile, GetLocale(), /*include_recipient=*/false,
                              /*include_country=*/true)
          .find(u"John H. Doe"),
      std::string::npos);
}

TEST_F(GetEnvelopeStyleAddressTest, EmptyFullname) {
  AutofillProfile profile(base::GenerateGUID(), /*origin=*/"");
  test::SetProfileInfo(&profile, /*first_name=*/"", /*middle_name=*/"",
                       /*last_name=*/"", "johndoe@hades.com", "Underworld",
                       "666 Erebus St.", "Apt 8", "Elysium", "CA", "91111",
                       "US", "16502111111");

  std::u16string address =
      GetEnvelopeStyleAddress(profile, GetLocale(), /*include_recipient=*/true,
                              /*include_country=*/true);
  // The US envelope style address should *not* start with a new line.
  EXPECT_NE(address.front(), '\n');
}

// Tests that when the company is empty, the envelope style address doesn't
// contain empty lines.
TEST_F(GetEnvelopeStyleAddressTest, EmptyCompanyShouldHaveNoEmptyLines) {
  AutofillProfile profile(base::GenerateGUID(), /*origin=*/"");
  test::SetProfileInfo(&profile, "FirstName", "MiddleName", "LastName",
                       "johndoe@hades.com", /*company=*/"", "666 Erebus St.",
                       "Apt 8", "Elysium", "CA", "91111", "US", "16502111111");

  std::u16string address =
      GetEnvelopeStyleAddress(profile, GetLocale(), /*include_recipient=*/true,
                              /*include_country=*/true);
  // There should be no consecutive new lines.
  EXPECT_EQ(address.find(u"\n\n"), std::string::npos);
}

// Tests that when the state is empty, the envelope style address doesn't
// contains consecutive white spaces.
TEST_F(GetEnvelopeStyleAddressTest,
       EmptyStateShouldHaveNoConsecutiveWhitespaces) {
  AutofillProfile profile(base::GenerateGUID(), /*origin=*/"");
  test::SetProfileInfo(&profile, "FirstName", "MiddleName", "LastName",
                       "johndoe@hades.com", "Underworld", "666 Erebus St.",
                       "Apt 8", "Elysium", /*state=*/"", "91111", "US",
                       "16502111111");

  std::u16string address =
      GetEnvelopeStyleAddress(profile, GetLocale(), /*include_recipient=*/true,
                              /*include_country=*/true);
  // There should be no consecutive white spaces.
  EXPECT_EQ(address.find(u"  "), std::string::npos);
}

TEST_F(GetEnvelopeStyleAddressTest, NoDifferencesBetweenIdenticalProfiles) {
  AutofillProfile profile = test::GetFullProfile();
  EXPECT_TRUE(GetProfileDifferenceForUi(profile, profile, "en-US").empty());
}

TEST_F(GetEnvelopeStyleAddressTest, DiffereceInUiWhenFullnameDiffers) {
  AutofillProfile profile1 = test::GetFullProfile();
  profile1.SetInfo(NAME_FULL, u"John H. Doe", "en-US");

  AutofillProfile profile2 = profile1;
  profile2.SetInfo(NAME_FULL, u"John Doe", "en-US");

  EXPECT_THAT(
      GetProfileDifferenceForUi(profile1, profile2, "en-US"),
      ElementsAre(ProfileValueDifference{NAME_FULL_WITH_HONORIFIC_PREFIX,
                                         u"John H. Doe", u"John Doe"}));
}

TEST_F(GetEnvelopeStyleAddressTest, DiffereceInUiWhenZipcodeDiffers) {
  AutofillProfile profile1 = test::GetFullProfile();
  profile1.SetInfo(ADDRESS_HOME_ZIP, u"91111", "en-US");

  AutofillProfile profile2 = profile1;
  profile2.SetInfo(ADDRESS_HOME_ZIP, u"90000", "en-US");

  EXPECT_THAT(GetProfileDifferenceForUi(profile1, profile2, "en-US"),
              ElementsAre(ProfileValueDifference{
                  ADDRESS_HOME_ADDRESS,
                  GetEnvelopeStyleAddress(profile1, "en-US",
                                          /*include_recipient=*/false,
                                          /*include_country=*/true),
                  GetEnvelopeStyleAddress(profile2, "en-US",
                                          /*include_recipient=*/false,
                                          /*include_country=*/true)}));
}

TEST(GetProfileDescription, NameAndAddress) {
  AutofillProfile profile = test::GetFullProfile();
  std::u16string description = GetProfileDescription(
      profile, "en-US", /*include_address_and_contacts=*/true);
  // Should contain full name and address line 1.
  EXPECT_EQ(description, u"John H. Doe, 666 Erebus St.");
}

TEST(GetProfileDescription, EmptyName) {
  AutofillProfile profile = test::GetFullProfile();
  profile.SetInfo(NAME_FULL, u"", "en-US");
  std::u16string description = GetProfileDescription(
      profile, "en-US", /*include_address_and_contacts=*/true);
  // Should contain 2 address components: address lines 1 & 2.
  EXPECT_EQ(description, u"666 Erebus St., Apt 8");
}

TEST(GetProfileDescription, NotIncludeAddressAndContacts) {
  AutofillProfile profile = test::GetFullProfile();
  std::u16string description = GetProfileDescription(
      profile, "en-US", /*include_address_and_contacts=*/false);
  // Should contain full name only.
  EXPECT_EQ(description, u"John H. Doe");
}

}  // namespace autofill
