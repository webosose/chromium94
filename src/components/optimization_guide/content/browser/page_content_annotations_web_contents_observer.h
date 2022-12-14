// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OPTIMIZATION_GUIDE_CONTENT_BROWSER_PAGE_CONTENT_ANNOTATIONS_WEB_CONTENTS_OBSERVER_H_
#define COMPONENTS_OPTIMIZATION_GUIDE_CONTENT_BROWSER_PAGE_CONTENT_ANNOTATIONS_WEB_CONTENTS_OBSERVER_H_

#include "base/memory/weak_ptr.h"
#include "components/optimization_guide/content/browser/page_text_dump_result.h"
#include "components/optimization_guide/content/browser/page_text_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class NavigationHandle;
}  // namespace content

namespace optimization_guide {

struct HistoryVisit;
class PageContentAnnotationsService;

// This class is used to dispatch page content to the
// PageContentAnnotationsService to be annotated.
class PageContentAnnotationsWebContentsObserver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<
          PageContentAnnotationsWebContentsObserver>,
      public PageTextObserver::Consumer {
 public:
  ~PageContentAnnotationsWebContentsObserver() override;

  PageContentAnnotationsWebContentsObserver(
      const PageContentAnnotationsWebContentsObserver&) = delete;
  PageContentAnnotationsWebContentsObserver& operator=(
      const PageContentAnnotationsWebContentsObserver&) = delete;

 protected:
  PageContentAnnotationsWebContentsObserver(
      content::WebContents* web_contents,
      PageContentAnnotationsService* page_content_annotations_service);

 private:
  friend class content::WebContentsUserData<
      PageContentAnnotationsWebContentsObserver>;
  friend class PageContentAnnotationsWebContentsObserverTest;

  // content::WebContentsObserver:
  void DidFinishNavigation(content::NavigationHandle* handle) override;

  // PageTextObserver::Consumer:
  std::unique_ptr<PageTextObserver::ConsumerTextDumpRequest>
  MaybeRequestFrameTextDump(
      content::NavigationHandle* navigation_handle) override;

  // Callback invoked when a text dump has been received for the |visit|.
  void OnTextDumpReceived(const HistoryVisit& visit,
                          const PageTextDumpResult& result);

  // Not owned. Guaranteed to outlive |this|.
  PageContentAnnotationsService* page_content_annotations_service_;

  // The max size to request for text dump.
  const uint64_t max_size_for_text_dump_;

  base::WeakPtrFactory<PageContentAnnotationsWebContentsObserver>
      weak_ptr_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace optimization_guide

#endif  // COMPONENTS_OPTIMIZATION_GUIDE_CONTENT_BROWSER_PAGE_CONTENT_ANNOTATIONS_WEB_CONTENTS_OBSERVER_H_
