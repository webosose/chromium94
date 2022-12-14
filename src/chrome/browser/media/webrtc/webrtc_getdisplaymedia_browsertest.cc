// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "build/chromeos_buildflags.h"
#include "chrome/browser/media/webrtc/webrtc_browsertest_base.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "media/base/media_switches.h"
#include "third_party/blink/public/common/features.h"

#if defined(OS_MAC)
#include "base/mac/mac_util.h"
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
#include "chrome/browser/ash/policy/dlp/dlp_content_manager_test_helper.h"
#include "chrome/browser/ash/policy/dlp/dlp_content_restriction_set.h"
#endif

namespace {

static const char kMainHtmlPage[] = "/webrtc/webrtc_getdisplaymedia_test.html";

enum class GetDisplayMediaVariant : int {
  kStandard = 0,
  kPreferCurrentTab = 1
};

struct TestConfigForPicker {
  bool should_prefer_current_tab_;
  // |accept_this_tab_capture| is only applicable if
  // |should_prefer_current_tab_| is set to true. Then, setting
  // |accept_this_tab_capture| to true accepts the current tab, and
  // |accept_this_tab_capture| set to false implies dismissing the media picker.
  bool accept_this_tab_capture;
};

struct TestConfigForFakeUI {
  bool should_prefer_current_tab_;
  const char* display_surface;
};

}  // namespace

// Base class for top level tests for getDisplayMedia().
class WebRtcScreenCaptureBrowserTest : public WebRtcTestBase {
 public:
  ~WebRtcScreenCaptureBrowserTest() override = default;

  void SetUpInProcessBrowserTestFixture() override {
    DetectErrorsInJavaScript();
  }

  void RunGetDisplayMedia(content::WebContents* tab,
                          const std::string& constraints,
                          bool is_fake_ui,
                          bool expect_success) {
    std::string result;
    EXPECT_TRUE(content::ExecuteScriptAndExtractString(
        tab->GetMainFrame(),
        base::StringPrintf("runGetDisplayMedia(%s, \"top-level-document\");",
                           constraints.c_str()),
        &result));

#if defined(OS_MAC)
    // Starting from macOS 10.15, screen capture requires system permissions
    // that are disabled by default. The permission is reported as granted
    // if the fake UI is used.
    EXPECT_EQ(result, base::mac::IsAtMostOS10_14() || is_fake_ui
                          ? "capture-success"
                          : "capture-failure");
#else
    EXPECT_EQ(result, expect_success ? "capture-success" : "capture-failure");
#endif
  }

  virtual bool PreferCurrentTab() const = 0;

  std::string GetConstraints(bool video, bool audio) const {
    return base::StringPrintf(
        "{video:%s, audio: %s, preferCurrentTab: %s}", video ? "true" : "false",
        audio ? "true" : "false", PreferCurrentTab() ? "true" : "false");
  }
};

// Top level test for getDisplayMedia().
// Pops picker UI and shares by default.
class WebRtcScreenCaptureBrowserTestWithPicker
    : public WebRtcScreenCaptureBrowserTest,
      public testing::WithParamInterface<TestConfigForPicker> {
 public:
  WebRtcScreenCaptureBrowserTestWithPicker() : test_config_(GetParam()) {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(
        switches::kEnableExperimentalWebPlatformFeatures);
    if (test_config_.should_prefer_current_tab_) {
      command_line->AppendSwitch(test_config_.accept_this_tab_capture
                                     ? switches::kThisTabCaptureAutoAccept
                                     : switches::kThisTabCaptureAutoReject);
    } else {
#if BUILDFLAG(IS_CHROMEOS_ASH)
      command_line->AppendSwitchASCII(switches::kAutoSelectDesktopCaptureSource,
                                      "Display");
#else
      command_line->AppendSwitchASCII(switches::kAutoSelectDesktopCaptureSource,
                                      "Entire screen");
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)
    }
  }

  bool PreferCurrentTab() const override {
    return test_config_.should_prefer_current_tab_;
  }

  const TestConfigForPicker test_config_;
};

// TODO(1170479): Real desktop capture is flaky on below platforms.
#if defined(OS_WIN) || defined(OS_MAC)
#define MAYBE_ScreenCaptureVideo DISABLED_ScreenCaptureVideo
#else
#define MAYBE_ScreenCaptureVideo ScreenCaptureVideo
#endif  // defined(OS_WIN)
IN_PROC_BROWSER_TEST_P(WebRtcScreenCaptureBrowserTestWithPicker,
                       MAYBE_ScreenCaptureVideo) {
  ASSERT_TRUE(embedded_test_server()->Start());

  content::WebContents* tab = OpenTestPageInNewTab(kMainHtmlPage);
  RunGetDisplayMedia(tab, GetConstraints(/*video=*/true, /*audio=*/false),
                     /*is_fake_ui=*/false,
                     test_config_.accept_this_tab_capture);
}

#if BUILDFLAG(IS_CHROMEOS_ASH)
IN_PROC_BROWSER_TEST_P(WebRtcScreenCaptureBrowserTestWithPicker,
                       ScreenCaptureVideoWithDlp) {
  ASSERT_TRUE(embedded_test_server()->Start());

  policy::DlpContentManagerTestHelper helper;
  content::WebContents* tab = OpenTestPageInNewTab(kMainHtmlPage);
  RunGetDisplayMedia(tab, GetConstraints(/*video=*/true, /*audio=*/false),
                     /*is_fake_ui=*/false,
                     test_config_.accept_this_tab_capture);

  if (!test_config_.accept_this_tab_capture) {
    // This test is not relevant for this parameterized test case because it
    // does not capture the tab/display surface.
    return;
  }

  std::string result;
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      tab->GetMainFrame(), "waitVideoUnmuted();", &result));
  EXPECT_EQ(result, "unmuted");

  const policy::DlpContentRestrictionSet kScreenShareRestricted(
      policy::DlpContentRestriction::kScreenShare,
      policy::DlpRulesManager::Level::kBlock);

  helper.ChangeConfidentiality(tab, kScreenShareRestricted);
  content::WaitForLoadStop(tab);

  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      tab->GetMainFrame(), "waitVideoMuted();", &result));
  EXPECT_EQ(result, "muted");

  const policy::DlpContentRestrictionSet kEmptyRestrictionSet;
  helper.ChangeConfidentiality(tab, kEmptyRestrictionSet);

  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      tab->GetMainFrame(), "waitVideoUnmuted();", &result));
  EXPECT_EQ(result, "unmuted");
}
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// TODO(1170479): Real desktop capture is flaky on below platforms.
#if defined(OS_WIN) || defined(OS_MAC)
#define MAYBE_ScreenCaptureVideoAndAudio DISABLED_ScreenCaptureVideoAndAudio
// On linux debug bots, it's flaky as well.
#elif ((defined(OS_LINUX) || BUILDFLAG(IS_CHROMEOS_LACROS)) && !defined(NDEBUG))
#define MAYBE_ScreenCaptureVideoAndAudio DISABLED_ScreenCaptureVideoAndAudio
// On linux asan bots, it's flaky as well - msan and other rel bot are fine.
#elif ((defined(OS_LINUX) || BUILDFLAG(IS_CHROMEOS_LACROS)) && \
       defined(ADDRESS_SANITIZER))
#define MAYBE_ScreenCaptureVideoAndAudio DISABLED_ScreenCaptureVideoAndAudio
#else
#define MAYBE_ScreenCaptureVideoAndAudio ScreenCaptureVideoAndAudio
#endif  // defined(OS_WIN)
IN_PROC_BROWSER_TEST_P(WebRtcScreenCaptureBrowserTestWithPicker,
                       MAYBE_ScreenCaptureVideoAndAudio) {
  ASSERT_TRUE(embedded_test_server()->Start());

  content::WebContents* tab = OpenTestPageInNewTab(kMainHtmlPage);
  RunGetDisplayMedia(tab, GetConstraints(/*video=*/true, /*audio=*/true),
                     /*is_fake_ui=*/false,
                     test_config_.accept_this_tab_capture);
}

INSTANTIATE_TEST_SUITE_P(
    All,
    WebRtcScreenCaptureBrowserTestWithPicker,
    testing::Values(TestConfigForPicker{/*should_prefer_current_tab_=*/false,
                                        /*accept_this_tab_capture=*/true},
                    TestConfigForPicker{/*should_prefer_current_tab_=*/true,
                                        /*accept_this_tab_capture=*/true},
                    TestConfigForPicker{/*should_prefer_current_tab_=*/true,
                                        /*accept_this_tab_capture=*/false}));

// Top level test for getDisplayMedia().
// Skips picker UI and uses fake device with specified type.
class WebRtcScreenCaptureBrowserTestWithFakeUI
    : public WebRtcScreenCaptureBrowserTest,
      public testing::WithParamInterface<TestConfigForFakeUI> {
 public:
  WebRtcScreenCaptureBrowserTestWithFakeUI() : test_config_(GetParam()) {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(
        switches::kEnableExperimentalWebPlatformFeatures);
    command_line->AppendSwitch(switches::kUseFakeUIForMediaStream);
    command_line->RemoveSwitch(switches::kUseFakeDeviceForMediaStream);
    command_line->AppendSwitchASCII(
        switches::kUseFakeDeviceForMediaStream,
        base::StringPrintf("display-media-type=%s",
                           test_config_.display_surface));
  }

  bool PreferCurrentTab() const override {
    return test_config_.should_prefer_current_tab_;
  }

 protected:
  const TestConfigForFakeUI test_config_;
};

IN_PROC_BROWSER_TEST_P(WebRtcScreenCaptureBrowserTestWithFakeUI,
                       ScreenCaptureVideo) {
  ASSERT_TRUE(embedded_test_server()->Start());

  content::WebContents* tab = OpenTestPageInNewTab(kMainHtmlPage);
  RunGetDisplayMedia(tab, GetConstraints(/*video=*/true, /*audio=*/false),
                     /*is_fake_ui=*/true, /*expect_success=*/true);

  std::string result;
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      tab->GetMainFrame(), "getDisplaySurfaceSetting();", &result));
  EXPECT_EQ(result, test_config_.display_surface);

  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      tab->GetMainFrame(), "getLogicalSurfaceSetting();", &result));
  EXPECT_EQ(result, "true");

  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      tab->GetMainFrame(), "getCursorSetting();", &result));
  EXPECT_EQ(result, "never");
}

IN_PROC_BROWSER_TEST_P(WebRtcScreenCaptureBrowserTestWithFakeUI,
                       ScreenCaptureVideoAndAudio) {
  ASSERT_TRUE(embedded_test_server()->Start());

  content::WebContents* tab = OpenTestPageInNewTab(kMainHtmlPage);
  RunGetDisplayMedia(tab, GetConstraints(/*video=*/true, /*audio=*/true),
                     /*is_fake_ui=*/true, /*expect_success=*/true);

  std::string result;
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      tab->GetMainFrame(), "hasAudioTrack();", &result));
  EXPECT_EQ(result, "true");
}

IN_PROC_BROWSER_TEST_P(WebRtcScreenCaptureBrowserTestWithFakeUI,
                       ScreenCaptureWithConstraints) {
  ASSERT_TRUE(embedded_test_server()->Start());

  content::WebContents* tab = OpenTestPageInNewTab(kMainHtmlPage);
  const int kMaxWidth = 200;
  const int kMaxFrameRate = 6;
  const std::string constraints = base::StringPrintf(
      "{video: {width: {max: %d}, frameRate: {max: %d}}, "
      "should_prefer_current_tab_: "
      "%s}",
      kMaxWidth, kMaxFrameRate,
      test_config_.should_prefer_current_tab_ ? "true" : "false");
  RunGetDisplayMedia(tab, constraints,
                     /*is_fake_ui=*/true, /*expect_success=*/true);

  std::string result;
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      tab->GetMainFrame(), "getWidthSetting();", &result));
  EXPECT_EQ(result, base::StringPrintf("%d", kMaxWidth));

  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      tab->GetMainFrame(), "getFrameRateSetting();", &result));
  EXPECT_EQ(result, base::StringPrintf("%d", kMaxFrameRate));
}

INSTANTIATE_TEST_SUITE_P(
    All,
    WebRtcScreenCaptureBrowserTestWithFakeUI,
    testing::Values(TestConfigForFakeUI{/*should_prefer_current_tab_=*/false,
                                        /*display_surface=*/"monitor"},
                    TestConfigForFakeUI{/*should_prefer_current_tab_=*/false,
                                        /*display_surface=*/"window"},
                    TestConfigForFakeUI{/*should_prefer_current_tab_=*/false,
                                        /*display_surface=*/"browser"},
                    TestConfigForFakeUI{/*should_prefer_current_tab_=*/true,
                                        /*display_surface=*/"browser"}));

// TODO(crbug.com/1215089): Enable this test suite on Lacros.
#if !BUILDFLAG(IS_CHROMEOS_LACROS)
class WebRtcScreenCapturePermissionPolicyBrowserTest
    : public WebRtcScreenCaptureBrowserTest,
      public testing::WithParamInterface<
          std::pair<GetDisplayMediaVariant, bool>> {
 public:
  WebRtcScreenCapturePermissionPolicyBrowserTest()
      : tested_variant_(GetParam().first),
        allowlisted_by_policy_(GetParam().second) {}

  ~WebRtcScreenCapturePermissionPolicyBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(
        switches::kEnableExperimentalWebPlatformFeatures);
    command_line->AppendSwitchASCII(
        switches::kAutoSelectTabCaptureSourceByTitle, "WebRTC Automated Test");
  }

  bool PreferCurrentTab() const override {
    return tested_variant_ == GetDisplayMediaVariant::kPreferCurrentTab;
  }

 protected:
  const GetDisplayMediaVariant tested_variant_;
  const bool allowlisted_by_policy_;

 private:
};

INSTANTIATE_TEST_SUITE_P(
    All,
    WebRtcScreenCapturePermissionPolicyBrowserTest,
    testing::Values(std::make_pair(GetDisplayMediaVariant::kStandard,
                                   /*allowlisted_by_policy=*/false),
                    std::make_pair(GetDisplayMediaVariant::kStandard,
                                   /*allowlisted_by_policy=*/true),
                    std::make_pair(GetDisplayMediaVariant::kPreferCurrentTab,
                                   /*allowlisted_by_policy=*/false),
                    std::make_pair(GetDisplayMediaVariant::kPreferCurrentTab,
                                   /*allowlisted_by_policy=*/true)));

IN_PROC_BROWSER_TEST_P(WebRtcScreenCapturePermissionPolicyBrowserTest,
                       ScreenShareFromEmbedded) {
  ASSERT_TRUE(embedded_test_server()->Start());

  const std::string constraints =
      base::StringPrintf("{video: true, preferCurrentTab: %s}",
                         PreferCurrentTab() ? "true" : "false");

  std::string result;
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      OpenTestPageInNewTab(kMainHtmlPage)->GetMainFrame(),
      base::StringPrintf(
          "runGetDisplayMedia(%s, \"%s\");", constraints.c_str(),
          allowlisted_by_policy_ ? "allowedFrame" : "disallowedFrame"),
      &result));
  EXPECT_EQ(result, allowlisted_by_policy_ ? "embedded-capture-success"
                                           : "embedded-capture-failure");
}
#endif
