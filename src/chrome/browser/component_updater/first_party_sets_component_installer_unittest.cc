// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/first_party_sets_component_installer.h"

#include "base/callback_helpers.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/sequence_checker.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/version.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "net/base/features.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace component_updater {

namespace {
using ::testing::_;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;
}  // namespace

class FirstPartySetsComponentInstallerTest : public ::testing::Test {
 public:
  FirstPartySetsComponentInstallerTest() {
    CHECK(component_install_dir_.CreateUniqueTempDir());
    scoped_feature_list_.InitAndEnableFeature(net::features::kFirstPartySets);
  }

 protected:
  base::test::TaskEnvironment env_;

  base::ScopedTempDir component_install_dir_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(FirstPartySetsComponentInstallerTest, FeatureDisabled) {
  scoped_feature_list_.Reset();
  scoped_feature_list_.InitAndDisableFeature(net::features::kFirstPartySets);
  auto service =
      std::make_unique<component_updater::MockComponentUpdateService>();
  EXPECT_CALL(*service, RegisterComponent(_)).Times(0);
  RegisterFirstPartySetsComponent(service.get());

  env_.RunUntilIdle();
}

TEST_F(FirstPartySetsComponentInstallerTest, LoadsSets_OnComponentReady) {
  SEQUENCE_CHECKER(sequence_checker);
  const std::string expectation = "some first party sets";
  base::RunLoop run_loop;
  auto policy = std::make_unique<FirstPartySetsComponentInstallerPolicy>(
      base::BindLambdaForTesting([&](const std::string& got) {
        DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker);
        EXPECT_EQ(got, expectation);
        run_loop.Quit();
      }));

  ASSERT_TRUE(
      base::WriteFile(FirstPartySetsComponentInstallerPolicy::GetInstalledPath(
                          component_install_dir_.GetPath()),
                      expectation));

  policy->ComponentReady(base::Version(), component_install_dir_.GetPath(),
                         std::make_unique<base::DictionaryValue>());

  run_loop.Run();
}

// Test if a component has been installed, ComponentReady will be no-op when
// newer versions are installed.
TEST_F(FirstPartySetsComponentInstallerTest, IgnoreNewSets_OnComponentReady) {
  SEQUENCE_CHECKER(sequence_checker);
  const std::string sets_v1 = "first party sets v1";
  const std::string sets_v2 = "first party sets v2";

  base::ScopedTempDir dir_v1;
  CHECK(dir_v1.CreateUniqueTempDirUnderPath(component_install_dir_.GetPath()));
  base::ScopedTempDir dir_v2;
  CHECK(dir_v2.CreateUniqueTempDirUnderPath(component_install_dir_.GetPath()));

  int callback_calls = 0;
  FirstPartySetsComponentInstallerPolicy policy(
      // It should run only once for the first ComponentReady call.
      base::BindLambdaForTesting([&](const std::string& got) {
        DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker);
        EXPECT_EQ(got, sets_v1);
        callback_calls++;
      }));

  ASSERT_TRUE(
      base::WriteFile(FirstPartySetsComponentInstallerPolicy::GetInstalledPath(
                          dir_v1.GetPath()),
                      sets_v1));
  policy.ComponentReady(base::Version(), dir_v1.GetPath(),
                        std::make_unique<base::DictionaryValue>());

  // Install newer version of the component, which should not be picked up when
  // calling ComponentReady again.
  ASSERT_TRUE(
      base::WriteFile(FirstPartySetsComponentInstallerPolicy::GetInstalledPath(
                          dir_v2.GetPath()),
                      sets_v2));
  policy.ComponentReady(base::Version(), dir_v2.GetPath(),
                        std::make_unique<base::DictionaryValue>());

  env_.RunUntilIdle();

  EXPECT_EQ(callback_calls, 1);
}

TEST_F(FirstPartySetsComponentInstallerTest, LoadsSets_OnNetworkRestart) {
  SEQUENCE_CHECKER(sequence_checker);
  const std::string expectation = "some first party sets";

  // We do this in order for the static to memoize the install path.
  {
    base::RunLoop run_loop;
    FirstPartySetsComponentInstallerPolicy policy(
        base::BindLambdaForTesting([&](const std::string& got) {
          DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker);
          EXPECT_EQ(got, expectation);
          run_loop.Quit();
        }));

    ASSERT_TRUE(base::WriteFile(
        FirstPartySetsComponentInstallerPolicy::GetInstalledPath(
            component_install_dir_.GetPath()),
        expectation));

    policy.ComponentReady(base::Version(), component_install_dir_.GetPath(),
                          std::make_unique<base::DictionaryValue>());

    run_loop.Run();
  }

  {
    base::RunLoop run_loop;

    FirstPartySetsComponentInstallerPolicy::ReconfigureAfterNetworkRestart(
        base::BindLambdaForTesting([&](const std::string& got) {
          DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker);
          EXPECT_EQ(got, expectation);
          run_loop.Quit();
        }));

    run_loop.Run();
  }
}

// Test ReconfigureAfterNetworkRestart calls the callback with the correct
// version, i.e. the first installed component, even if there are newer versions
// installed after browser startup.
TEST_F(FirstPartySetsComponentInstallerTest, IgnoreNewSets_OnNetworkRestart) {
  SEQUENCE_CHECKER(sequence_checker);
  const std::string sets_v1 = "first party sets v1";
  const std::string sets_v2 = "first party sets v2";

  base::ScopedTempDir dir_v1;
  CHECK(dir_v1.CreateUniqueTempDirUnderPath(component_install_dir_.GetPath()));
  base::ScopedTempDir dir_v2;
  CHECK(dir_v2.CreateUniqueTempDirUnderPath(component_install_dir_.GetPath()));

  FirstPartySetsComponentInstallerPolicy policy(
      base::BindLambdaForTesting([&](const std::string& got) {
        DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker);
        EXPECT_EQ(got, sets_v1);
      }));

  ASSERT_TRUE(
      base::WriteFile(FirstPartySetsComponentInstallerPolicy::GetInstalledPath(
                          dir_v1.GetPath()),
                      sets_v1));
  policy.ComponentReady(base::Version(), dir_v1.GetPath(),
                        std::make_unique<base::DictionaryValue>());

  // Install newer version of the component, which should not be picked up when
  // calling ComponentReady again.
  ASSERT_TRUE(
      base::WriteFile(FirstPartySetsComponentInstallerPolicy::GetInstalledPath(
                          dir_v2.GetPath()),
                      sets_v2));
  policy.ComponentReady(base::Version(), dir_v2.GetPath(),
                        std::make_unique<base::DictionaryValue>());

  env_.RunUntilIdle();

  // ReconfigureAfterNetworkRestart calls the callback with the correct version.
  int callback_calls = 0;
  FirstPartySetsComponentInstallerPolicy::ReconfigureAfterNetworkRestart(
      base::BindLambdaForTesting([&](const std::string& got) {
        DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker);
        EXPECT_EQ(got, sets_v1);
        callback_calls++;
      }));

  env_.RunUntilIdle();
  EXPECT_EQ(callback_calls, 1);
}

TEST_F(FirstPartySetsComponentInstallerTest, GetInstallerAttributes_Disabled) {
  scoped_feature_list_.Reset();
  scoped_feature_list_.InitAndDisableFeature(net::features::kFirstPartySets);

  FirstPartySetsComponentInstallerPolicy policy(
      base::DoNothing::Repeatedly<const std::string&>());

  EXPECT_THAT(policy.GetInstallerAttributes(),
              UnorderedElementsAre(Pair(FirstPartySetsComponentInstallerPolicy::
                                            kDogfoodInstallerAttributeName,
                                        "false")));
}

TEST_F(FirstPartySetsComponentInstallerTest,
       GetInstallerAttributes_NonDogfooder) {
  scoped_feature_list_.Reset();
  scoped_feature_list_.InitAndEnableFeatureWithParameters(
      net::features::kFirstPartySets,
      {{net::features::kFirstPartySetsIsDogfooder.name, "false"}});

  FirstPartySetsComponentInstallerPolicy policy(
      base::DoNothing::Repeatedly<const std::string&>());

  EXPECT_THAT(policy.GetInstallerAttributes(),
              UnorderedElementsAre(Pair(FirstPartySetsComponentInstallerPolicy::
                                            kDogfoodInstallerAttributeName,
                                        "false")));
}

TEST_F(FirstPartySetsComponentInstallerTest, GetInstallerAttributes_Dogfooder) {
  scoped_feature_list_.Reset();
  scoped_feature_list_.InitAndEnableFeatureWithParameters(
      net::features::kFirstPartySets,
      {{net::features::kFirstPartySetsIsDogfooder.name, "true"}});

  FirstPartySetsComponentInstallerPolicy policy(
      base::DoNothing::Repeatedly<const std::string&>());

  EXPECT_THAT(policy.GetInstallerAttributes(),
              UnorderedElementsAre(Pair(FirstPartySetsComponentInstallerPolicy::
                                            kDogfoodInstallerAttributeName,
                                        "true")));
}

}  // namespace component_updater
