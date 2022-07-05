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

#ifndef NEVA_BROWSER_SHELL_SERVICE_BROWSER_SHELL_WINDOW_IMPL_H_
#define NEVA_BROWSER_SHELL_SERVICE_BROWSER_SHELL_WINDOW_IMPL_H_

#include <string>

#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "neva/app_runtime/app/app_runtime_shell_window_delegate.h"
#include "neva/browser_shell/service/public/mojom/browser_shell_window.mojom.h"

namespace neva_app_runtime {
class ShellWindow;
}

namespace browser_shell {

class ShellEnvironment;

class ShellWindowImpl : public mojom::ShellWindow,
                        public neva_app_runtime::ShellWindowDelegate {
 public:
  ShellWindowImpl(neva_app_runtime::ShellWindow* shell_window,
                  ShellEnvironment* shell_environment,
                  std::string name);
  ShellWindowImpl(const ShellWindowImpl&) = delete;
  ShellWindowImpl& operator=(const ShellWindowImpl&) = delete;
  ~ShellWindowImpl() override;

  std::string GetName() const;
  neva_app_runtime::ShellWindow* GetShellWindow() const;

  mojo::PendingAssociatedReceiver<mojom::ShellWindowClient>
  BindNewEndpointAndPassReceiver();

  // mojom::ShellWindow
  void BindPageView(mojo::PendingReceiver<mojom::PageView> receiver,
                    BindPageViewCallback callback) override;

  void SyncName(SyncNameCallback callback) override;

  // neva_app_runtime::ShellWindowDelegate
  void OnWindowClosing() override;

 private:
  std::string name_;
  ShellEnvironment* shell_environment_;
  neva_app_runtime::ShellWindow* shell_window_;
  mojo::AssociatedRemote<mojom::ShellWindowClient> remote_client_;
};

}  // namespace browser_shell

#endif  // NEVA_BROWSER_SHELL_SERVICE_BROWSER_SHELL_WINDOW_IMPL_H_
