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

#include "neva/injection/renderer/browser_shell/browser_shell_page_view.h"

#include "base/bind.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "gin/handle.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "neva/browser_shell/service/public/mojom/browser_shell_page_contents.mojom.h"
#include "neva/injection/renderer/browser_shell/browser_shell_page_contents.h"

namespace injections {

gin::WrapperInfo BrowserShellPageView::kWrapperInfo = {
    gin::kEmbedderNativeGin };

char BrowserShellPageView::kAddChildViewMethodName[] = "addChildView";
char BrowserShellPageView::kBringToFrontMethodName[] = "bringToFront";
char BrowserShellPageView::kGetBoundsMethodName[] = "getBounds";
char BrowserShellPageView::kGetChildViewsMethodName[] = "getChildViews";
char BrowserShellPageView::kIsVisibleMethodName[] = "isVisible";
char BrowserShellPageView::kOnMethodName[] = "on";
char BrowserShellPageView::kPageContentsPropertyName[] = "pageContents";
char BrowserShellPageView::kRemoveChildViewMethodName[] = "removeChildView";
char BrowserShellPageView::kSendToBackMethodName[] = "sendToBack";
char BrowserShellPageView::kSetBoundsMethodName[] = "setBounds";
char BrowserShellPageView::kSetVisibleMethodName[] = "setVisible";

// static
void BrowserShellPageView::ConstructorCallback(
    mojo::Remote<browser_shell::mojom::ShellService>* shell_service,
    gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  v8::HandleScope handle_scope(isolate);

  if (!args->IsConstructCall()) {
    isolate->ThrowException(v8::Exception::Error(
        gin::StringToV8(args->isolate(), "Must be a constructor call")));
    return;
  }

  mojo::Remote<browser_shell::mojom::PageView> remote_view;
  auto pending_receiver = remote_view.BindNewPipeAndPassReceiver();
  auto* shell_page_view =
      new injections::BrowserShellPageView(isolate, std::move(remote_view));

  (*shell_service)
      ->CreatePageView(std::move(pending_receiver),
                       base::BindOnce(&BrowserShellPageView::Setup,
                                      base::Unretained(shell_page_view)));

  gin::Handle<injections::BrowserShellPageView> handle =
      gin::CreateHandle(isolate, shell_page_view);

  if (!handle.IsEmpty())
    args->Return(handle.ToV8());
}

BrowserShellPageView::BrowserShellPageView(
    v8::Isolate* isolate,
    mojo::Remote<browser_shell::mojom::PageView> remote,
    bool is_main_view)
    : is_main_view_(is_main_view),
      remote_(std::move(remote)),
      client_receiver_(this) {
  mojo::Remote<browser_shell::mojom::PageContents> remote_contents;
  auto pending_receiver = remote_contents.BindNewPipeAndPassReceiver();
  auto* shell_page_contents = new injections::BrowserShellPageContents(
      isolate, std::move(remote_contents), is_main_view);

  remote_->BindPageContents(
      std::move(pending_receiver),
      base::BindOnce(&BrowserShellPageContents::Setup,
                     base::Unretained(shell_page_contents)));

  gin::Handle<injections::BrowserShellPageContents> handle =
      gin::CreateHandle(isolate, shell_page_contents);

  if (!handle.IsEmpty()) {
    page_contents_object_.Reset(isolate,
                                handle->GetWrapper(isolate).ToLocalChecked());
  }
}

BrowserShellPageView::~BrowserShellPageView() = default;

uint64_t BrowserShellPageView::GetID() {
  if (!id_ && remote_.is_connected())
    remote_->SyncId(&id_);
  return id_;
}

bool BrowserShellPageView::IsMainView() const {
  return is_main_view_;
}

void BrowserShellPageView::Setup(
    uint64_t id,
    mojo::PendingAssociatedReceiver<browser_shell::mojom::PageViewClient>
        receiver) {
  id_ = id;
  client_receiver_.Bind(std::move(receiver));
}

void BrowserShellPageView::Updated() {
  NOTIMPLEMENTED();
}

v8::Local<v8::Object> BrowserShellPageView::GetPageContents(
    v8::Isolate* isolate) {
  return page_contents_object_.Get(isolate);
}

void BrowserShellPageView::SetPageContents(v8::Isolate* isolate,
                                           v8::Local<v8::Object> object) {
  if (is_main_view_)
    return;

  BrowserShellPageContents* page_contents = nullptr;
  gin::Converter<BrowserShellPageContents*>::FromV8(isolate, object,
                                                    &page_contents);
  if (page_contents) {
    const uint64_t id = page_contents->GetID();
    remote_->SetPageContents(id);
    page_contents_object_.Reset(isolate, object);
  }
}

void BrowserShellPageView::SetEventHandler(gin::Arguments* args) {
  NOTIMPLEMENTED();
}

void BrowserShellPageView::AddChildView(v8::Isolate* isolate,
                                        v8::Local<v8::Object> object) {
  BrowserShellPageView* page_view = nullptr;
  gin::Converter<BrowserShellPageView*>::FromV8(isolate, object, &page_view);

  if (!page_view || page_view->IsMainView())
    return;

  const uint64_t id = page_view->GetID();
  if (!id)
    return;

  v8::Global<v8::Object> child_obj(isolate, object);
  if (child_view_objects_.insert({id, std::move(child_obj)}).second)
    remote_->AddChildView(id);
}

void BrowserShellPageView::RemoveChildView(v8::Isolate* isolate,
                                           v8::Local<v8::Object> object) {
  BrowserShellPageView* page_view = nullptr;
  gin::Converter<BrowserShellPageView*>::FromV8(isolate, object, &page_view);

  if (page_view) {
    const uint64_t id = page_view->GetID();
    if (id && child_view_objects_.erase(id))
      remote_->RemoveChildView(id);
  }
}

void BrowserShellPageView::GetChildViews(gin::Arguments* args) {
  NOTIMPLEMENTED();
}

void BrowserShellPageView::SetVisible(bool visible) {
  remote_->SetVisible(visible);
  visible_ = visible;
}

bool BrowserShellPageView::IsVisible() const {
  return visible_;
}

void BrowserShellPageView::SetBounds(int x, int y, int w, int h) {
  remote_->SetBounds(x, y, w, h);
}

void BrowserShellPageView::GetBounds(gin::Arguments* args) {
  NOTIMPLEMENTED();
}

void BrowserShellPageView::BringToFront() {
  remote_->BringToFront();
}

void BrowserShellPageView::SendToBack() {
  remote_->SendToBack();
}

gin::ObjectTemplateBuilder
BrowserShellPageView::GetObjectTemplateBuilder(v8::Isolate* isolate) {
  return gin::Wrappable<BrowserShellPageView>::GetObjectTemplateBuilder(isolate)
      .SetProperty(kPageContentsPropertyName,
                   &BrowserShellPageView::GetPageContents,
                   &BrowserShellPageView::SetPageContents)
      .SetMethod(kOnMethodName, &BrowserShellPageView::SetEventHandler)
      .SetMethod(kSetBoundsMethodName, &BrowserShellPageView::SetBounds)
      .SetMethod(kGetBoundsMethodName, &BrowserShellPageView::GetBounds)
      .SetMethod(kSetVisibleMethodName, &BrowserShellPageView::SetVisible)
      .SetMethod(kIsVisibleMethodName, &BrowserShellPageView::IsVisible)
      .SetMethod(kAddChildViewMethodName, &BrowserShellPageView::AddChildView)
      .SetMethod(kRemoveChildViewMethodName,
                 &BrowserShellPageView::RemoveChildView)
      .SetMethod(kGetChildViewsMethodName, &BrowserShellPageView::GetChildViews)
      .SetMethod(kBringToFrontMethodName, &BrowserShellPageView::BringToFront)
      .SetMethod(kSendToBackMethodName, &BrowserShellPageView::SendToBack);
}

}  // namespace injections
