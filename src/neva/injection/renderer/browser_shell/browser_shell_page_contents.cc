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

#include "neva/injection/renderer/browser_shell/browser_shell_page_contents.h"

#include "base/bind.h"
#include "base/logging.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "gin/handle.h"
#include "neva/browser_shell/service/public/mojom/browser_shell_constants.mojom.h"

namespace injections {

gin::WrapperInfo BrowserShellPageContents::kWrapperInfo = {
    gin::kEmbedderNativeGin
};

char BrowserShellPageContents::kOnMethodName[] = "on";
char BrowserShellPageContents::kLoadURLMethodName[] = "loadURL";

// static
void BrowserShellPageContents::ConstructorCallback(
    mojo::Remote<browser_shell::mojom::ShellService>* shell_service,
    gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  v8::HandleScope handle_scope(isolate);

  if (!args->IsConstructCall()) {
    isolate->ThrowException(v8::Exception::Error(
        gin::StringToV8(isolate, "Must be a constructor call")));
    return;
  }

  mojo::Remote<browser_shell::mojom::PageContents> remote_contents;
  auto pending_receiver = remote_contents.BindNewPipeAndPassReceiver();
  auto* shell_page_contents = new injections::BrowserShellPageContents(
      isolate, std::move(remote_contents));

  (*shell_service)
      ->CreatePageContents(
          std::move(pending_receiver),
          base::BindOnce(&BrowserShellPageContents::Setup,
                         base::Unretained(shell_page_contents)));

  gin::Handle<injections::BrowserShellPageContents> handle =
      gin::CreateHandle(isolate, shell_page_contents);

  if (!handle.IsEmpty())
    args->Return(handle.ToV8());
}

BrowserShellPageContents::BrowserShellPageContents(
    v8::Isolate* isolate,
    mojo::Remote<browser_shell::mojom::PageContents> remote,
    bool is_main_contents)
    : is_main_contents_(is_main_contents),
      remote_(std::move(remote)),
      client_receiver_(this) {}

BrowserShellPageContents::~BrowserShellPageContents() = default;

uint64_t BrowserShellPageContents::GetID() {
  if (!id_ && remote_.is_connected())
    remote_->SyncId(&id_);
  return id_;
}

void BrowserShellPageContents::Setup(
    uint64_t id,
    mojo::PendingAssociatedReceiver<browser_shell::mojom::PageContentsClient>
        receiver) {
  id_ = id;
  client_receiver_.Bind(std::move(receiver));
}

void BrowserShellPageContents::OnLoadURL(const std::string& url) {
  NOTIMPLEMENTED();
}

void BrowserShellPageContents::Updated(
    browser_shell::mojom::PageContentsStatePtr state) {
  NOTIMPLEMENTED();
}

void BrowserShellPageContents::DidLoad(const std::string& url) {
  NOTIMPLEMENTED();
}

void BrowserShellPageContents::SetEventHandler(gin::Arguments* args) {
  NOTIMPLEMENTED();
}

void BrowserShellPageContents::LoadURL(std::string url) {
  if (is_main_contents_) {
    LOG(INFO) << "Reloading contents of main view is now unsupported!";
    return;
  }
  remote_->LoadURL(url, base::BindOnce(&BrowserShellPageContents::OnLoadURL,
                                       base::Unretained(this)));
}

gin::ObjectTemplateBuilder BrowserShellPageContents::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<BrowserShellPageContents>::GetObjectTemplateBuilder(
             isolate)
      .SetMethod(kOnMethodName, &BrowserShellPageContents::SetEventHandler)
      .SetMethod(kLoadURLMethodName, &BrowserShellPageContents::LoadURL);
}

}  // namespace injections
