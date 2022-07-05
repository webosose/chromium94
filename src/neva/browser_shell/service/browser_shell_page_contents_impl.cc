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

#include "neva/browser_shell/service/browser_shell_page_contents_impl.h"

#include "base/logging.h"
#include "neva/app_runtime/app/app_runtime_page_contents.h"
#include "neva/browser_shell/service/browser_shell_environment.h"
#include "neva/logging.h"

namespace browser_shell {

PageContentsImpl::PageContentsImpl(
    neva_app_runtime::PageContents* page_contents,
    ShellEnvironment* shell_environment)
    : shell_environment_(shell_environment),
      page_contents_(page_contents),
      id_(shell_environment ? shell_environment->GetNextIDFor(page_contents)
                            : 0) {
  NEVA_DCHECK(shell_environment_);
  NEVA_DCHECK(page_contents_);
  NEVA_DCHECK(id_);
  page_contents_->SetDelegate(this);
}

PageContentsImpl::~PageContentsImpl() {
  page_contents_->SetDelegate(nullptr);
  shell_environment_->Remove(page_contents_);
}

uint64_t PageContentsImpl::GetID() const {
  return id_;
}

neva_app_runtime::PageContents* PageContentsImpl::GetPageContents() const {
  return page_contents_;
}

mojo::PendingAssociatedReceiver<mojom::PageContentsClient>
PageContentsImpl::BindNewEndpointAndPassReceiver() {
  return remote_client_.BindNewEndpointAndPassReceiver();
}

void PageContentsImpl::SyncId(SyncIdCallback callback) {
  std::move(callback).Run(id_);
}

void PageContentsImpl::LoadURL(const std::string& url,
                               LoadURLCallback callback) {
  page_contents_->LoadURL(url);
  std::move(callback).Run(url);
  remote_client_->DidLoad(url);
}

void PageContentsImpl::Stop() {
  NOTIMPLEMENTED();
}

void PageContentsImpl::Reload() {
  NOTIMPLEMENTED();
}

void PageContentsImpl::GoBack() {
  NOTIMPLEMENTED();
}

void PageContentsImpl::GoForward() {
  NOTIMPLEMENTED();
}

void PageContentsImpl::OnDestroying(neva_app_runtime::PageContents* contents) {}

}  // namespace browser_shell
