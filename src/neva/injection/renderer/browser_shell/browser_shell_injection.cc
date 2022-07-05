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

#include "neva/injection/renderer/browser_shell/browser_shell_injection.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "gin/function_template.h"
#include "gin/handle.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "neva/browser_shell/service/public/mojom/browser_shell_window.mojom.h"
#include "neva/injection/renderer/browser_shell/browser_shell_page_contents.h"
#include "neva/injection/renderer/browser_shell/browser_shell_page_view.h"
#include "neva/injection/renderer/browser_shell/browser_shell_window.h"
#include "neva/injection/renderer/gin/function_template_neva.h"
#include "third_party/blink/public/common/thread_safe_browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace injections {

gin::WrapperInfo BrowserShellInjection::kWrapperInfo = {
    gin::kEmbedderNativeGin
};

const char BrowserShellInjection::kShellWindowPropertyName[] = "shellWindow";
const char BrowserShellInjection::kCreateWindowMethodName[] = "createWindow";

void BrowserShellInjection::Install(blink::WebLocalFrame* frame) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Local<v8::Object> global = context->Global();
  v8::Context::Scope context_scope(context);
  v8::Local<v8::Value> shell_value =
      global->Get(context, gin::StringToV8(isolate, "shell"))
          .ToLocalChecked();

  if (!shell_value.IsEmpty() && shell_value->IsObject())
    return;

  gin::Handle<BrowserShellInjection> shell =
      gin::CreateHandle(isolate, new BrowserShellInjection(isolate, global));
  global
      ->Set(isolate->GetCurrentContext(), gin::StringToV8(isolate, "shell"),
            shell.ToV8())
      .Check();
}

void BrowserShellInjection::Uninstall(blink::WebLocalFrame* frame) {
  NOTIMPLEMENTED();
}

BrowserShellInjection::BrowserShellInjection(v8::Isolate* isolate,
                                             v8::Local<v8::Object> global) {
  auto context = isolate->GetCurrentContext();

  blink::Platform::Current()->GetBrowserInterfaceBroker()->GetInterface(
      remote_.BindNewPipeAndPassReceiver());

  // PageView Constructor
  v8::Local<v8::FunctionTemplate> page_view_templ =
      gin::CreateConstructorTemplate(
          isolate,
          base::BindRepeating(&BrowserShellPageView::ConstructorCallback,
                              base::Unretained(&remote_)));
  global
      ->Set(context, gin::StringToSymbol(isolate, "PageView"),
            page_view_templ->GetFunction(context).ToLocalChecked())
      .Check();

  // PageContents Constructor
  v8::Local<v8::FunctionTemplate> page_contents_templ =
      gin::CreateConstructorTemplate(
          isolate,
          base::BindRepeating(&BrowserShellPageContents::ConstructorCallback,
                              base::Unretained(&remote_)));

  global
      ->Set(context,
            gin::StringToSymbol(isolate, "PageContents"),
            page_contents_templ->GetFunction(context).ToLocalChecked())
      .Check();

  // Bind Shell Window
  mojo::Remote<browser_shell::mojom::ShellWindow> window_remote;
  auto window_receiver = window_remote.BindNewPipeAndPassReceiver();
  auto* shell_window =
      new injections::BrowserShellWindow(isolate, std::move(window_remote));
  remote_->BindShellWindow(std::move(window_receiver),
                           base::BindOnce(&BrowserShellWindow::Setup,
                                          base::Unretained(shell_window)));
  gin::Handle<injections::BrowserShellWindow> handle =
      gin::CreateHandle(isolate, shell_window);

  if (!handle.IsEmpty()) {
    shell_window_object_.Reset(isolate,
                               handle->GetWrapper(isolate).ToLocalChecked());
  }
}

BrowserShellInjection::~BrowserShellInjection() = default;

v8::Local<v8::Object> BrowserShellInjection::GetShellWindow(
    v8::Isolate* isolate) {
  return shell_window_object_.Get(isolate);
}

void BrowserShellInjection::CreateWindow() {
  NOTIMPLEMENTED();
}

// static
gin::ObjectTemplateBuilder BrowserShellInjection::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<BrowserShellInjection>::GetObjectTemplateBuilder(
             isolate)
      .SetProperty(kShellWindowPropertyName,
                   &BrowserShellInjection::GetShellWindow)
      .SetMethod(kCreateWindowMethodName, &BrowserShellInjection::CreateWindow);
}

}  // namespace injections
