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

#ifndef NEVA_INJECTION_RENDERER_BROWSER_SHELL_BROWSER_SHELL_INJECTION_H_
#define NEVA_INJECTION_RENDERER_BROWSER_SHELL_BROWSER_SHELL_INJECTION_H_

#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "neva/browser_shell/service/public/mojom/browser_shell_service.mojom.h"
#include "v8/include/v8.h"

namespace blink {
class WebLocalFrame;
}

namespace injections {

class BrowserShellInjection : public gin::Wrappable<BrowserShellInjection> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static const char kShellWindowPropertyName[];
  static const char kCreateWindowMethodName[];

  static void Install(blink::WebLocalFrame* frame);
  static void Uninstall(blink::WebLocalFrame* frame);

  BrowserShellInjection(v8::Isolate* isolate, v8::Local<v8::Object> global);
  BrowserShellInjection(const BrowserShellInjection&) = delete;
  BrowserShellInjection& operator=(const BrowserShellInjection&) = delete;
  ~BrowserShellInjection() override;

  v8::Local<v8::Object> GetShellWindow(v8::Isolate* isolate);
  void CreateWindow();

 private:
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) final;

  mojo::Remote<browser_shell::mojom::ShellService> remote_;
  v8::Global<v8::Object> shell_window_object_;
};

}  // namespace injections

#endif  // NEVA_INJECTION_RENDERER_BROWSER_SHELL_BROWSER_SHELL_INJECTION_H_
