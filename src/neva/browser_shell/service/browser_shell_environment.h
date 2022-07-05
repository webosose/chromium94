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

#ifndef NEVA_BROWSER_SHELL_SERVICE_BROWSER_SHELL_ENVIRONMENT_H_
#define NEVA_BROWSER_SHELL_SERVICE_BROWSER_SHELL_ENVIRONMENT_H_

#include <stdint.h>
#include <map>
#include <memory>

namespace neva_app_runtime {
class PageContents;
class PageView;
}  // namespace neva_app_runtime

namespace browser_shell {

class ShellEnvironment {
 public:
  using PageViewPtr = std::unique_ptr<neva_app_runtime::PageView>;
  using PageContentsPtr = std::unique_ptr<neva_app_runtime::PageContents>;

  ShellEnvironment();
  ShellEnvironment(const ShellEnvironment&) = delete;
  ShellEnvironment& operator=(const ShellEnvironment&) = delete;
  ~ShellEnvironment();

  uint64_t GetNextID();
  uint64_t GetNextIDFor(neva_app_runtime::PageView* ptr);
  uint64_t GetNextIDFor(neva_app_runtime::PageContents* ptr);

  uint64_t GetID(neva_app_runtime::PageView* ptr) const;
  uint64_t GetID(neva_app_runtime::PageContents* ptr) const;
  neva_app_runtime::PageView* GetViewPtr(uint64_t id) const;
  neva_app_runtime::PageContents* GetContentsPtr(uint64_t id) const;

  void Remove(neva_app_runtime::PageView* ptr);
  void Remove(neva_app_runtime::PageContents* ptr);
  void RemoveView(uint64_t id);
  void RemoveContents(uint64_t id);

  PageViewPtr SaveDetachedView(PageViewPtr page_view);
  PageContentsPtr SaveDetachedContents(PageContentsPtr page_contents);

  PageViewPtr ReleaseDetachedView(neva_app_runtime::PageView* ptr);
  PageContentsPtr ReleaseDetachedContents(neva_app_runtime::PageContents* ptr);

 private:
  uint64_t id_counter_ = 0;
  std::map<uint64_t, neva_app_runtime::PageView*> id_to_view_ptr_;
  std::map<uint64_t, neva_app_runtime::PageContents*> id_to_contents_ptr_;
  std::map<neva_app_runtime::PageView*, uint64_t> view_ptr_to_id_;
  std::map<neva_app_runtime::PageContents*, uint64_t> contents_ptr_to_id_;

  std::map<neva_app_runtime::PageView*, PageViewPtr> detached_views_;
  std::map<neva_app_runtime::PageContents*, PageContentsPtr> detached_contents_;
};

}  // namespace browser_shell

#endif  // NEVA_BROWSER_SHELL_SERVICE_BROWSER_SHELL_ENVIRONMENT_H_
