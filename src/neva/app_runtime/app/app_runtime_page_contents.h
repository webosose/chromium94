// Copyright 2021 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef NEVA_APP_RUNTIME_APP_APP_RUNTIME_PAGE_CONTENTS_H_
#define NEVA_APP_RUNTIME_APP_APP_RUNTIME_PAGE_CONTENTS_H_

#include <list>
#include <memory>
#include <string>

#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "neva/app_runtime/app/app_runtime_page_contents_delegate.h"

namespace content {
class WebContents;
}

namespace neva_app_runtime {

class PageView;
class WebAppInjectionManager;
class WebViewProfile;

class PageContents : public content::WebContentsDelegate,
                     public content::WebContentsObserver {
 public:
  struct CreateParams {
    CreateParams();
    CreateParams(const CreateParams&);
    CreateParams& operator=(const CreateParams&);
    ~CreateParams();

    int width = 0;
    int height = 0;
    PageContentsDelegate* delegate = nullptr;
    WebViewProfile* profile = nullptr;
    std::list<std::string> injections;
    bool inspectable = false;
  };

  explicit PageContents(const CreateParams& params);
  PageContents(const PageContents&) = delete;
  PageContents& operator=(const PageContents&) = delete;
  ~PageContents() override;

  void SetDelegate(PageContentsDelegate* delegate);
  PageContentsDelegate* GetDelegate() const;

  content::WebContents* GetWebContents() const;
  void LoadURL(std::string url_string);

  PageView* GetParentPageView() const;

 private:
  friend PageView;
  void SetParentPageView(PageView* page_view);
  void RequestAllInjectionsLoading();

  PageView* parent_page_view_ = nullptr;
  PageContentsDelegate* delegate_ = nullptr;
  PageContentsDelegate stub_delegate_;
  WebViewProfile* profile_ = nullptr;
  std::unique_ptr<content::WebContents> web_contents_;
  std::list<std::string> injections_;
  std::unique_ptr<WebAppInjectionManager> injection_manager_;
};

}  // namespace neva_app_runtime

#endif  // NEVA_APP_RUNTIME_APP_APP_RUNTIME_PAGE_CONTENTS_H_
