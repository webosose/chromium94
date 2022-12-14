// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/path_service.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/optimization_guide/browser_test_util.h"
#include "chrome/browser/optimization_guide/optimization_guide_keyed_service.h"
#include "chrome/browser/optimization_guide/optimization_guide_keyed_service_factory.h"
#include "chrome/browser/optimization_guide/page_content_annotations_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/history/core/browser/history_database.h"
#include "components/history/core/browser/history_db_task.h"
#include "components/history/core/browser/history_service.h"
#include "components/optimization_guide/content/browser/page_content_annotations_service.h"
#include "components/optimization_guide/core/optimization_guide_enums.h"
#include "components/optimization_guide/core/optimization_guide_features.h"
#include "components/optimization_guide/core/optimization_guide_test_util.h"
#include "components/optimization_guide/core/test_model_info_builder.h"
#include "components/optimization_guide/machine_learning_tflite_buildflags.h"
#include "components/optimization_guide/proto/page_topics_model_metadata.pb.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"

namespace optimization_guide {

#if BUILDFLAG(BUILD_WITH_TFLITE_LIB)
// A HistoryDBTask that retrieves content annotations.
class GetContentAnnotationsTask : public history::HistoryDBTask {
 public:
  GetContentAnnotationsTask(
      const GURL& url,
      base::OnceCallback<void(
          const absl::optional<history::VisitContentAnnotations>&)> callback)
      : url_(url), callback_(std::move(callback)) {}
  ~GetContentAnnotationsTask() override = default;

  // history::HistoryDBTask:
  bool RunOnDBThread(history::HistoryBackend* backend,
                     history::HistoryDatabase* db) override {
    // Get visits for URL.
    const history::URLID url_id = db->GetRowForURL(url_, nullptr);
    history::VisitVector visits;
    if (!db->GetVisitsForURL(url_id, &visits))
      return true;

    // No visits for URL.
    if (visits.empty())
      return true;

    history::VisitContentAnnotations annotations;
    if (db->GetContentAnnotationsForVisit(visits.at(0).visit_id, &annotations))
      stored_content_annotations_ = annotations;

    return true;
  }
  void DoneRunOnMainThread() override {
    std::move(callback_).Run(stored_content_annotations_);
  }

 private:
  // The URL to get content annotations for.
  const GURL url_;
  // The callback to invoke when the database call has completed.
  base::OnceCallback<void(
      const absl::optional<history::VisitContentAnnotations>&)>
      callback_;
  // The content annotations that were stored for |url_|.
  absl::optional<history::VisitContentAnnotations> stored_content_annotations_;
};

class PageContentAnnotationsServiceDisabledBrowserTest
    : public InProcessBrowserTest {
 public:
  PageContentAnnotationsServiceDisabledBrowserTest() {
    scoped_feature_list_.InitWithFeatures(
        /*enabled_features=*/{},
        {features::kOptimizationHints, features::kPageContentAnnotations});
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};
#endif  // BUILDFLAG(BUILD_WITH_TFLITE_LIB)

IN_PROC_BROWSER_TEST_F(PageContentAnnotationsServiceDisabledBrowserTest,
                       KeyedServiceEnabledButFeaturesDisabled) {
  EXPECT_EQ(nullptr, PageContentAnnotationsServiceFactory::GetForProfile(
                         browser()->profile()));
}

class PageContentAnnotationsServiceBrowserTest : public InProcessBrowserTest {
 public:
  PageContentAnnotationsServiceBrowserTest() {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{features::kOptimizationHints, {}},
         {features::kPageContentAnnotations,
          {
              {"write_to_history_service", "true"},
          }}},
        /*disabled_features=*/{features::kLoadModelFileForEachExecution});
  }
  ~PageContentAnnotationsServiceBrowserTest() override = default;

  void set_model_is_lazily_loaded(bool model_is_lazily_loaded) {
    model_is_lazily_loaded_ = model_is_lazily_loaded;
  }

  void set_load_model_on_startup(bool load_model_on_startup) {
    load_model_on_startup_ = load_model_on_startup;
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    InProcessBrowserTest::SetUpOnMainThread();

    embedded_test_server()->ServeFilesFromSourceDirectory(
        "chrome/test/data/optimization_guide");
    ASSERT_TRUE(embedded_test_server()->Start());

    if (load_model_on_startup_) {
      LoadAndWaitForModel();
    }
  }

  void LoadAndWaitForModel() {
    base::HistogramTester histogram_tester;

    proto::Any any_metadata;
    any_metadata.set_type_url(
        "type.googleapis.com/com.foo.PageTopicsModelMetadata");
    proto::PageTopicsModelMetadata page_topics_model_metadata;
    page_topics_model_metadata.set_version(123);
    page_topics_model_metadata.add_supported_output(
        proto::PAGE_TOPICS_SUPPORTED_OUTPUT_CATEGORIES);
    auto* output_params =
        page_topics_model_metadata.mutable_output_postprocessing_params();
    auto* category_params = output_params->mutable_category_params();
    category_params->set_max_categories(5);
    category_params->set_min_none_weight(0.8);
    category_params->set_min_category_weight(0.0);
    category_params->set_min_normalized_weight_within_top_n(0.1);
    output_params->mutable_floc_protected_params()->set_category_name(
        "FLOC_PROTECTED");
    page_topics_model_metadata.SerializeToString(any_metadata.mutable_value());
    base::FilePath source_root_dir;
    base::PathService::Get(base::DIR_SOURCE_ROOT, &source_root_dir);
    base::FilePath model_file_path =
        source_root_dir.AppendASCII("components")
            .AppendASCII("test")
            .AppendASCII("data")
            .AppendASCII("optimization_guide")
            .AppendASCII("bert_page_topics_model.tflite");
    OptimizationGuideKeyedServiceFactory::GetForProfile(browser()->profile())
        ->OverrideTargetModelForTesting(
            proto::OPTIMIZATION_TARGET_PAGE_TOPICS,
            optimization_guide::TestModelInfoBuilder()
                .SetModelFilePath(model_file_path)
                .SetModelMetadata(any_metadata)
                .Build());

#if BUILDFLAG(BUILD_WITH_TFLITE_LIB)
    bool expect_model_loaded = !model_is_lazily_loaded_;
#else
    bool expect_model_loaded = false;
#endif

    if (expect_model_loaded) {
      RetryForHistogramUntilCountReached(
          &histogram_tester,
          "OptimizationGuide.ModelExecutor.ModelLoadingResult.PageTopics", 1);
      histogram_tester.ExpectUniqueSample(
          "OptimizationGuide.ModelExecutor.ModelLoadingResult.PageTopics",
          ModelExecutorLoadingState::kModelFileValidAndMemoryMapped, 1);
      histogram_tester.ExpectTotalCount(
          "OptimizationGuide.ModelExecutor.ModelLoadingDuration.PageTopics", 1);
    } else {
      base::RunLoop().RunUntilIdle();
    }
  }

#if BUILDFLAG(BUILD_WITH_TFLITE_LIB)
  absl::optional<history::VisitContentAnnotations> GetContentAnnotationsForURL(
      const GURL& url) {
    history::HistoryService* history_service =
        HistoryServiceFactory::GetForProfile(
            browser()->profile(), ServiceAccessType::IMPLICIT_ACCESS);
    if (!history_service)
      return absl::nullopt;

    std::unique_ptr<base::RunLoop> run_loop = std::make_unique<base::RunLoop>();
    absl::optional<history::VisitContentAnnotations> got_content_annotations;

    base::CancelableTaskTracker task_tracker;
    history_service->ScheduleDBTask(
        FROM_HERE,
        std::make_unique<GetContentAnnotationsTask>(
            url, base::BindOnce(
                     [](base::RunLoop* run_loop,
                        absl::optional<history::VisitContentAnnotations>*
                            out_content_annotations,
                        const absl::optional<history::VisitContentAnnotations>&
                            content_annotations) {
                       *out_content_annotations = content_annotations;
                       run_loop->Quit();
                     },
                     run_loop.get(), &got_content_annotations)),
        &task_tracker);

    run_loop->Run();
    return got_content_annotations;
  }
#endif  // BUILDFLAG(BUILD_WITH_TFLITE_LIB)

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  bool model_is_lazily_loaded_ = false;
  bool load_model_on_startup_ = true;
};

IN_PROC_BROWSER_TEST_F(PageContentAnnotationsServiceBrowserTest,
                       ModelExecutes) {
  base::HistogramTester histogram_tester;

  GURL url(embedded_test_server()->GetURL("a.com", "/hello.html"));
  ui_test_utils::NavigateToURL(browser(), url);

#if BUILDFLAG(BUILD_WITH_TFLITE_LIB)
  int expected_count = 1;
#else
  int expected_count = 0;
#endif
  RetryForHistogramUntilCountReached(
      &histogram_tester,
      "OptimizationGuide.PageContentAnnotationsService.ContentAnnotated",
      expected_count);

  histogram_tester.ExpectTotalCount(
      "OptimizationGuide.PageContentAnnotationsService.ContentAnnotated",
      expected_count);
#if BUILDFLAG(BUILD_WITH_TFLITE_LIB)
  histogram_tester.ExpectUniqueSample(
      "OptimizationGuide.PageContentAnnotationsService.ContentAnnotated", true,
      1);
#endif

  PageContentAnnotationsService* service =
      PageContentAnnotationsServiceFactory::GetForProfile(browser()->profile());

#if BUILDFLAG(BUILD_WITH_TFLITE_LIB)
  absl::optional<int64_t> model_version = service->GetPageTopicsModelVersion();
  EXPECT_TRUE(model_version.has_value());
  EXPECT_EQ(123, *model_version);
#else
  EXPECT_FALSE(service->GetPageTopicsModelVersion().has_value());
#endif

#if BUILDFLAG(BUILD_WITH_TFLITE_LIB)
  RetryForHistogramUntilCountReached(
      &histogram_tester,
      "OptimizationGuide.PageContentAnnotationsService."
      "ContentAnnotationsStorageStatus",
      1);

  histogram_tester.ExpectUniqueSample(
      "OptimizationGuide.PageContentAnnotationsService."
      "ContentAnnotationsStorageStatus",
      PageContentAnnotationsStorageStatus::kSuccess, 1);

  absl::optional<history::VisitContentAnnotations> got_content_annotations =
      GetContentAnnotationsForURL(url);
  ASSERT_TRUE(got_content_annotations.has_value());
  EXPECT_NE(-1.0,
            got_content_annotations->model_annotations.floc_protected_score);
  EXPECT_FALSE(got_content_annotations->model_annotations.categories.empty());
  EXPECT_EQ(
      123,
      got_content_annotations->model_annotations.page_topics_model_version);
#endif  // BUILDFLAG(BUILD_WITH_TFLITE_LIB)
}

#if BUILDFLAG(BUILD_WITH_TFLITE_LIB)
IN_PROC_BROWSER_TEST_F(PageContentAnnotationsServiceBrowserTest,
                       NoVisitsForUrlInHistory) {
  base::HistogramTester histogram_tester;

  PageContentAnnotationsService* service =
      PageContentAnnotationsServiceFactory::GetForProfile(browser()->profile());

  HistoryVisit history_visit;
  history_visit.url = GURL("https://probablynotarealurl.com/");

  {
    base::HistogramTester histogram_tester;

    service->Annotate(history_visit, "sometext");

    RetryForHistogramUntilCountReached(
        &histogram_tester,
        "OptimizationGuide.PageContentAnnotationsService.ContentAnnotated", 1);

    histogram_tester.ExpectUniqueSample(
        "OptimizationGuide.PageContentAnnotationsService.ContentAnnotated",
        true, 1);

    RetryForHistogramUntilCountReached(
        &histogram_tester,
        "OptimizationGuide.PageContentAnnotationsService."
        "ContentAnnotationsStorageStatus",
        1);

    histogram_tester.ExpectUniqueSample(
        "OptimizationGuide.PageContentAnnotationsService."
        "ContentAnnotationsStorageStatus",
        PageContentAnnotationsStorageStatus::kNoVisitsForUrl, 1);

    EXPECT_FALSE(GetContentAnnotationsForURL(history_visit.url).has_value());
  }

  {
    base::HistogramTester histogram_tester;

    // Make sure a repeat visit is not annotated again.
    service->Annotate(history_visit, "sometext");

    base::RunLoop().RunUntilIdle();

    histogram_tester.ExpectTotalCount(
        "OptimizationGuide.PageContentAnnotationsService.ContentAnnotated", 0);
  }
}

class PageContentAnnotationsServiceNoHistoryTest
    : public PageContentAnnotationsServiceBrowserTest {
 public:
  PageContentAnnotationsServiceNoHistoryTest() {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{features::kOptimizationHints, {}},
         {features::kPageContentAnnotations,
          {
              {"write_to_history_service", "false"},
          }}},
        /*disabled_features=*/{features::kLoadModelFileForEachExecution});
  }
  ~PageContentAnnotationsServiceNoHistoryTest() override = default;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// TODO(https://crbug.com/1225946): Test is flaky.
IN_PROC_BROWSER_TEST_F(PageContentAnnotationsServiceNoHistoryTest,
                       DISABLED_ModelExecutesButDoesntWriteToHistory) {
  base::HistogramTester histogram_tester;

  GURL url(embedded_test_server()->GetURL("a.com", "/hello-no-history.html"));
  ui_test_utils::NavigateToURL(browser(), url);

  RetryForHistogramUntilCountReached(
      &histogram_tester,
      "OptimizationGuide.PageContentAnnotationsService.ContentAnnotated", 1);

  histogram_tester.ExpectUniqueSample(
      "OptimizationGuide.PageContentAnnotationsService.ContentAnnotated", true,
      1);

  base::RunLoop().RunUntilIdle();

  histogram_tester.ExpectTotalCount(
      "OptimizationGuide.PageContentAnnotationsService."
      "ContentAnnotationsStorageStatus",
      0);

  EXPECT_FALSE(GetContentAnnotationsForURL(url).has_value());
}

class PageContentAnnotationsServiceLoadEachExecutionTest
    : public PageContentAnnotationsServiceBrowserTest {
 public:
  PageContentAnnotationsServiceLoadEachExecutionTest() {
    scoped_feature_list_.InitWithFeatures(
        /*enabled_features=*/{features::kOptimizationHints,
                              features::kPageContentAnnotations,
                              features::kLoadModelFileForEachExecution},
        /*disabled_features=*/{});
    set_model_is_lazily_loaded(true);
  }
  ~PageContentAnnotationsServiceLoadEachExecutionTest() override = default;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Regression test for crbug/1204162.
IN_PROC_BROWSER_TEST_F(PageContentAnnotationsServiceLoadEachExecutionTest,
                       ModelLoadsAndExecutes) {
  base::HistogramTester histogram_tester;

  GURL url(embedded_test_server()->GetURL("a.com", "/hello.html"));
  ui_test_utils::NavigateToURL(browser(), url);

  RetryForHistogramUntilCountReached(
      &histogram_tester,
      "OptimizationGuide.ModelExecutor.ModelLoadingResult.PageTopics", 1);
  histogram_tester.ExpectUniqueSample(
      "OptimizationGuide.ModelExecutor.ModelLoadingResult.PageTopics",
      ModelExecutorLoadingState::kModelFileValidAndMemoryMapped, 1);
  histogram_tester.ExpectTotalCount(
      "OptimizationGuide.ModelExecutor.ModelLoadingDuration.PageTopics", 1);

  RetryForHistogramUntilCountReached(
      &histogram_tester,
      "OptimizationGuide.PageContentAnnotationsService.ContentAnnotated", 1);

  histogram_tester.ExpectTotalCount(
      "OptimizationGuide.PageContentAnnotationsService.ContentAnnotated", 1);
  histogram_tester.ExpectUniqueSample(
      "OptimizationGuide.PageContentAnnotationsService.ContentAnnotated", true,
      1);

  PageContentAnnotationsService* service =
      PageContentAnnotationsServiceFactory::GetForProfile(browser()->profile());

  absl::optional<int64_t> model_version = service->GetPageTopicsModelVersion();
  EXPECT_TRUE(model_version.has_value());
  EXPECT_EQ(123, *model_version);

  RetryForHistogramUntilCountReached(
      &histogram_tester,
      "OptimizationGuide.PageContentAnnotationsService."
      "ContentAnnotationsStorageStatus",
      1);

  histogram_tester.ExpectUniqueSample(
      "OptimizationGuide.PageContentAnnotationsService."
      "ContentAnnotationsStorageStatus",
      PageContentAnnotationsStorageStatus::kSuccess, 1);

  absl::optional<history::VisitContentAnnotations> got_content_annotations =
      GetContentAnnotationsForURL(url);
  ASSERT_TRUE(got_content_annotations.has_value());
  EXPECT_NE(-1.0,
            got_content_annotations->model_annotations.floc_protected_score);
  EXPECT_FALSE(got_content_annotations->model_annotations.categories.empty());
  EXPECT_EQ(
      123,
      got_content_annotations->model_annotations.page_topics_model_version);
}

class PageContentAnnotationsServiceLoadEachExecutionNotStartupTest
    : public PageContentAnnotationsServiceBrowserTest {
 public:
  PageContentAnnotationsServiceLoadEachExecutionNotStartupTest() {
    scoped_feature_list_.InitWithFeatures(
        /*enabled_features=*/{features::kOptimizationHints,
                              features::kPageContentAnnotations,
                              features::kLoadModelFileForEachExecution},
        /*disabled_features=*/{});
    set_model_is_lazily_loaded(true);
    set_load_model_on_startup(false);
  }
  ~PageContentAnnotationsServiceLoadEachExecutionNotStartupTest() override =
      default;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Flaky on Win 7 Tests x64: crbug.com/1223172
#if defined(OS_WIN)
#define MAYBE_ModelNotAvailableForFirstNavigation \
  DISABLED_ModelNotAvailableForFirstNavigation
#else
#define MAYBE_ModelNotAvailableForFirstNavigation \
  ModelNotAvailableForFirstNavigation
#endif
IN_PROC_BROWSER_TEST_F(
    PageContentAnnotationsServiceLoadEachExecutionNotStartupTest,
    MAYBE_ModelNotAvailableForFirstNavigation) {
  base::HistogramTester histogram_tester;

  GURL url(embedded_test_server()->GetURL("a.com", "/hello.html"));
  ui_test_utils::NavigateToURL(browser(), url);

  RetryForHistogramUntilCountReached(
      &histogram_tester,
      "OptimizationGuide.PageContentAnnotationsService.ModelAvailable", 1);

  histogram_tester.ExpectUniqueSample(
      "OptimizationGuide.PageContentAnnotationsService.ModelAvailable", false,
      1);

  LoadAndWaitForModel();

  GURL url2(
      embedded_test_server()->GetURL("a.com", "/hello.html?totally=different"));
  ui_test_utils::NavigateToURL(browser(), url2);

  RetryForHistogramUntilCountReached(
      &histogram_tester,
      "OptimizationGuide.PageContentAnnotationsService.ModelAvailable", 2);

  histogram_tester.ExpectBucketCount(
      "OptimizationGuide.PageContentAnnotationsService.ModelAvailable", false,
      1);
  histogram_tester.ExpectBucketCount(
      "OptimizationGuide.PageContentAnnotationsService.ModelAvailable", true,
      1);
  histogram_tester.ExpectTotalCount(
      "OptimizationGuide.PageContentAnnotationsService.ModelAvailable", 2);
}
#endif  // BUILDFLAG(BUILD_WITH_TFLITE_LIB)

}  // namespace optimization_guide
