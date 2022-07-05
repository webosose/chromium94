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

#include "neva/browser_shell/service/browser_shell_page_view_impl.h"

#include "base/logging.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "neva/app_runtime/app/app_runtime_page_contents.h"
#include "neva/app_runtime/app/app_runtime_page_view.h"
#include "neva/browser_shell/service/browser_shell_environment.h"
#include "neva/browser_shell/service/browser_shell_page_contents_impl.h"
#include "neva/logging.h"

namespace browser_shell {

PageViewImpl::PageViewImpl(neva_app_runtime::PageView* page_view,
                           ShellEnvironment* shell_environment)
    : shell_environment_(shell_environment),
      page_view_(page_view),
      id_(shell_environment ? shell_environment->GetNextIDFor(page_view) : 0) {
  NEVA_DCHECK(shell_environment_);
  NEVA_DCHECK(page_view_);
  NEVA_DCHECK(id_);
  page_view_->SetDelegate(this);
}

PageViewImpl::~PageViewImpl() {
  page_view_->SetDelegate(this);
  shell_environment_->Remove(page_view_);
}

uint64_t PageViewImpl::GetID() const {
  return id_;
}

neva_app_runtime::PageView* PageViewImpl::GetPageView() const {
  return page_view_;
}

mojo::PendingAssociatedReceiver<mojom::PageViewClient>
PageViewImpl::BindNewEndpointAndPassReceiver() {
  return remote_client_.BindNewEndpointAndPassReceiver();
}

void PageViewImpl::BindPageContents(
    mojo::PendingReceiver<mojom::PageContents> receiver,
    BindPageContentsCallback callback) {
  auto* page_contents = page_view_->GetPageContents();
  auto page_contents_impl =
      std::make_unique<PageContentsImpl>(page_contents, shell_environment_);
  const uint64_t id = page_contents_impl->GetID();
  auto client_receiver = page_contents_impl->BindNewEndpointAndPassReceiver();
  mojo::MakeSelfOwnedReceiver(std::move(page_contents_impl),
                              std::move(receiver));
  std::move(callback).Run(id, std::move(client_receiver));
}

void PageViewImpl::SyncId(SyncIdCallback callback) {
  std::move(callback).Run(id_);
}

void PageViewImpl::SetBounds(int32_t x, int32_t y, int32_t w, int32_t h) {
  page_view_->SetBounds(x, y, w, h);
}

void PageViewImpl::SetVisible(bool visible) {
  page_view_->SetVisible(visible);
}

void PageViewImpl::BringToFront() {
  page_view_->BringToFront();
}

void PageViewImpl::SendToBack() {
  page_view_->SendToBack();
}

void PageViewImpl::AddChildView(uint64_t id) {
  auto* child_ptr = shell_environment_->GetViewPtr(id);
  if (!child_ptr)
    return;

  auto child = shell_environment_->ReleaseDetachedView(child_ptr);
  if (child)
    page_view_->AddChildPageView(std::move(child));
}

void PageViewImpl::RemoveChildView(uint64_t id) {
  auto* child_ptr = shell_environment_->GetViewPtr(id);
  if (!child_ptr)
    return;

  auto child = page_view_->RemoveChildPageView(child_ptr);
  if (child.get() && child->GetDelegate())
    shell_environment_->SaveDetachedView(std::move(child));
}

void PageViewImpl::SetPageContents(uint64_t id) {
  auto* contents_ptr = shell_environment_->GetContentsPtr(id);
  if (!contents_ptr)
    return;

  auto contents = shell_environment_->ReleaseDetachedContents(contents_ptr);
  auto previos_contents = page_view_->SetPageContents(std::move(contents));
  if (previos_contents.get() && previos_contents->GetDelegate())
    shell_environment_->SaveDetachedContents(std::move(previos_contents));
}

void PageViewImpl::OnDestroying(neva_app_runtime::PageView* view) {}

}  // namespace browser_shell
