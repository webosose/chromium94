// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_HOST_ORG_KDE_KWIN_IDLE_H_
#define UI_OZONE_PLATFORM_WAYLAND_HOST_ORG_KDE_KWIN_IDLE_H_

#include <memory>

#include "base/time/time.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/ozone/platform/wayland/common/wayland_object.h"

namespace ui {

class WaylandConnection;

// Wraps the KDE Wayland user idle time manager, which is provided via
// org_kde_kwin_idle interface.
class OrgKdeKwinIdle : public wl::GlobalObjectRegistrar<OrgKdeKwinIdle> {
 public:
  static void Register(WaylandConnection* connection);
  static void Instantiate(WaylandConnection* connection,
                          wl_registry* registry,
                          uint32_t name,
                          uint32_t version);

  OrgKdeKwinIdle(org_kde_kwin_idle* idle, WaylandConnection* connection);
  OrgKdeKwinIdle(const OrgKdeKwinIdle&) = delete;
  OrgKdeKwinIdle& operator=(const OrgKdeKwinIdle&) = delete;
  ~OrgKdeKwinIdle();

  // Returns the idle time if querying it is possible, absl::nullopt otherwise.
  absl::optional<base::TimeDelta> GetIdleTime() const;

 private:
  class Timeout;

  // Wayland object wrapped by this class.
  wl::Object<org_kde_kwin_idle> idle_;
  // The actual idle timeout connection point.
  mutable std::unique_ptr<Timeout> idle_timeout_;

  WaylandConnection* const connection_;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_HOST_ORG_KDE_KWIN_IDLE_H_
