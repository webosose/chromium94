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

#ifndef NEVA_APP_RUNTIME_APP_APP_RUNTIME_SHELL_WINDOW_H_
#define NEVA_APP_RUNTIME_APP_APP_RUNTIME_SHELL_WINDOW_H_

#include <memory>
#include <string>

#include "neva/app_runtime/app/app_runtime_shell_window_delegate.h"
#include "ui/views/widget/widget_delegate.h"

namespace neva_app_runtime {

class PageView;

class ShellWindow : public views::WidgetDelegateView {
 public:
  explicit ShellWindow(ShellWindowDelegate* delegate = nullptr);
  ShellWindow(const ShellWindow&) = delete;
  ShellWindow& operator=(const ShellWindow&) = delete;
  ~ShellWindow() override;

  PageView* GetPageView() const;
  std::unique_ptr<PageView> SetPageView(std::unique_ptr<PageView> page_view);
  std::unique_ptr<PageView> RemovePageView();

  void SetWindowTitle(std::u16string title);

  // views::WidgetDelegateView
  std::u16string GetWindowTitle() const override;
  void WindowClosing() override;

 private:
  void Init();
  std::u16string title_;
  ShellWindowDelegate* delegate_;
  ShellWindowDelegate stub_delegate_;
  std::unique_ptr<PageView> page_view_;
};

}  // namespace neva_app_runtime

#endif  // NEVA_APP_RUNTIME_APP_APP_RUNTIME_WINDOW_H_
