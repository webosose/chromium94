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

#include "neva/app_runtime/app/app_runtime_shell.h"

#include "content/public/browser/browser_thread.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/render_process_host.h"
#include "neva/app_runtime/app/app_runtime_page_contents.h"
#include "neva/app_runtime/app/app_runtime_page_view.h"
#include "neva/app_runtime/app/app_runtime_shell_window.h"
#include "neva/app_runtime/browser/ui/app_runtime_views_delegate.h"
#include "neva/app_runtime/webview_profile.h"

namespace neva_app_runtime {

namespace {
base::NoDestructor<base::OnceClosure> g_quit_main_message_loop;
}

Shell::Shell(bool enable_dev_tools)
    : enable_dev_tools_(enable_dev_tools),
      views_delegate_(std::make_unique<AppRuntimeViewsDelegate>()) {
}

Shell::~Shell() = default;

ShellWindow* Shell::CreateMainWindow(std::string url) {
  if (main_window_)
    return main_window_;

  PageContents::CreateParams params;
  params.profile = WebViewProfile::GetDefaultProfile();
  params.injections.push_back("v8/browser_shell");
  params.inspectable = enable_dev_tools_;

  auto page_contents = std::make_unique<PageContents>(params);
  page_contents->LoadURL(std::move(url));
  auto page_view = std::make_unique<PageView>();
  page_view->SetPageContents(std::move(page_contents));

  // Shell only creates ShellWindow.
  // After creation ShellWindow pass ownership to Widget.
  main_window_ = new ShellWindow(this);
  main_window_->SetPageView(std::move(page_view));

  windows_.insert(main_window_);
  return main_window_;
}

ShellWindow* Shell::GetMainWindow() {
  return main_window_;
}

PageContents::CreateParams Shell::GetDefaultContentsParams() {
  PageContents::CreateParams nested_params;
  nested_params.profile = WebViewProfile::GetDefaultProfile();
  nested_params.inspectable = enable_dev_tools_;
  return nested_params;
}

void Shell::OnWindowClosing() {
  Shutdown();
}

// static
void Shell::SetQuitClosure(base::OnceClosure quit_main_message_loop) {
  *g_quit_main_message_loop = std::move(quit_main_message_loop);
}

// static
void Shell::Shutdown() {
  for (auto it = content::RenderProcessHost::AllHostsIterator();
       !it.IsAtEnd(); it.Advance()) {
    it.GetCurrentValue()->DisableKeepAliveRefCount();
  }

  if (*g_quit_main_message_loop)
    std::move(*g_quit_main_message_loop).Run();
  // Pump the message loop to allow window teardown tasks to run.
  base::RunLoop().RunUntilIdle();
}

}  // namespace neva_app_runtime
