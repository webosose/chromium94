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
#ifndef NEVA_INJECTION_RENDERER_BROWSER_SHELL_BROWSER_SHELL_PAGE_WINDOW_H_
#define NEVA_INJECTION_RENDERER_BROWSER_SHELL_BROWSER_SHELL_PAGE_WINDOW_H_

#include <string>

#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "neva/browser_shell/service/public/mojom/browser_shell_window.mojom.h"
#include "v8/include/v8.h"

namespace gin {
class Arguments;
}

namespace injections {

class BrowserShellPageView;

class BrowserShellWindow
    : public gin::Wrappable<BrowserShellWindow>,
      public browser_shell::mojom::ShellWindowClient {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static char kPageViewPropertyName[];
  static char kOnMethodName[];
  static char kSetVisibleMethodName[];
  static char kIsVisibleMethodName[];

  BrowserShellWindow(v8::Isolate* isolate,
                     mojo::Remote<browser_shell::mojom::ShellWindow> remote);
  BrowserShellWindow(const BrowserShellWindow&) = delete;
  BrowserShellWindow& operator=(const BrowserShellWindow&) = delete;
  ~BrowserShellWindow() override;

  void Setup(
      const std::string& name,
      mojo::PendingAssociatedReceiver<browser_shell::mojom::ShellWindowClient>
          receiver);

  void Updated() override;

 private:
  std::string& GetName();

  v8::Local<v8::Object> GetPageView(v8::Isolate* isolate);
  void SetVisible(bool visible);
  bool IsVisible() const;
  void SetEventHandler(gin::Arguments* args);

  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  std::string name_;
  mojo::Remote<browser_shell::mojom::ShellWindow> remote_;
  mojo::AssociatedReceiver<browser_shell::mojom::ShellWindowClient>
      client_receiver_;
  v8::Global<v8::Object> page_view_object_;
};

}  // namespace injections

#endif  // NEVA_INJECTION_RENDERER_BROWSER_SHELL_BROWSER_SHELL_PAGE_WINDOW_H_
