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

#include "neva/app_runtime/app/app_runtime_page_contents.h"

#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/user_agent.h"
#include "neva/app_runtime/app/app_runtime_main_delegate.h"
#include "neva/app_runtime/app/app_runtime_page_contents_delegate.h"
#include "neva/app_runtime/browser/app_runtime_browser_context.h"
#include "neva/app_runtime/browser/app_runtime_browser_context_adapter.h"
#include "neva/app_runtime/webapp_injection_manager.h"
#include "neva/app_runtime/webview_profile.h"
#include "neva/logging.h"
#include "url/gurl.h"

namespace neva_app_runtime {

PageContents::CreateParams::CreateParams() = default;

PageContents::CreateParams::CreateParams(const CreateParams&) = default;

PageContents::CreateParams&
PageContents::CreateParams::operator=(const CreateParams&) = default;

PageContents::CreateParams::~CreateParams() = default;

PageContents::PageContents(const CreateParams& params)
    : delegate_(params.delegate ? params.delegate : &stub_delegate_),
      profile_(params.profile),
      injections_(params.injections),
      injection_manager_(std::make_unique<WebAppInjectionManager>()) {
  content::BrowserContext* browser_context =
      profile_->GetBrowserContextAdapter()->GetBrowserContext();
  content::WebContents::CreateParams contents_params(browser_context, nullptr);
  web_contents_ = std::unique_ptr<content::WebContents>(
      content::WebContents::Create(contents_params));
  web_contents_->SetDelegate(this);
}

PageContents::~PageContents() {
  if (delegate_)
    delegate_->OnDestroying(this);
  web_contents_->SetDelegate(nullptr);
}

void PageContents::SetDelegate(PageContentsDelegate* delegate) {
  delegate_ = delegate ? delegate : &stub_delegate_;
}

PageContentsDelegate* PageContents::GetDelegate() const {
  return (delegate_ == &stub_delegate_) ? nullptr : delegate_;
}

content::WebContents* PageContents::GetWebContents() const {
  return web_contents_.get();
}

void PageContents::LoadURL(std::string url_string) {
  const GURL url(url_string);
  content::NavigationController::LoadURLParams params(url);
  params.transition_type = ui::PageTransitionFromInt(
      ui::PAGE_TRANSITION_TYPED | ui::PAGE_TRANSITION_FROM_API);
  params.frame_name = std::string("");
  params.override_user_agent = content::NavigationController::UA_OVERRIDE_TRUE;
  // todo: allow_local_resources_load_
  params.can_load_local_resources = true;
  web_contents_->GetController().LoadURLWithParams(params);
  RequestAllInjectionsLoading();
}

PageView* PageContents::GetParentPageView() const {
  return parent_page_view_;
}

void PageContents::SetParentPageView(PageView* page_view) {
  NEVA_DCHECK(!page_view || !parent_page_view_);
  parent_page_view_ = page_view;
}

void PageContents::RequestAllInjectionsLoading() {
  for (const auto& name : injections_) {
    injection_manager_->RequestLoadInjection(web_contents_->GetMainFrame(),
                                             name);
  }
}

}  // namespace neva_app_runtime
