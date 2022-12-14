// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/flatland/flatland_sysmem_native_pixmap.h"

#include "base/fuchsia/fuchsia_logging.h"
#include "base/logging.h"
#include "ui/gfx/geometry/rect_f.h"

namespace ui {

FlatlandSysmemNativePixmap::FlatlandSysmemNativePixmap(
    scoped_refptr<FlatlandSysmemBufferCollection> collection,
    gfx::NativePixmapHandle handle)
    : collection_(collection), handle_(std::move(handle)) {}

FlatlandSysmemNativePixmap::~FlatlandSysmemNativePixmap() = default;

bool FlatlandSysmemNativePixmap::AreDmaBufFdsValid() const {
  return false;
}

int FlatlandSysmemNativePixmap::GetDmaBufFd(size_t plane) const {
  NOTREACHED();
  return -1;
}

uint32_t FlatlandSysmemNativePixmap::GetDmaBufPitch(size_t plane) const {
  NOTREACHED();
  return 0u;
}

size_t FlatlandSysmemNativePixmap::GetDmaBufOffset(size_t plane) const {
  NOTREACHED();
  return 0u;
}

size_t FlatlandSysmemNativePixmap::GetDmaBufPlaneSize(size_t plane) const {
  NOTREACHED();
  return 0;
}

size_t FlatlandSysmemNativePixmap::GetNumberOfPlanes() const {
  NOTREACHED();
  return 0;
}

uint64_t FlatlandSysmemNativePixmap::GetBufferFormatModifier() const {
  NOTREACHED();
  return 0;
}

gfx::BufferFormat FlatlandSysmemNativePixmap::GetBufferFormat() const {
  return collection_->format();
}

gfx::Size FlatlandSysmemNativePixmap::GetBufferSize() const {
  return collection_->size();
}

uint32_t FlatlandSysmemNativePixmap::GetUniqueId() const {
  return 0;
}

bool FlatlandSysmemNativePixmap::ScheduleOverlayPlane(
    gfx::AcceleratedWidget widget,
    int plane_z_order,
    gfx::OverlayTransform plane_transform,
    const gfx::Rect& display_bounds,
    const gfx::RectF& crop_rect,
    bool enable_blend,
    const gfx::Rect& damage_rect,
    std::vector<gfx::GpuFence> acquire_fences,
    std::vector<gfx::GpuFence> release_fences) {
  return false;
}

gfx::NativePixmapHandle FlatlandSysmemNativePixmap::ExportHandle() {
  return gfx::CloneHandleForIPC(handle_);
}

bool FlatlandSysmemNativePixmap::SupportsOverlayPlane(
    gfx::AcceleratedWidget widget) const {
  return false;
}

}  // namespace ui
