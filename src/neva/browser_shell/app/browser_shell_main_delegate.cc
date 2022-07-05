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

#include "neva/browser_shell/app/browser_shell_main_delegate.h"

#include "base/command_line.h"
#include "base/run_loop.h"
#include "content/public/common/content_switches.h"
#include "neva/app_runtime/app/app_runtime_shell.h"
#include "neva/app_runtime/browser/app_runtime_browser_main_parts.h"
#include "neva/app_runtime/browser/app_runtime_content_browser_client.h"
#include "neva/browser_shell/common/browser_shell_switches.h"
#include "neva/browser_shell/service/browser_shell_service_impl.h"
#include "neva/browser_shell/service/public/browser_shell_service.h"
#include "url/gurl.h"

namespace browser_shell {

BrowserShellMainDelegate::BrowserShellMainDelegate(
    const content::MainFunctionParams& parameters)
  : parameters_(parameters) {}

BrowserShellMainDelegate::~BrowserShellMainDelegate() = default;

void BrowserShellMainDelegate::PreMainMessageLoopRun() {
  std::string path = base::CommandLine::ForCurrentProcess()->
      GetSwitchValueASCII(switches::kShellAppPath);
  GURL url(path);
  if (!url.is_valid())
    LOG(ERROR) << "shell-app-path switch has invalid url: " << path;

  const bool enable_dev_tools =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kRemoteDebuggingPort);

  if (enable_dev_tools) {
    // TODO(pikulik): That is how it's done in wam_demo and app_runtime
    // now. I think we should to revise the method we get access to
    // AppRuntimeBrowserMainParts.
    neva_app_runtime::AppRuntimeBrowserMainParts* main_parts =
        static_cast<neva_app_runtime::AppRuntimeBrowserMainParts*>(
            neva_app_runtime::GetAppRuntimeContentBrowserClient()
                ->GetMainParts());
    if (main_parts)
      main_parts->EnableDevTools();
  }

  auto shell = std::make_unique<neva_app_runtime::Shell>(enable_dev_tools);
  shell->CreateMainWindow(url.spec());
  RegisterShellService(std::make_unique<ShellServiceImpl>(std::move(shell)));
}

void BrowserShellMainDelegate::WillRunMainMessageLoop(
    std::unique_ptr<base::RunLoop>& run_loop) {
  neva_app_runtime::Shell::SetQuitClosure(run_loop->QuitClosure());
}

}  // namespace browser_shell
