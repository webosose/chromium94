// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/gpu/webgpu_image_bitmap_handler.h"

#include "gpu/command_buffer/client/shared_image_interface.h"
#include "gpu/command_buffer/client/webgpu_interface.h"
#include "gpu/command_buffer/common/shared_image_usage.h"
#include "third_party/blink/renderer/platform/graphics/static_bitmap_image.h"

namespace blink {

namespace {
static constexpr uint64_t kDawnBytesPerRowAlignmentBits = 8;

// Calculate bytes per row for T2B/B2T copy
// TODO(shaobo.yan@intel.com): Using Dawn's constants once they are exposed
uint64_t AlignWebGPUBytesPerRow(uint64_t bytesPerRow) {
  return (((bytesPerRow - 1) >> kDawnBytesPerRowAlignmentBits) + 1)
         << kDawnBytesPerRowAlignmentBits;
}

SkColorType DawnColorTypeToSkColorType(WGPUTextureFormat dawn_format) {
  switch (dawn_format) {
    case WGPUTextureFormat_RGBA8Unorm:
    // According to WebGPU spec, format with -srgb suffix will do color
    // space conversion when reading and writing in shader. In this uploading
    // path, we should keep the conversion happening in canvas color space and
    // leave the srgb color space conversion to the GPU.
    case WGPUTextureFormat_RGBA8UnormSrgb:
      return SkColorType::kRGBA_8888_SkColorType;
    case WGPUTextureFormat_BGRA8Unorm:
    case WGPUTextureFormat_BGRA8UnormSrgb:
      return SkColorType::kBGRA_8888_SkColorType;
    case WGPUTextureFormat_RGB10A2Unorm:
      return SkColorType::kRGBA_1010102_SkColorType;
    case WGPUTextureFormat_RGBA16Float:
      return SkColorType::kRGBA_F16_SkColorType;
    case WGPUTextureFormat_RGBA32Float:
      return SkColorType::kRGBA_F32_SkColorType;
    case WGPUTextureFormat_RG8Unorm:
      return SkColorType::kR8G8_unorm_SkColorType;
    case WGPUTextureFormat_RG16Float:
      return SkColorType::kR16G16_float_SkColorType;
    default:
      return SkColorType::kUnknown_SkColorType;
  }
}

}  // anonymous namespace

WebGPUImageUploadSizeInfo ComputeImageBitmapWebGPUUploadSizeInfo(
    const IntRect& rect,
    const WGPUTextureFormat& destination_format) {
  WebGPUImageUploadSizeInfo info;

  uint64_t bytes_per_pixel = DawnTextureFormatBytesPerPixel(destination_format);
  DCHECK_NE(bytes_per_pixel, 0u);

  uint64_t bytes_per_row =
      AlignWebGPUBytesPerRow(rect.Width() * bytes_per_pixel);

  // Currently, bytes per row for buffer copy view in WebGPU is an uint32_t type
  // value and the maximum value is std::numeric_limits<uint32_t>::max().
  DCHECK(bytes_per_row <= std::numeric_limits<uint32_t>::max());

  info.wgpu_bytes_per_row = static_cast<uint32_t>(bytes_per_row);
  info.size_in_bytes = bytes_per_row * rect.Height();

  return info;
}

bool CopyBytesFromImageBitmapForWebGPU(
    scoped_refptr<StaticBitmapImage> image,
    base::span<uint8_t> dst,
    const IntRect& rect,
    const WGPUTextureFormat destination_format,
    bool premultipliedAlpha,
    bool flipY) {
  DCHECK(image);
  DCHECK_GT(dst.size(), static_cast<size_t>(0));
  DCHECK(image->width() - rect.X() >= rect.Width());
  DCHECK(image->height() - rect.Y() >= rect.Height());
  DCHECK(rect.Width());
  DCHECK(rect.Height());

  WebGPUImageUploadSizeInfo wgpu_info =
      ComputeImageBitmapWebGPUUploadSizeInfo(rect, destination_format);
  DCHECK_EQ(static_cast<uint64_t>(dst.size()), wgpu_info.size_in_bytes);

  // Prepare extract data from SkImage.
  SkColorType sk_color_type = DawnColorTypeToSkColorType(destination_format);
  if (sk_color_type == kUnknown_SkColorType) {
    return false;
  }
  PaintImage paint_image = image->PaintImageForCurrentFrame();

  // Read pixel request dst info.
  // TODO(crbug.com/1217153): Convert to user-provided color space.
  SkImageInfo info = SkImageInfo::Make(
      rect.Width(), rect.Height(), sk_color_type,
      premultipliedAlpha ? kPremul_SkAlphaType : kUnpremul_SkAlphaType,
      paint_image.GetSkImageInfo().refColorSpace());

  if (!flipY) {
    return paint_image.readPixels(
        info, dst.data(), wgpu_info.wgpu_bytes_per_row, rect.X(), rect.Y());
  } else {
    // Do flipY for the bottom left image.
    std::vector<uint8_t> flipped;
    flipped.resize(wgpu_info.wgpu_bytes_per_row * rect.Height());
    if (!paint_image.readPixels(info, flipped.data(),
                                wgpu_info.wgpu_bytes_per_row, rect.X(),
                                rect.Y())) {
      return false;
    }
    for (int i = 0; i < rect.Height(); ++i) {
      memcpy(
          dst.data() + (rect.Height() - 1 - i) * wgpu_info.wgpu_bytes_per_row,
          flipped.data() + i * wgpu_info.wgpu_bytes_per_row,
          wgpu_info.wgpu_bytes_per_row);
    }
  }

  return true;
}

uint64_t DawnTextureFormatBytesPerPixel(const WGPUTextureFormat color_type) {
  switch (color_type) {
    case WGPUTextureFormat_RG8Unorm:
      return 2;
    case WGPUTextureFormat_RGBA8Unorm:
    case WGPUTextureFormat_RGBA8UnormSrgb:
    case WGPUTextureFormat_BGRA8Unorm:
    case WGPUTextureFormat_BGRA8UnormSrgb:
    case WGPUTextureFormat_RGB10A2Unorm:
    case WGPUTextureFormat_RG16Float:
      return 4;
    case WGPUTextureFormat_RGBA16Float:
      return 8;
    case WGPUTextureFormat_RGBA32Float:
      return 16;
    default:
      NOTREACHED();
      return 0;
  }
}

}  // namespace blink
