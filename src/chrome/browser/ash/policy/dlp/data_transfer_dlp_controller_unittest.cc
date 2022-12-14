// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/policy/dlp/data_transfer_dlp_controller.h"

#include <memory>

#include "base/stl_util.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/mock_callback.h"
#include "chrome/browser/ash/policy/dlp/dlp_histogram_helper.h"
#include "chrome/browser/ash/policy/dlp/dlp_policy_event.pb.h"
#include "chrome/browser/ash/policy/dlp/dlp_reporting_manager.h"
#include "chrome/browser/ash/policy/dlp/dlp_reporting_manager_test_helper.h"
#include "chrome/browser/ash/policy/dlp/dlp_rules_manager.h"
#include "chrome/browser/ash/policy/dlp/mock_dlp_rules_manager.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/account_id/account_id.h"
#include "components/reporting/client/mock_report_queue.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/data_transfer_policy/data_transfer_endpoint.h"
#include "url/origin.h"

namespace policy {

namespace {

constexpr char kExample1Url[] = "https://www.example1.com";
constexpr char kExample2Url[] = "https://www.example2.com";

class MockDlpController : public DataTransferDlpController {
 public:
  explicit MockDlpController(const DlpRulesManager& dlp_rules_manager)
      : DataTransferDlpController(dlp_rules_manager) {}

  MOCK_METHOD2(NotifyBlockedPaste,
               void(const ui::DataTransferEndpoint* const data_src,
                    const ui::DataTransferEndpoint* const data_dst));

  MOCK_METHOD2(NotifyBlockedDrop,
               void(const ui::DataTransferEndpoint* const data_src,
                    const ui::DataTransferEndpoint* const data_dst));

  MOCK_METHOD2(WarnOnPaste,
               void(const ui::DataTransferEndpoint* const data_src,
                    const ui::DataTransferEndpoint* const data_dst));

  MOCK_METHOD4(WarnOnBlinkPaste,
               void(const ui::DataTransferEndpoint* const data_src,
                    const ui::DataTransferEndpoint* const data_dst,
                    content::WebContents* web_contents,
                    base::OnceCallback<void(bool)> paste_cb));

  MOCK_METHOD1(ShouldPasteOnWarn,
               bool(const ui::DataTransferEndpoint* const data_dst));

  MOCK_METHOD1(ShouldCancelOnWarn,
               bool(const ui::DataTransferEndpoint* const data_dst));
};

// Creates a new MockDlpRulesManager for the given |context|.
std::unique_ptr<KeyedService> BuildDlpRulesManager(
    content::BrowserContext* context) {
  return std::make_unique<::testing::StrictMock<MockDlpRulesManager>>();
}

absl::optional<ui::DataTransferEndpoint> CreateEndpoint(
    ui::EndpointType* type,
    bool notify_if_restricted) {
  if (type && *type == ui::EndpointType::kUrl) {
    return ui::DataTransferEndpoint(
        url::Origin::Create(GURL(kExample2Url)),
        /*notify_if_restricted=*/notify_if_restricted);
  } else if (type) {
    return ui::DataTransferEndpoint(
        *type,
        /*notify_if_restricted=*/notify_if_restricted);
  }
  return absl::nullopt;
}

std::unique_ptr<content::WebContents> CreateTestWebContents(
    content::BrowserContext* browser_context) {
  auto site_instance = content::SiteInstance::Create(browser_context);
  return content::WebContentsTester::CreateTestWebContents(
      browser_context, std::move(site_instance));
}

DlpRulesManager::Component GetComponent(ui::EndpointType endpoint_type) {
  switch (endpoint_type) {
    case ui::EndpointType::kArc:
      return DlpRulesManager::Component::kArc;
    case ui::EndpointType::kCrostini:
      return DlpRulesManager::Component::kCrostini;
    case ui::EndpointType::kPluginVm:
      return DlpRulesManager::Component::kPluginVm;
    default:
      return DlpRulesManager::Component::kUnknownComponent;
  }
}

}  // namespace

class DataTransferDlpControllerTest
    : public ::testing::TestWithParam<
          std::tuple<absl::optional<ui::EndpointType>, bool>> {
 protected:
  DataTransferDlpControllerTest()
      : rules_manager_(), dlp_controller_(rules_manager_) {}

  ~DataTransferDlpControllerTest() override = default;

  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler rvh_test_enabler_;
  ::testing::StrictMock<MockDlpRulesManager> rules_manager_;
  ::testing::StrictMock<MockDlpController> dlp_controller_;
  base::HistogramTester histogram_tester_;
};

TEST_F(DataTransferDlpControllerTest, NullSrc) {
  EXPECT_EQ(true, dlp_controller_.IsClipboardReadAllowed(nullptr, nullptr,
                                                         absl::nullopt));
  EXPECT_EQ(true, dlp_controller_.IsDragDropAllowed(nullptr, nullptr,
                                                    /*is_drop=*/false));
  histogram_tester_.ExpectUniqueSample(
      GetDlpHistogramPrefix() + dlp::kClipboardReadBlockedUMA, false, 1);
  histogram_tester_.ExpectUniqueSample(
      GetDlpHistogramPrefix() + dlp::kDragDropBlockedUMA, false, 1);
}

TEST_F(DataTransferDlpControllerTest, ClipboardHistoryDst) {
  ui::DataTransferEndpoint data_src(url::Origin::Create(GURL(kExample1Url)));
  ui::DataTransferEndpoint data_dst(ui::EndpointType::kClipboardHistory);
  EXPECT_EQ(true, dlp_controller_.IsClipboardReadAllowed(&data_src, &data_dst,
                                                         absl::nullopt));
  histogram_tester_.ExpectUniqueSample(
      GetDlpHistogramPrefix() + dlp::kClipboardReadBlockedUMA, false, 1);
}

TEST_F(DataTransferDlpControllerTest, PasteIfAllowed_Allow) {
  ui::DataTransferEndpoint data_src(url::Origin::Create(GURL(kExample1Url)));
  ui::DataTransferEndpoint data_dst(url::Origin::Create(GURL(kExample2Url)));

  // IsClipboardReadAllowed
  EXPECT_CALL(rules_manager_, IsRestrictedDestination)
      .WillOnce(testing::Return(DlpRulesManager::Level::kAllow));

  ::testing::StrictMock<base::MockOnceCallback<void(bool)>> callback;
  EXPECT_CALL(callback, Run(true));

  std::unique_ptr<TestingProfile> testing_profile =
      TestingProfile::Builder().Build();
  auto web_contents = CreateTestWebContents(testing_profile.get());
  dlp_controller_.PasteIfAllowed(&data_src, &data_dst, absl::nullopt,
                                 web_contents->GetMainFrame(), callback.Get());
}

TEST_F(DataTransferDlpControllerTest, PasteIfAllowed_NullWebContents) {
  ui::DataTransferEndpoint data_src(url::Origin::Create(GURL(kExample1Url)));
  ui::DataTransferEndpoint data_dst(url::Origin::Create(GURL(kExample2Url)));

  ::testing::StrictMock<base::MockOnceCallback<void(bool)>> callback;
  EXPECT_CALL(callback, Run(false));
  dlp_controller_.PasteIfAllowed(&data_src, &data_dst, absl::nullopt, nullptr,
                                 callback.Get());
}

TEST_F(DataTransferDlpControllerTest, PasteIfAllowed_WarnDst) {
  ui::DataTransferEndpoint data_src(url::Origin::Create(GURL(kExample1Url)));
  ui::DataTransferEndpoint data_dst(url::Origin::Create(GURL(kExample2Url)));

  std::unique_ptr<TestingProfile> testing_profile =
      TestingProfile::Builder().Build();
  auto web_contents = CreateTestWebContents(testing_profile.get());

  ::testing::StrictMock<base::MockOnceCallback<void(bool)>> callback;

  // ShouldPasteOnWarn returns false.
  EXPECT_CALL(rules_manager_, IsRestrictedDestination)
      .WillOnce(testing::Return(DlpRulesManager::Level::kWarn));
  EXPECT_CALL(dlp_controller_, ShouldPasteOnWarn)
      .WillRepeatedly(testing::Return(false));
  EXPECT_CALL(dlp_controller_, ShouldCancelOnWarn)
      .WillRepeatedly(testing::Return(false));
  EXPECT_CALL(dlp_controller_, WarnOnBlinkPaste);

  dlp_controller_.PasteIfAllowed(&data_src, &data_dst, absl::nullopt,
                                 web_contents->GetMainFrame(), callback.Get());
}

TEST_F(DataTransferDlpControllerTest, PasteIfAllowed_ProceedDst) {
  ui::DataTransferEndpoint data_src(url::Origin::Create(GURL(kExample1Url)));
  ui::DataTransferEndpoint data_dst(url::Origin::Create(GURL(kExample2Url)));

  std::unique_ptr<TestingProfile> testing_profile =
      TestingProfile::Builder().Build();
  auto web_contents = CreateTestWebContents(testing_profile.get());

  ::testing::StrictMock<base::MockOnceCallback<void(bool)>> callback;

  // ShouldPasteOnWarn returns true.
  EXPECT_CALL(rules_manager_, IsRestrictedDestination)
      .WillOnce(testing::Return(DlpRulesManager::Level::kWarn));
  EXPECT_CALL(dlp_controller_, ShouldPasteOnWarn)
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(dlp_controller_, ShouldCancelOnWarn)
      .WillRepeatedly(testing::Return(false));

  EXPECT_CALL(callback, Run(true));
  dlp_controller_.PasteIfAllowed(&data_src, &data_dst, absl::nullopt,
                                 web_contents->GetMainFrame(), callback.Get());
}

TEST_F(DataTransferDlpControllerTest, PasteIfAllowed_CancelDst) {
  ui::DataTransferEndpoint data_src(url::Origin::Create(GURL(kExample1Url)));
  ui::DataTransferEndpoint data_dst(url::Origin::Create(GURL(kExample2Url)));

  std::unique_ptr<TestingProfile> testing_profile =
      TestingProfile::Builder().Build();
  auto web_contents = CreateTestWebContents(testing_profile.get());

  ::testing::StrictMock<base::MockOnceCallback<void(bool)>> callback;

  // ShouldCancelOnWarn returns true.
  EXPECT_CALL(rules_manager_, IsRestrictedDestination)
      .WillOnce(testing::Return(DlpRulesManager::Level::kWarn));
  EXPECT_CALL(dlp_controller_, ShouldPasteOnWarn)
      .WillRepeatedly(testing::Return(false));
  EXPECT_CALL(dlp_controller_, ShouldCancelOnWarn)
      .WillRepeatedly(testing::Return(true));

  EXPECT_CALL(callback, Run(false));
  dlp_controller_.PasteIfAllowed(&data_src, &data_dst, absl::nullopt,
                                 web_contents->GetMainFrame(), callback.Get());
}

// Create a version of the test class for parameterized testing.
class DlpControllerTest : public DataTransferDlpControllerTest {
 protected:
  void SetUp() override {
    data_src_ =
        ui::DataTransferEndpoint(url::Origin::Create(GURL(kExample1Url)));
    absl::optional<ui::EndpointType> endpoint_type;
    std::tie(endpoint_type, do_notify_) = GetParam();
    data_dst_ =
        CreateEndpoint(base::OptionalOrNullptr(endpoint_type), do_notify_);
    dst_ptr_ = base::OptionalOrNullptr(data_dst_);

    SetReportQueueForReportingManager(&reporting_manager_, events_);
    EXPECT_CALL(rules_manager_, GetReportingManager)
        .WillRepeatedly(::testing::Return(&reporting_manager_));
  }

  ui::DataTransferEndpoint data_src_{ui::EndpointType::kDefault};
  bool do_notify_;
  absl::optional<ui::DataTransferEndpoint> data_dst_;
  ui::DataTransferEndpoint* dst_ptr_;
  DlpReportingManager reporting_manager_;
  std::vector<DlpPolicyEvent> events_;
};

INSTANTIATE_TEST_SUITE_P(
    DlpClipboard,
    DlpControllerTest,
    ::testing::Combine(::testing::Values(absl::nullopt,
                                         ui::EndpointType::kDefault,
                                         ui::EndpointType::kUnknownVm,
                                         ui::EndpointType::kBorealis,
                                         ui::EndpointType::kUrl),
                       testing::Bool()));

TEST_P(DlpControllerTest, Allow) {
  // IsClipboardReadAllowed
  EXPECT_CALL(rules_manager_, IsRestrictedDestination)
      .WillOnce(testing::Return(DlpRulesManager::Level::kAllow));

  EXPECT_EQ(true, dlp_controller_.IsClipboardReadAllowed(&data_src_, dst_ptr_,
                                                         absl::nullopt));
  testing::Mock::VerifyAndClearExpectations(&dlp_controller_);

  // IsDragDropAllowed
  EXPECT_CALL(rules_manager_, IsRestrictedDestination)
      .WillOnce(testing::Return(DlpRulesManager::Level::kAllow));

  EXPECT_EQ(true, dlp_controller_.IsDragDropAllowed(&data_src_, dst_ptr_,
                                                    /*is_drop=*/do_notify_));
  testing::Mock::VerifyAndClearExpectations(&dlp_controller_);
  histogram_tester_.ExpectUniqueSample(
      GetDlpHistogramPrefix() + dlp::kClipboardReadBlockedUMA, false, 1);
  histogram_tester_.ExpectUniqueSample(
      GetDlpHistogramPrefix() + dlp::kDragDropBlockedUMA, false, 1);
}

TEST_P(DlpControllerTest, Block_IsClipboardReadAllowed) {
  EXPECT_CALL(rules_manager_, IsRestrictedDestination)
      .WillOnce(testing::Return(DlpRulesManager::Level::kBlock));
  if (do_notify_ || !dst_ptr_)
    EXPECT_CALL(dlp_controller_, NotifyBlockedPaste);

  EXPECT_EQ(false, dlp_controller_.IsClipboardReadAllowed(&data_src_, dst_ptr_,
                                                          absl::nullopt));
  testing::Mock::VerifyAndClearExpectations(&dlp_controller_);

  if (!data_dst_ || do_notify_) {
    EXPECT_EQ(events_.size(), 1u);
    EXPECT_THAT(events_[0],
                IsDlpPolicyEvent(CreateDlpPolicyEvent(
                    "", "", DlpRulesManager::Restriction::kClipboard,
                    DlpRulesManager::Level::kBlock)));
  } else {
    EXPECT_TRUE(events_.empty());
  }

  histogram_tester_.ExpectUniqueSample(
      GetDlpHistogramPrefix() + dlp::kClipboardReadBlockedUMA, true, 1);
}

TEST_P(DlpControllerTest, Block_IsDragDropAllowed) {
  EXPECT_CALL(rules_manager_, IsRestrictedDestination)
      .WillOnce(testing::Return(DlpRulesManager::Level::kBlock));
  if (do_notify_)
    EXPECT_CALL(dlp_controller_, NotifyBlockedDrop);

  EXPECT_EQ(false, dlp_controller_.IsDragDropAllowed(&data_src_, dst_ptr_,
                                                     /*is_drop=*/do_notify_));
  testing::Mock::VerifyAndClearExpectations(&dlp_controller_);

  if (!data_dst_ || do_notify_) {
    EXPECT_EQ(events_.size(), 1u);
    EXPECT_THAT(events_[0],
                IsDlpPolicyEvent(CreateDlpPolicyEvent(
                    "", "", DlpRulesManager::Restriction::kClipboard,
                    DlpRulesManager::Level::kBlock)));
  } else {
    EXPECT_TRUE(events_.empty());
  }

  histogram_tester_.ExpectUniqueSample(
      GetDlpHistogramPrefix() + dlp::kDragDropBlockedUMA, true, 1);
}

TEST_P(DlpControllerTest, Report_IsClipboardReadAllowed) {
  EXPECT_CALL(rules_manager_, IsRestrictedDestination)
      .WillOnce(testing::Return(DlpRulesManager::Level::kReport));

  EXPECT_EQ(true, dlp_controller_.IsClipboardReadAllowed(&data_src_, dst_ptr_,
                                                         absl::nullopt));
  testing::Mock::VerifyAndClearExpectations(&dlp_controller_);

  if (!data_dst_ || do_notify_) {
    EXPECT_EQ(events_.size(), 1u);
    EXPECT_THAT(events_[0],
                IsDlpPolicyEvent(CreateDlpPolicyEvent(
                    "", "", DlpRulesManager::Restriction::kClipboard,
                    DlpRulesManager::Level::kReport)));
  } else {
    EXPECT_TRUE(events_.empty());
  }
}

TEST_P(DlpControllerTest, Report_IsDragDropAllowed) {
  EXPECT_CALL(rules_manager_, IsRestrictedDestination)
      .WillOnce(testing::Return(DlpRulesManager::Level::kReport));

  EXPECT_EQ(true, dlp_controller_.IsDragDropAllowed(&data_src_, dst_ptr_,
                                                    /*is_drop=*/do_notify_));
  testing::Mock::VerifyAndClearExpectations(&dlp_controller_);

  if (!data_dst_ || do_notify_) {
    EXPECT_EQ(events_.size(), 1u);
    EXPECT_THAT(events_[0],
                IsDlpPolicyEvent(CreateDlpPolicyEvent(
                    "", "", DlpRulesManager::Restriction::kClipboard,
                    DlpRulesManager::Level::kReport)));
  } else {
    EXPECT_TRUE(events_.empty());
  }
}

TEST_P(DlpControllerTest, Warn) {
  // ShouldPasteOnWarn returns false.
  EXPECT_CALL(rules_manager_, IsRestrictedDestination)
      .WillOnce(testing::Return(DlpRulesManager::Level::kWarn));
  EXPECT_CALL(dlp_controller_, ShouldPasteOnWarn)
      .WillRepeatedly(testing::Return(false));
  EXPECT_CALL(dlp_controller_, ShouldCancelOnWarn)
      .WillRepeatedly(testing::Return(false));
  bool show_warning = dst_ptr_ ? (do_notify_ && !dst_ptr_->IsUrlType()) : true;
  if (show_warning)
    EXPECT_CALL(dlp_controller_, WarnOnPaste);

  EXPECT_EQ(!show_warning, dlp_controller_.IsClipboardReadAllowed(
                               &data_src_, dst_ptr_, absl::nullopt));
  testing::Mock::VerifyAndClearExpectations(&dlp_controller_);

  // ShouldPasteOnWarn returns true.
  EXPECT_CALL(rules_manager_, IsRestrictedDestination)
      .WillOnce(testing::Return(DlpRulesManager::Level::kWarn));
  EXPECT_CALL(dlp_controller_, ShouldPasteOnWarn)
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(dlp_controller_, ShouldCancelOnWarn)
      .WillRepeatedly(testing::Return(false));
  EXPECT_EQ(true, dlp_controller_.IsClipboardReadAllowed(&data_src_, dst_ptr_,
                                                         absl::nullopt));
  testing::Mock::VerifyAndClearExpectations(&dlp_controller_);
  histogram_tester_.ExpectBucketCount(
      GetDlpHistogramPrefix() + dlp::kClipboardReadBlockedUMA, false,
      show_warning ? 1 : 2);
  histogram_tester_.ExpectBucketCount(
      GetDlpHistogramPrefix() + dlp::kClipboardReadBlockedUMA, true,
      show_warning ? 1 : 0);
}

TEST_P(DlpControllerTest, Warn_ShouldCancelOnWarn) {
  // ShouldCancelOnWarn returns true.
  EXPECT_CALL(rules_manager_, IsRestrictedDestination)
      .WillOnce(testing::Return(DlpRulesManager::Level::kWarn));
  EXPECT_CALL(dlp_controller_, ShouldCancelOnWarn)
      .WillRepeatedly(testing::Return(true));

  bool expected_is_read = data_dst_.has_value() ? !do_notify_ : false;
  EXPECT_EQ(expected_is_read, dlp_controller_.IsClipboardReadAllowed(
                                  &data_src_, dst_ptr_, absl::nullopt));
  testing::Mock::VerifyAndClearExpectations(&dlp_controller_);
}

// Create a version of the test class for parameterized testing.
class DlpControllerVMsTest : public DataTransferDlpControllerTest {
 protected:
  void SetUp() override {
    data_src_ =
        ui::DataTransferEndpoint(url::Origin::Create(GURL(kExample1Url)));
    std::tie(endpoint_type_, do_notify_) = GetParam();
    ASSERT_TRUE(endpoint_type_.has_value());
    data_dst_ = ui::DataTransferEndpoint(endpoint_type_.value(), do_notify_);

    SetReportQueueForReportingManager(&reporting_manager_, events_);
    EXPECT_CALL(rules_manager_, GetReportingManager)
        .WillRepeatedly(::testing::Return(&reporting_manager_));
  }

  ui::DataTransferEndpoint data_src_{ui::EndpointType::kDefault};
  absl::optional<ui::EndpointType> endpoint_type_;
  bool do_notify_;
  ui::DataTransferEndpoint data_dst_{ui::EndpointType::kDefault};
  DlpReportingManager reporting_manager_;
  std::vector<DlpPolicyEvent> events_;
};

INSTANTIATE_TEST_SUITE_P(
    DlpClipboard,
    DlpControllerVMsTest,
    ::testing::Combine(::testing::Values(ui::EndpointType::kArc,
                                         ui::EndpointType::kCrostini,
                                         ui::EndpointType::kPluginVm),
                       testing::Bool()));

TEST_P(DlpControllerVMsTest, Allow) {
  ui::DataTransferEndpoint data_src(url::Origin::Create(GURL(kExample1Url)));
  absl::optional<ui::EndpointType> endpoint_type;
  bool do_notify;
  std::tie(endpoint_type, do_notify) = GetParam();
  ASSERT_TRUE(endpoint_type.has_value());
  ui::DataTransferEndpoint data_dst(endpoint_type.value(), do_notify);

  // IsClipboardReadAllowed
  EXPECT_CALL(rules_manager_, IsRestrictedComponent)
      .WillOnce(testing::Return(DlpRulesManager::Level::kAllow));

  EXPECT_EQ(true, dlp_controller_.IsClipboardReadAllowed(&data_src, &data_dst,
                                                         absl::nullopt));
  testing::Mock::VerifyAndClearExpectations(&dlp_controller_);

  // IsDragDropAllowed
  EXPECT_CALL(rules_manager_, IsRestrictedComponent)
      .WillOnce(testing::Return(DlpRulesManager::Level::kAllow));

  EXPECT_EQ(true, dlp_controller_.IsDragDropAllowed(&data_src, &data_dst,
                                                    /*is_drop=*/do_notify));
  testing::Mock::VerifyAndClearExpectations(&dlp_controller_);
  histogram_tester_.ExpectUniqueSample(
      GetDlpHistogramPrefix() + dlp::kClipboardReadBlockedUMA, false, 1);
  histogram_tester_.ExpectUniqueSample(
      GetDlpHistogramPrefix() + dlp::kDragDropBlockedUMA, false, 1);
}

TEST_P(DlpControllerVMsTest, Block_IsClipboardReadAllowed) {
  EXPECT_CALL(rules_manager_, IsRestrictedComponent)
      .WillOnce(testing::Return(DlpRulesManager::Level::kBlock));
  if (do_notify_)
    EXPECT_CALL(dlp_controller_, NotifyBlockedPaste);

  EXPECT_EQ(false, dlp_controller_.IsClipboardReadAllowed(
                       &data_src_, &data_dst_, absl::nullopt));
  testing::Mock::VerifyAndClearExpectations(&dlp_controller_);

  if (do_notify_) {
    EXPECT_EQ(events_.size(), 1u);
    EXPECT_THAT(events_[0], IsDlpPolicyEvent(CreateDlpPolicyEvent(
                                "", GetComponent(endpoint_type_.value()),
                                DlpRulesManager::Restriction::kClipboard,
                                DlpRulesManager::Level::kBlock)));
  } else {
    EXPECT_TRUE(events_.empty());
  }

  histogram_tester_.ExpectUniqueSample(
      GetDlpHistogramPrefix() + dlp::kClipboardReadBlockedUMA, true, 1);
}

TEST_P(DlpControllerVMsTest, Block_IsDragDropAllowed) {
  EXPECT_CALL(rules_manager_, IsRestrictedComponent)
      .WillOnce(testing::Return(DlpRulesManager::Level::kBlock));
  if (do_notify_)
    EXPECT_CALL(dlp_controller_, NotifyBlockedDrop);

  EXPECT_EQ(false, dlp_controller_.IsDragDropAllowed(&data_src_, &data_dst_,
                                                     /*is_drop=*/do_notify_));
  testing::Mock::VerifyAndClearExpectations(&dlp_controller_);

  if (do_notify_) {
    EXPECT_EQ(events_.size(), 1u);
    EXPECT_THAT(events_[0], IsDlpPolicyEvent(CreateDlpPolicyEvent(
                                "", GetComponent(endpoint_type_.value()),
                                DlpRulesManager::Restriction::kClipboard,
                                DlpRulesManager::Level::kBlock)));
  } else {
    EXPECT_TRUE(events_.empty());
  }

  histogram_tester_.ExpectUniqueSample(
      GetDlpHistogramPrefix() + dlp::kDragDropBlockedUMA, true, 1);
}

TEST_P(DlpControllerVMsTest, Report_IsClipboardReadAllowed) {
  EXPECT_CALL(rules_manager_, IsRestrictedComponent)
      .WillOnce(testing::Return(DlpRulesManager::Level::kReport));

  EXPECT_EQ(true, dlp_controller_.IsClipboardReadAllowed(&data_src_, &data_dst_,
                                                         absl::nullopt));
  testing::Mock::VerifyAndClearExpectations(&dlp_controller_);

  if (do_notify_) {
    EXPECT_EQ(events_.size(), 1u);
    EXPECT_THAT(events_[0], IsDlpPolicyEvent(CreateDlpPolicyEvent(
                                "", GetComponent(endpoint_type_.value()),
                                DlpRulesManager::Restriction::kClipboard,
                                DlpRulesManager::Level::kReport)));
  } else {
    EXPECT_TRUE(events_.empty());
  }
}

TEST_P(DlpControllerVMsTest, Report_IsDragDropAllowed) {
  EXPECT_CALL(rules_manager_, IsRestrictedComponent)
      .WillOnce(testing::Return(DlpRulesManager::Level::kReport));

  EXPECT_EQ(true, dlp_controller_.IsDragDropAllowed(&data_src_, &data_dst_,
                                                    /*is_drop=*/do_notify_));
  testing::Mock::VerifyAndClearExpectations(&dlp_controller_);

  if (do_notify_) {
    EXPECT_EQ(events_.size(), 1u);
    EXPECT_THAT(events_[0], IsDlpPolicyEvent(CreateDlpPolicyEvent(
                                "", GetComponent(endpoint_type_.value()),
                                DlpRulesManager::Restriction::kClipboard,
                                DlpRulesManager::Level::kReport)));
  } else {
    EXPECT_TRUE(events_.empty());
  }
}

TEST_P(DlpControllerVMsTest, Warn) {
  ui::DataTransferEndpoint data_src(url::Origin::Create(GURL(kExample1Url)));
  absl::optional<ui::EndpointType> endpoint_type;
  bool do_notify;
  std::tie(endpoint_type, do_notify) = GetParam();
  ASSERT_TRUE(endpoint_type.has_value());
  ui::DataTransferEndpoint data_dst(endpoint_type.value(), do_notify);

  // IsClipboardReadAllowed
  EXPECT_CALL(rules_manager_, IsRestrictedComponent)
      .WillOnce(testing::Return(DlpRulesManager::Level::kWarn));
  if (do_notify)
    EXPECT_CALL(dlp_controller_, WarnOnPaste);

  EXPECT_EQ(true, dlp_controller_.IsClipboardReadAllowed(&data_src, &data_dst,
                                                         absl::nullopt));
  testing::Mock::VerifyAndClearExpectations(&dlp_controller_);
  histogram_tester_.ExpectUniqueSample(
      GetDlpHistogramPrefix() + dlp::kClipboardReadBlockedUMA, false, 1);
}

}  // namespace policy
