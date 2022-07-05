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

#include "neva/browser_shell/service/browser_shell_service_impl.h"

#include <memory>

#include "base/bind.h"
#include "base/logging.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "neva/app_runtime/app/app_runtime_page_contents.h"
#include "neva/app_runtime/app/app_runtime_page_view.h"
#include "neva/app_runtime/app/app_runtime_shell.h"
#include "neva/app_runtime/app/app_runtime_shell_window.h"
#include "neva/browser_shell/service/browser_shell_page_contents_impl.h"
#include "neva/browser_shell/service/browser_shell_page_view_impl.h"
#include "neva/browser_shell/service/browser_shell_window_impl.h"

namespace browser_shell {

ShellServiceImpl::ShellServiceImpl(
    std::unique_ptr<neva_app_runtime::Shell> shell)
    : shell_(std::move(shell)) {}

ShellServiceImpl::~ShellServiceImpl() = default;

void ShellServiceImpl::AddBinding(
    mojo::PendingReceiver<mojom::ShellService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void ShellServiceImpl::BindRemoteClient(
    mojo::PendingRemote<mojom::ShellServiceClient> client_remote) {
  NOTIMPLEMENTED();
}

void ShellServiceImpl::BindShellWindow(
    mojo::PendingReceiver<mojom::ShellWindow> receiver,
    BindShellWindowCallback callback) {
  auto* window = shell_->GetMainWindow();
  // It's only 1 shell window allowed yet.
  if (window && shell_window_receivers_.empty()) {
    const std::string name("shell_main_window");
    auto shell_window_impl =
        std::make_unique<ShellWindowImpl>(window, &shell_environment_, name);
    auto client_receiver = shell_window_impl->BindNewEndpointAndPassReceiver();
    shell_window_receivers_.Add(std::move(shell_window_impl),
                                std::move(receiver));
    std::move(callback).Run(name, std::move(client_receiver));
    return;
  }
  std::move(callback).Run(std::string(), mojo::NullAssociatedReceiver());
}

void ShellServiceImpl::CreatePageView(
    mojo::PendingReceiver<mojom::PageView> receiver,
    CreatePageViewCallback callback) {
  auto page_view = std::make_unique<neva_app_runtime::PageView>();
  auto page_contents = std::make_unique<neva_app_runtime::PageContents>(
      shell_->GetDefaultContentsParams());
  page_view->SetPageContents(std::move(page_contents));
  auto page_view_impl =
      std::make_unique<PageViewImpl>(page_view.get(), &shell_environment_);

  shell_environment_.SaveDetachedView(std::move(page_view));

  const uint64_t id = page_view_impl->GetID();
  auto client_receiver = page_view_impl->BindNewEndpointAndPassReceiver();
  mojo::MakeSelfOwnedReceiver(std::move(page_view_impl), std::move(receiver));
  std::move(callback).Run(id, std::move(client_receiver));
}

void ShellServiceImpl::CreatePageContents(
    mojo::PendingReceiver<mojom::PageContents> receiver,
    CreatePageContentsCallback callback) {
  auto page_contents = std::make_unique<neva_app_runtime::PageContents>(
      shell_->GetDefaultContentsParams());
  auto page_contents_impl = std::make_unique<PageContentsImpl>(
      page_contents.get(), &shell_environment_);

  shell_environment_.SaveDetachedContents(std::move(page_contents));

  const uint64_t id = page_contents_impl->GetID();
  auto client_receiver = page_contents_impl->BindNewEndpointAndPassReceiver();
  mojo::MakeSelfOwnedReceiver(std::move(page_contents_impl),
                              std::move(receiver));
  std::move(callback).Run(id, std::move(client_receiver));
}

}  // namespace browser_shell
