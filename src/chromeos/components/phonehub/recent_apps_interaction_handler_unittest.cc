// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/phonehub/recent_apps_interaction_handler.h"

#include <memory>

#include "chromeos/components/phonehub/notification.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace phonehub {
namespace {

class FakeClickHandler : public RecentAppClickObserver {
 public:
  FakeClickHandler() = default;
  ~FakeClickHandler() override = default;

  std::string get_package_name() { return package_name; }

  void OnRecentAppClicked(const std::string& recent_app_package_name) override {
    package_name = recent_app_package_name;
  }

 private:
  std::string package_name;
};

}  // namespace

class RecentAppsInteractionHandlerTest : public testing::Test {
 protected:
  RecentAppsInteractionHandlerTest() = default;
  RecentAppsInteractionHandlerTest(const RecentAppsInteractionHandlerTest&) =
      delete;
  RecentAppsInteractionHandlerTest& operator=(
      const RecentAppsInteractionHandlerTest&) = delete;
  ~RecentAppsInteractionHandlerTest() override = default;

  // testing::Test:
  void SetUp() override {
    interaction_handler_ = std::make_unique<RecentAppsInteractionHandler>();
    interaction_handler_->AddRecentAppClickObserver(&fake_click_handler_);
  }

  void TearDown() override {
    interaction_handler_->RemoveRecentAppClickObserver(&fake_click_handler_);
  }

  std::string GetPackageName() {
    return fake_click_handler_.get_package_name();
  }

  RecentAppsInteractionHandler& handler() { return *interaction_handler_; }

 private:
  FakeClickHandler fake_click_handler_;
  std::unique_ptr<RecentAppsInteractionHandler> interaction_handler_;
};

TEST_F(RecentAppsInteractionHandlerTest, RecentAppsClicked) {
  const char16_t expected_app_visible_name[] = u"Fake App";
  const char expected_package_name[] = "com.fakeapp";
  auto expected_app_metadata = Notification::AppMetadata(
      expected_app_visible_name, expected_package_name, gfx::Image());

  handler().NotifyRecentAppClicked(expected_package_name);

  EXPECT_EQ(expected_package_name, GetPackageName());
}

TEST_F(RecentAppsInteractionHandlerTest, RecentAppsUpdated) {
  const char16_t app_visible_name1[] = u"Fake App";
  const char package_name1[] = "com.fakeapp";
  auto app_metadata1 =
      Notification::AppMetadata(app_visible_name1, package_name1, gfx::Image());

  const char16_t app_visible_name2[] = u"Fake App2";
  const char package_name2[] = "com.fakeapp2";
  auto app_metadata2 =
      Notification::AppMetadata(app_visible_name2, package_name2, gfx::Image());
  const base::Time now = base::Time::Now();

  handler().NotifyRecentAppAddedOrUpdated(app_metadata1, now);
  EXPECT_EQ(1U, handler().recent_app_metadata_list_.size());
  EXPECT_EQ(now, handler().recent_app_metadata_list_[0].second);

  // The same package name only update last accessed timestamp.
  const base::Time next_minute =
      base::Time::Now() + base::TimeDelta::FromMinutes(1);
  handler().NotifyRecentAppAddedOrUpdated(app_metadata1, next_minute);
  EXPECT_EQ(1U, handler().recent_app_metadata_list_.size());
  EXPECT_EQ(next_minute, handler().recent_app_metadata_list_[0].second);

  const base::Time next_hour =
      base::Time::Now() + base::TimeDelta::FromHours(1);
  handler().NotifyRecentAppAddedOrUpdated(app_metadata2, next_hour);
  EXPECT_EQ(2U, handler().recent_app_metadata_list_.size());
}

TEST_F(RecentAppsInteractionHandlerTest, FetchRecentAppMetadataList) {
  const char16_t app_visible_name1[] = u"Fake App";
  const char package_name1[] = "com.fakeapp";
  auto app_metadata1 =
      Notification::AppMetadata(app_visible_name1, package_name1, gfx::Image());

  const char16_t app_visible_name2[] = u"Fake App2";
  const char package_name2[] = "com.fakeapp2";
  auto app_metadata2 =
      Notification::AppMetadata(app_visible_name2, package_name2, gfx::Image());

  const char16_t app_visible_name3[] = u"Fake App3";
  const char package_name3[] = "com.fakeapp3";
  auto app_metadata3 =
      Notification::AppMetadata(app_visible_name3, package_name3, gfx::Image());

  const base::Time now = base::Time::Now();
  const base::Time next_minute =
      base::Time::Now() + base::TimeDelta::FromMinutes(1);
  const base::Time next_hour =
      base::Time::Now() + base::TimeDelta::FromHours(1);

  handler().NotifyRecentAppAddedOrUpdated(app_metadata1, now);
  handler().NotifyRecentAppAddedOrUpdated(app_metadata2, next_minute);
  handler().NotifyRecentAppAddedOrUpdated(app_metadata3, next_hour);

  std::vector<Notification::AppMetadata> recent_apps_metadata_result =
      handler().FetchRecentAppMetadataList();

  EXPECT_EQ(3U, recent_apps_metadata_result.size());
  EXPECT_EQ(package_name3, recent_apps_metadata_result[0].package_name);
  EXPECT_EQ(package_name2, recent_apps_metadata_result[1].package_name);
  EXPECT_EQ(package_name1, recent_apps_metadata_result[2].package_name);

  const char16_t app_visible_name4[] = u"Fake App4";
  const char package_name4[] = "com.fakeapp4";
  auto app_metadata4 =
      Notification::AppMetadata(app_visible_name4, package_name4, gfx::Image());

  const char16_t app_visible_name5[] = u"Fake App5";
  const char package_name5[] = "com.fakeapp5";
  auto app_metadata5 =
      Notification::AppMetadata(app_visible_name5, package_name5, gfx::Image());

  const char16_t app_visible_name6[] = u"Fake App6";
  const char package_name6[] = "com.fakeapp6";
  auto app_metadata6 =
      Notification::AppMetadata(app_visible_name6, package_name6, gfx::Image());

  const base::Time next_two_hour =
      base::Time::Now() + base::TimeDelta::FromHours(2);
  const base::Time next_three_hour =
      base::Time::Now() + base::TimeDelta::FromHours(3);
  const base::Time next_four_hour =
      base::Time::Now() + base::TimeDelta::FromHours(4);

  handler().NotifyRecentAppAddedOrUpdated(app_metadata4, next_two_hour);
  handler().NotifyRecentAppAddedOrUpdated(app_metadata5, next_three_hour);
  handler().NotifyRecentAppAddedOrUpdated(app_metadata6, next_four_hour);

  const size_t max_most_recent_apps = 5;
  recent_apps_metadata_result = handler().FetchRecentAppMetadataList();
  EXPECT_EQ(max_most_recent_apps, recent_apps_metadata_result.size());

  EXPECT_EQ(package_name6, recent_apps_metadata_result[0].package_name);
  EXPECT_EQ(package_name5, recent_apps_metadata_result[1].package_name);
  EXPECT_EQ(package_name4, recent_apps_metadata_result[2].package_name);
  EXPECT_EQ(package_name3, recent_apps_metadata_result[3].package_name);
  EXPECT_EQ(package_name2, recent_apps_metadata_result[4].package_name);
}

}  // namespace phonehub
}  // namespace chromeos
