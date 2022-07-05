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

#include "neva/app_runtime/app/app_runtime_shell_window.h"

#include "neva/app_runtime/app/app_runtime_page_view.h"
#include "neva/app_runtime/app/app_runtime_shell_window_delegate.h"
#include "ui/views/layout/fill_layout.h"

namespace neva_app_runtime {

ShellWindow::ShellWindow(ShellWindowDelegate* delegate) :
    delegate_(delegate ? delegate : &stub_delegate_) {
  Init();
}

ShellWindow::~ShellWindow() {
  if (delegate_)
    delegate_->OnDestroying(this);
}

void ShellWindow::SetWindowTitle(std::u16string title) {
  title_ = std::move(title);
}

PageView* ShellWindow::GetPageView() const {
  return page_view_.get();
}

std::unique_ptr<PageView> ShellWindow::SetPageView(
    std::unique_ptr<PageView> page_view) {
  auto previous_page_view = std::move(page_view_);
  page_view_ = std::move(page_view);
  page_view_->SetParentShellWindow(this);

  RemoveAllChildViews();
  SetLayoutManager(std::make_unique<views::FillLayout>());
  AddChildView(page_view_->GetView());
  Layout();
  if (previous_page_view.get())
    previous_page_view->SetParentShellWindow(nullptr);
  return previous_page_view;
}

std::unique_ptr<PageView> ShellWindow::RemovePageView() {
  RemoveChildView(page_view_->GetView());
  page_view_->SetParentShellWindow(nullptr);
  return std::move(page_view_);
}

std::u16string ShellWindow::GetWindowTitle() const {
  return title_;
}

void ShellWindow::WindowClosing() {
  if (delegate_)
    delegate_->OnWindowClosing();
}

void ShellWindow::Init() {
  // widget will be removed by its NativeWidget, or when closing with CloseNow.
  views::Widget* widget = new views::Widget;
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_WINDOW);
  params.bounds = gfx::Rect(0, 0, 800, 600);
  // Since ShellWindow is WigetViewDelegate, it's set being owned by its widget.
  params.delegate = this;
  params.show_state = ui::SHOW_STATE_DEFAULT;
  widget->Init(std::move(params));
  widget->Show();
}

}  // namespace neva_app_runtime
