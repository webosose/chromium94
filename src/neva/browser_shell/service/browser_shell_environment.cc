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

#include "neva/browser_shell/service/browser_shell_environment.h"

#include "base/macros.h"
#include "neva/app_runtime/app/app_runtime_page_contents.h"
#include "neva/app_runtime/app/app_runtime_page_view.h"

namespace browser_shell {

ShellEnvironment::ShellEnvironment() = default;
ShellEnvironment::~ShellEnvironment() = default;

uint64_t ShellEnvironment::GetNextID() {
  return ++id_counter_;
}

uint64_t ShellEnvironment::GetNextIDFor(neva_app_runtime::PageView* ptr) {
  const uint64_t id = ++id_counter_;
  id_to_view_ptr_[id] = ptr;
  view_ptr_to_id_[ptr] = id;
  return id;
}

uint64_t ShellEnvironment::GetNextIDFor(neva_app_runtime::PageContents* ptr) {
  const uint64_t id = ++id_counter_;
  id_to_contents_ptr_[id] = ptr;
  contents_ptr_to_id_[ptr] = id;
  return id;
}

uint64_t ShellEnvironment::GetID(neva_app_runtime::PageView* ptr) const {
  auto it = view_ptr_to_id_.find(ptr);
  if (it == view_ptr_to_id_.cend())
    return 0;
  return it->second;
}

uint64_t ShellEnvironment::GetID(neva_app_runtime::PageContents* ptr) const {
  auto it = contents_ptr_to_id_.find(ptr);
  if (it == contents_ptr_to_id_.cend())
    return 0;
  return it->second;
}

neva_app_runtime::PageView* ShellEnvironment::GetViewPtr(uint64_t id) const {
  auto it = id_to_view_ptr_.find(id);
  if (it == id_to_view_ptr_.cend())
    return nullptr;
  return it->second;
}

neva_app_runtime::PageContents* ShellEnvironment::GetContentsPtr(
    uint64_t id) const {
  auto it = id_to_contents_ptr_.find(id);
  if (it == id_to_contents_ptr_.cend())
    return nullptr;
  return it->second;
}

void ShellEnvironment::Remove(neva_app_runtime::PageView* ptr) {
  auto it = view_ptr_to_id_.find(ptr);
  if (it != view_ptr_to_id_.end()) {
    ignore_result(detached_views_.erase(ptr));
    ignore_result(id_to_view_ptr_.erase(it->second));
    ignore_result(view_ptr_to_id_.erase(it));
  }
}

void ShellEnvironment::Remove(neva_app_runtime::PageContents* ptr) {
  ignore_result(detached_contents_.erase(ptr));
  auto it = contents_ptr_to_id_.find(ptr);
  if (it != contents_ptr_to_id_.end()) {
    ignore_result(detached_contents_.erase(ptr));
    ignore_result(id_to_contents_ptr_.erase(it->second));
    ignore_result(contents_ptr_to_id_.erase(it));
  }
}

void ShellEnvironment::RemoveView(uint64_t id) {
  auto it = id_to_view_ptr_.find(id);
  if (it != id_to_view_ptr_.end()) {
    ignore_result(detached_views_.erase(it->second));
    ignore_result(view_ptr_to_id_.erase(it->second));
    ignore_result(id_to_view_ptr_.erase(it));
  }
}

void ShellEnvironment::RemoveContents(uint64_t id) {
  auto it = id_to_contents_ptr_.find(id);
  if (it != id_to_contents_ptr_.end()) {
    ignore_result(detached_contents_.erase(it->second));
    ignore_result(contents_ptr_to_id_.erase(it->second));
    ignore_result(id_to_contents_ptr_.erase(it));
  }
}

ShellEnvironment::PageViewPtr ShellEnvironment::SaveDetachedView(
    PageViewPtr page_view) {
  detached_views_[page_view.get()] = std::move(page_view);
  return PageViewPtr();
}

ShellEnvironment::PageContentsPtr ShellEnvironment::SaveDetachedContents(
    PageContentsPtr page_contents) {
  detached_contents_[page_contents.get()] = std::move(page_contents);
  return PageContentsPtr();
}

ShellEnvironment::PageViewPtr ShellEnvironment::ReleaseDetachedView(
    neva_app_runtime::PageView* ptr) {
  auto it = detached_views_.find(ptr);
  if (it != detached_views_.end()) {
    PageViewPtr ret(std::move(it->second));
    detached_views_.erase(it);
    return ret;
  }
  return PageViewPtr();
}

ShellEnvironment::PageContentsPtr ShellEnvironment::ReleaseDetachedContents(
    neva_app_runtime::PageContents* ptr) {
  auto it = detached_contents_.find(ptr);
  if (it != detached_contents_.end()) {
    PageContentsPtr ret(std::move(it->second));
    detached_contents_.erase(it);
    return ret;
  }
  return PageContentsPtr();
}

}  // namespace browser_shell
