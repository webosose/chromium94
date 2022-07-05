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

#include "neva/injection/renderer/browser_shell/browser_shell_window.h"

#include "base/bind.h"
#include "gin/function_template.h"
#include "gin/handle.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "neva/browser_shell/service/public/mojom/browser_shell_constants.mojom.h"
#include "neva/injection/renderer/browser_shell/browser_shell_page_view.h"

namespace injections {

gin::WrapperInfo BrowserShellWindow::kWrapperInfo = {
    gin::kEmbedderNativeGin
};

char BrowserShellWindow::kPageViewPropertyName[] = "pageView";
char BrowserShellWindow::kOnMethodName[] = "on";
char BrowserShellWindow::kSetVisibleMethodName[] = "setVisible";
char BrowserShellWindow::kIsVisibleMethodName[] = "isVisible";

BrowserShellWindow::BrowserShellWindow(
    v8::Isolate* isolate,
    mojo::Remote<browser_shell::mojom::ShellWindow> remote)
    : remote_(std::move(remote)), client_receiver_(this) {
  mojo::Remote<browser_shell::mojom::PageView> remote_view;
  auto view_pending_receiver = remote_view.BindNewPipeAndPassReceiver();

  auto* shell_page_view = new injections::BrowserShellPageView(
      isolate, std::move(remote_view), false);

  remote_->BindPageView(std::move(view_pending_receiver),
                        base::BindOnce(&BrowserShellPageView::Setup,
                                       base::Unretained(shell_page_view)));

  gin::Handle<injections::BrowserShellPageView> handle =
      gin::CreateHandle(isolate, shell_page_view);

  if (!handle.IsEmpty()) {
    page_view_object_.Reset(isolate,
                            handle->GetWrapper(isolate).ToLocalChecked());
  }
}

BrowserShellWindow::~BrowserShellWindow() = default;

void BrowserShellWindow::Setup(
    const std::string& name,
    mojo::PendingAssociatedReceiver<browser_shell::mojom::ShellWindowClient>
        receiver) {
  name_ = name;
  client_receiver_.Bind(std::move(receiver));
}

void BrowserShellWindow::Updated() {
  NOTIMPLEMENTED();
}

std::string& BrowserShellWindow::GetName() {
  if (name_.empty() && remote_.is_connected())
    remote_->SyncName(&name_);
  return name_;
}

v8::Local<v8::Object> BrowserShellWindow::GetPageView(v8::Isolate* isolate) {
  return page_view_object_.Get(isolate);
}

void BrowserShellWindow::SetVisible(bool visible) {
  NOTIMPLEMENTED();
}

bool BrowserShellWindow::IsVisible() const {
  NOTIMPLEMENTED();
  return true;
}

void BrowserShellWindow::SetEventHandler(gin::Arguments* args) {
  NOTIMPLEMENTED();
}

gin::ObjectTemplateBuilder
BrowserShellWindow::GetObjectTemplateBuilder(v8::Isolate* isolate) {
  return gin::Wrappable<BrowserShellWindow>::GetObjectTemplateBuilder(isolate)
      .SetProperty(kPageViewPropertyName, &BrowserShellWindow::GetPageView)
      .SetMethod(kSetVisibleMethodName, &BrowserShellWindow::SetVisible)
      .SetMethod(kIsVisibleMethodName, &BrowserShellWindow::IsVisible)
      .SetMethod(kOnMethodName, &BrowserShellWindow::SetEventHandler);
}

}  // namespace injections
