// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/optimization_guide/core/optimization_guide_switches.h"

#include "base/base64.h"
#include "base/command_line.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "components/optimization_guide/core/optimization_guide_test_util.h"
#include "components/optimization_guide/proto/hints.pb.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace optimization_guide {
namespace switches {

#if !defined(OS_WIN)

TEST(OptimizationGuideSwitchesTest, ParseHintsFetchOverrideFromCommandLine) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(kFetchHintsOverride,
                                                            "whatever.com");

  absl::optional<std::vector<std::string>> parsed_hosts =
      ParseHintsFetchOverrideFromCommandLine();

  EXPECT_TRUE(parsed_hosts.has_value());
  EXPECT_EQ(1ul, parsed_hosts.value().size());
  EXPECT_EQ("whatever.com", parsed_hosts.value()[0]);
}

TEST(OptimizationGuideSwitchesTest,
     ParseHintsFetchOverrideFromCommandLineMultipleHosts) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      kFetchHintsOverride, "whatever.com, whatever-2.com, ,");

  absl::optional<std::vector<std::string>> parsed_hosts =
      ParseHintsFetchOverrideFromCommandLine();

  EXPECT_TRUE(parsed_hosts.has_value());
  EXPECT_EQ(2ul, parsed_hosts.value().size());
  EXPECT_EQ("whatever.com", parsed_hosts.value()[0]);
  EXPECT_EQ("whatever-2.com", parsed_hosts.value()[1]);
}

TEST(OptimizationGuideSwitchesTest,
     ParseHintsFetchOverrideFromCommandLineNoSwitch) {
  absl::optional<std::vector<std::string>> parsed_hosts =
      ParseHintsFetchOverrideFromCommandLine();

  EXPECT_FALSE(parsed_hosts.has_value());
}

TEST(OptimizationGuideSwitchesTest, ParseComponentConfigFromCommandLine) {
  optimization_guide::proto::Configuration config;
  optimization_guide::proto::Hint* hint = config.add_hints();
  hint->set_key("somedomain.org");
  hint->set_key_representation(optimization_guide::proto::HOST);

  std::string encoded_config;
  config.SerializeToString(&encoded_config);
  base::Base64Encode(encoded_config, &encoded_config);

  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(kHintsProtoOverride,
                                                            encoded_config);

  std::unique_ptr<optimization_guide::proto::Configuration> parsed_config =
      ParseComponentConfigFromCommandLine();

  EXPECT_EQ(1, parsed_config->hints_size());
  EXPECT_EQ("somedomain.org", parsed_config->hints(0).key());
}

TEST(OptimizationGuideSwitchesTest,
     ParseComponentConfigFromCommandLineNotAProto) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(kHintsProtoOverride,
                                                            "not-a-proto");

  std::unique_ptr<optimization_guide::proto::Configuration> parsed_config =
      ParseComponentConfigFromCommandLine();

  EXPECT_EQ(nullptr, parsed_config);
}

TEST(OptimizationGuideSwitchesTest,
     ParseComponentConfigFromCommandLineSwitchNotSet) {
  std::unique_ptr<optimization_guide::proto::Configuration> parsed_config =
      ParseComponentConfigFromCommandLine();

  EXPECT_EQ(nullptr, parsed_config);
}

TEST(OptimizationGuideSwitchesTest,
     ParseComponentConfigFromCommandLineNotAConfiguration) {
  optimization_guide::proto::HostInfo host_info;
  host_info.set_host("whatever.com");
  std::string encoded_proto;
  host_info.SerializeToString(&encoded_proto);
  base::Base64Encode(encoded_proto, &encoded_proto);

  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(kHintsProtoOverride,
                                                            encoded_proto);

  std::unique_ptr<optimization_guide::proto::Configuration> parsed_config =
      ParseComponentConfigFromCommandLine();

  EXPECT_EQ(nullptr, parsed_config);
}

TEST(OptimizationGuideSwitchesTest,
     GetModelOverrideForOptimizationTargetSwitchNotSet) {
  absl::optional<
      std::pair<std::string, absl::optional<optimization_guide::proto::Any>>>
      file_path_and_metadata = GetModelOverrideForOptimizationTarget(
          optimization_guide::proto::OPTIMIZATION_TARGET_PAINFUL_PAGE_LOAD);

  EXPECT_EQ(absl::nullopt, file_path_and_metadata);
  EXPECT_FALSE(IsModelOverridePresent());
}

TEST(OptimizationGuideSwitchesTest,
     GetModelOverrideForOptimizationTargetEmptyInput) {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(kModelOverride);

  absl::optional<
      std::pair<std::string, absl::optional<optimization_guide::proto::Any>>>
      file_path_and_metadata = GetModelOverrideForOptimizationTarget(
          optimization_guide::proto::OPTIMIZATION_TARGET_PAINFUL_PAGE_LOAD);

  EXPECT_EQ(absl::nullopt, file_path_and_metadata);
}

TEST(OptimizationGuideSwitchesTest,
     GetModelOverrideForOptimizationTargetBadInput) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(kModelOverride,
                                                            "whatever");

  absl::optional<
      std::pair<std::string, absl::optional<optimization_guide::proto::Any>>>
      file_path_and_metadata = GetModelOverrideForOptimizationTarget(
          optimization_guide::proto::OPTIMIZATION_TARGET_PAINFUL_PAGE_LOAD);

  EXPECT_EQ(absl::nullopt, file_path_and_metadata);
}

TEST(OptimizationGuideSwitchesTest,
     GetModelOverrideForOptimizationTargetInvalidOptimizationTarget) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      kModelOverride,
      "notanoptimizationtarget:" + std::string(kTestAbsoluteFilePath));

  absl::optional<
      std::pair<std::string, absl::optional<optimization_guide::proto::Any>>>
      file_path_and_metadata = GetModelOverrideForOptimizationTarget(
          optimization_guide::proto::OPTIMIZATION_TARGET_PAINFUL_PAGE_LOAD);

  EXPECT_EQ(absl::nullopt, file_path_and_metadata);
}

TEST(OptimizationGuideSwitchesTest,
     GetModelOverrideForOptimizationTargetRelativeFilePath) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      kModelOverride, "OPTIMIZATION_TARGET_PAINFUL_PAGE_LOAD:" +
                          std::string(kTestRelativeFilePath));

  absl::optional<
      std::pair<std::string, absl::optional<optimization_guide::proto::Any>>>
      file_path_and_metadata = GetModelOverrideForOptimizationTarget(
          optimization_guide::proto::OPTIMIZATION_TARGET_PAINFUL_PAGE_LOAD);

  EXPECT_EQ(absl::nullopt, file_path_and_metadata);
}

TEST(OptimizationGuideSwitchesTest,
     GetModelOverrideForOptimizationTargetRelativeFilePathWithMetadata) {
  optimization_guide::proto::Any metadata;
  metadata.set_type_url("sometypeurl");
  std::string encoded_metadata;
  metadata.SerializeToString(&encoded_metadata);
  base::Base64Encode(encoded_metadata, &encoded_metadata);

  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      kModelOverride,
      base::StringPrintf("OPTIMIZATION_TARGET_PAINFUL_PAGE_LOAD:%s:%s",
                         kTestRelativeFilePath, encoded_metadata.c_str()));

  absl::optional<
      std::pair<std::string, absl::optional<optimization_guide::proto::Any>>>
      file_path_and_metadata = GetModelOverrideForOptimizationTarget(
          optimization_guide::proto::OPTIMIZATION_TARGET_PAINFUL_PAGE_LOAD);

  EXPECT_EQ(absl::nullopt, file_path_and_metadata);
}

TEST(OptimizationGuideSwitchesTest,
     GetModelOverrideForOptimizationTargetOneFilePath) {
  optimization_guide::proto::Any metadata;
  metadata.set_type_url("sometypeurl");
  std::string encoded_metadata;
  metadata.SerializeToString(&encoded_metadata);
  base::Base64Encode(encoded_metadata, &encoded_metadata);
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      kModelOverride,
      base::StringPrintf("OPTIMIZATION_TARGET_PAINFUL_PAGE_LOAD:%s:%s",
                         kTestAbsoluteFilePath, encoded_metadata.c_str()));

  absl::optional<
      std::pair<std::string, absl::optional<optimization_guide::proto::Any>>>
      file_path_and_metadata = GetModelOverrideForOptimizationTarget(
          optimization_guide::proto::OPTIMIZATION_TARGET_PAINFUL_PAGE_LOAD);

  EXPECT_EQ(kTestAbsoluteFilePath, file_path_and_metadata->first);
  EXPECT_EQ("sometypeurl", file_path_and_metadata->second->type_url());
}

TEST(OptimizationGuideSwitchesTest,
     GetModelOverrideForOptimizationTargetMultipleFilePath) {
  const char kOtherAbsoluteFilePath[] = "/other/file/path";
  optimization_guide::proto::Any metadata;
  metadata.set_type_url("sometypeurl");
  std::string encoded_metadata;
  metadata.SerializeToString(&encoded_metadata);
  base::Base64Encode(encoded_metadata, &encoded_metadata);
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      kModelOverride,
      base::StringPrintf("OPTIMIZATION_TARGET_PAINFUL_PAGE_LOAD:%s,"
                         "OPTIMIZATION_TARGET_PAGE_TOPICS:%s:%s",
                         kTestAbsoluteFilePath, kOtherAbsoluteFilePath,
                         encoded_metadata.c_str()));

  absl::optional<
      std::pair<std::string, absl::optional<optimization_guide::proto::Any>>>
      file_path_and_metadata = GetModelOverrideForOptimizationTarget(
          optimization_guide::proto::OPTIMIZATION_TARGET_PAINFUL_PAGE_LOAD);
  EXPECT_EQ(kTestAbsoluteFilePath, file_path_and_metadata->first);

  file_path_and_metadata = GetModelOverrideForOptimizationTarget(
      optimization_guide::proto::OPTIMIZATION_TARGET_PAGE_TOPICS);
  EXPECT_EQ(kOtherAbsoluteFilePath, file_path_and_metadata->first);
  EXPECT_EQ("sometypeurl", file_path_and_metadata->second->type_url());
}

#endif

}  // namespace switches
}  // namespace optimization_guide
