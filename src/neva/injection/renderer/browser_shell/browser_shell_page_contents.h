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
#ifndef NEVA_INJECTION_RENDERER_BROWSER_SHELL_BROWSER_SHELL_PAGE_CONTENTS_H_
#define NEVA_INJECTION_RENDERER_BROWSER_SHELL_BROWSER_SHELL_PAGE_CONTENTS_H_

#include <string>

#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "neva/browser_shell/service/public/mojom/browser_shell_page_contents.mojom.h"
#include "neva/browser_shell/service/public/mojom/browser_shell_service.mojom.h"
#include "v8/include/v8.h"

namespace blink {
class WebLocalFrame;
}

namespace gin {
class Arguments;
}

namespace injections {

class BrowserShellPageContents
    : public gin::Wrappable<BrowserShellPageContents>,
      public browser_shell::mojom::PageContentsClient {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static char kOnMethodName[];
  static char kLoadURLMethodName[];

  static void ConstructorCallback(
      mojo::Remote<browser_shell::mojom::ShellService>* shell_service,
      gin::Arguments* args);

  BrowserShellPageContents(v8::Isolate* isolate,
                           mojo::Remote<browser_shell::mojom::PageContents>,
                           bool is_main_contents = false);
  BrowserShellPageContents(const BrowserShellPageContents&) = delete;
  BrowserShellPageContents& operator=(const BrowserShellPageContents&) = delete;
  ~BrowserShellPageContents() override;

  uint64_t GetID();
  void Setup(
      uint64_t id,
      mojo::PendingAssociatedReceiver<browser_shell::mojom::PageContentsClient>
          receiver);
  void OnLoadURL(const std::string& url);

  void Updated(browser_shell::mojom::PageContentsStatePtr state) override;
  void DidLoad(const std::string& url) override;

 private:
  void SetEventHandler(gin::Arguments* args);
  void LoadURL(std::string url);

  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  uint64_t id_ = 0;
  bool is_main_contents_;
  mojo::Remote<browser_shell::mojom::PageContents> remote_;
  mojo::AssociatedReceiver<browser_shell::mojom::PageContentsClient>
      client_receiver_;
};

}  // namespace injections

#endif  // NEVA_INJECTION_RENDERER_BROWSER_SHELL_BROWSER_SHELL_PAGE_CONTENTS_H_
