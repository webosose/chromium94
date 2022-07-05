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

#ifndef NEVA_BROWSER_SHELL_SERVICE_BROWSER_SHELL_PAGE_CONTENTS_IMPL_H_
#define NEVA_BROWSER_SHELL_SERVICE_BROWSER_SHELL_PAGE_CONTENTS_IMPL_H_

#include "base/component_export.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "neva/app_runtime/app/app_runtime_page_contents_delegate.h"
#include "neva/browser_shell/service/public/mojom/browser_shell_page_contents.mojom.h"

namespace neva_app_runtime {
class PageContents;
}

namespace browser_shell {

class ShellEnvironment;

class PageContentsImpl : public mojom::PageContents,
                         public neva_app_runtime::PageContentsDelegate {
 public:
  PageContentsImpl(neva_app_runtime::PageContents* page_contents,
                   ShellEnvironment* shell_environment);
  PageContentsImpl(const PageContentsImpl&) = delete;
  PageContentsImpl& operator=(const PageContentsImpl&) = delete;
  ~PageContentsImpl() override;

  uint64_t GetID() const;
  neva_app_runtime::PageContents* GetPageContents() const;
  mojo::PendingAssociatedReceiver<mojom::PageContentsClient>
  BindNewEndpointAndPassReceiver();

  // mojom::PageContents
  void SyncId(SyncIdCallback callback) override;
  void LoadURL(const std::string& url, LoadURLCallback callback) override;
  void Stop() override;
  void Reload() override;
  void GoBack() override;
  void GoForward() override;

  // neva_app_runtime::PageContentsDelegate
  void OnDestroying(neva_app_runtime::PageContents* contents) override;

 private:
  ShellEnvironment* const shell_environment_;
  neva_app_runtime::PageContents* const page_contents_;
  const uint64_t id_ = 0;
  mojo::AssociatedRemote<mojom::PageContentsClient> remote_client_;
};

}  // namespace browser_shell

#endif  // NEVA_BROWSER_SHELL_SERVICE_BROWSER_SHELL_PAGE_CONTENTS_IMPL_H_
