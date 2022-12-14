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

#ifndef UI_OZONE_PLATFORM_WAYLAND_EXTENSIONS_AGL_HOST_AGL_SHELL_WRAPPER_H_
#define UI_OZONE_PLATFORM_WAYLAND_EXTENSIONS_AGL_HOST_AGL_SHELL_WRAPPER_H_

#include <string>

#include "ui/ozone/platform/wayland/extensions/agl/common/wayland_object_agl.h"

namespace ui {

class WaylandConnection;
class WaylandWindow;

class AglShellWrapper {
 public:
  AglShellWrapper(agl_shell* agl_shell, WaylandConnection* wayland_connection);
  AglShellWrapper(const AglShellWrapper&) = delete;
  AglShellWrapper& operator=(const AglShellWrapper&) = delete;
  ~AglShellWrapper();

  void SetAglActivateApp(const std::string& app_id);
  void SetAglPanel(WaylandWindow* window, uint32_t edge);
  void SetAglBackground(WaylandWindow* window);
  void SetAglReady();

 private:
  wl::Object<agl_shell> agl_shell_;

  WaylandConnection* connection_;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_EXTENSIONS_AGL_HOST_AGL_SHELL_WRAPPER_H_