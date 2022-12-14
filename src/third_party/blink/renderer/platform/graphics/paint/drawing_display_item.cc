// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/drawing_display_item.h"

#include "base/logging.h"
#include "cc/paint/display_item_list.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/logging_canvas.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_canvas.h"
#include "third_party/blink/renderer/platform/wtf/size_assertions.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkData.h"

namespace blink {

static SkBitmap RecordToBitmap(sk_sp<const PaintRecord> record,
                               const IntRect& bounds) {
  SkBitmap bitmap;
  bitmap.allocPixels(
      SkImageInfo::MakeN32Premul(bounds.Width(), bounds.Height()));
  SkiaPaintCanvas canvas(bitmap);
  canvas.clear(SK_ColorTRANSPARENT);
  canvas.translate(-bounds.X(), -bounds.Y());
  canvas.drawPicture(std::move(record));
  return bitmap;
}

static bool BitmapsEqual(sk_sp<const PaintRecord> record1,
                         sk_sp<const PaintRecord> record2,
                         const IntRect& bounds) {
  SkBitmap bitmap1 = RecordToBitmap(record1, bounds);
  SkBitmap bitmap2 = RecordToBitmap(record2, bounds);
  int mismatch_count = 0;
  constexpr int kMaxMismatches = 10;
  for (int y = 0; y < bounds.Height(); ++y) {
    for (int x = 0; x < bounds.Width(); ++x) {
      SkColor pixel1 = bitmap1.getColor(x, y);
      SkColor pixel2 = bitmap2.getColor(x, y);
      if (pixel1 != pixel2) {
        if (!RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled())
          return false;
        LOG(ERROR) << "x=" << x << " y=" << y << " " << std::hex << pixel1
                   << " vs " << std::hex << pixel2;
        if (++mismatch_count >= kMaxMismatches)
          return false;
      }
    }
  }
  return !mismatch_count;
}

bool DrawingDisplayItem::EqualsForUnderInvalidationImpl(
    const DrawingDisplayItem& other) const {
  DCHECK(RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled());

  const auto& record = GetPaintRecord();
  const auto& other_record = other.GetPaintRecord();
  if (!record && !other_record)
    return true;
  if (!record || !other_record)
    return false;

  auto bounds = VisualRect();
  const auto& other_bounds = other.VisualRect();
  if (bounds != other_bounds)
    return false;

  if (*record == *other_record)
    return true;

  // Sometimes the client may produce different records for the same visual
  // result, which should be treated as equal.
  // Limit the bounds to prevent OOM.
  bounds.Intersect(IntRect(bounds.X(), bounds.Y(), 6000, 6000));
  return BitmapsEqual(std::move(record), std::move(other_record), bounds);
}

SkColor DrawingDisplayItem::BackgroundColor(float& area) const {
  DCHECK(!IsTombstone());

  if (GetType() != DisplayItem::kBoxDecorationBackground &&
      GetType() != DisplayItem::kDocumentBackground &&
      GetType() != DisplayItem::kDocumentRootBackdrop)
    return SK_ColorTRANSPARENT;

  if (!record_)
    return SK_ColorTRANSPARENT;

  for (cc::PaintOpBuffer::Iterator it(record_.get()); it; ++it) {
    const auto* op = *it;
    if (!op->IsPaintOpWithFlags())
      continue;
    const auto& flags = static_cast<const cc::PaintOpWithFlags*>(op)->flags;
    // Skip op with looper or shader which may modify the color.
    if (flags.getLooper() || flags.getShader() ||
        flags.getStyle() != cc::PaintFlags::kFill_Style) {
      continue;
    }
    SkRect item_rect;
    switch (op->GetType()) {
      case cc::PaintOpType::DrawRect:
        item_rect = static_cast<const cc::DrawRectOp*>(op)->rect;
        break;
      case cc::PaintOpType::DrawIRect:
        item_rect = SkRect::Make(static_cast<const cc::DrawIRectOp*>(op)->rect);
        break;
      case cc::PaintOpType::DrawRRect:
        item_rect = static_cast<const cc::DrawRRectOp*>(op)->rrect.rect();
        break;
      default:
        continue;
    }
    area = item_rect.width() * item_rect.height();
    return flags.getColor();
  }
  return SK_ColorTRANSPARENT;
}

IntRect DrawingDisplayItem::CalculateRectKnownToBeOpaque() const {
  IntRect rect = CalculateRectKnownToBeOpaqueForRecord(record_.get());
  if (rect.IsEmpty()) {
    SetOpaqueness(Opaqueness::kNone);
  } else if (rect == VisualRect()) {
    SetOpaqueness(Opaqueness::kFull);
  } else {
    DCHECK(VisualRect().Contains(rect));
    DCHECK_EQ(GetOpaqueness(), Opaqueness::kOther);
  }
  return rect;
}

// This is not a PaintRecord method because it's not a general opaqueness
// detection algorithm (which might be more complex and slower), but works well
// and fast for most blink painted results.
IntRect DrawingDisplayItem::CalculateRectKnownToBeOpaqueForRecord(
    const PaintRecord* record) const {
  if (!record)
    return IntRect();

  // This limit keeps the algorithm fast, while allowing check of enough paint
  // operations for most blink painted results.
  constexpr wtf_size_t kOpCountLimit = 8;
  IntRect opaque_rect;
  wtf_size_t op_count = 0;
  IntRect clip_rect = VisualRect();
  for (cc::PaintOpBuffer::Iterator it(record); it; ++it) {
    if (++op_count > kOpCountLimit)
      break;

    const auto* op = *it;
    // Deal with the common pattern of clipped bleed avoiding images like:
    // Save, ClipRect, Draw..., Restore.
    if (op->GetType() == cc::PaintOpType::Save)
      continue;
    if (op->GetType() == cc::PaintOpType::ClipRect) {
      clip_rect.Intersect(
          EnclosedIntRect(static_cast<const cc::ClipRectOp*>(op)->rect));
      continue;
    }

    if (!op->IsDrawOp())
      break;

    IntRect op_opaque_rect;
    if (op->GetType() == cc::PaintOpType::DrawRecord) {
      op_opaque_rect = CalculateRectKnownToBeOpaqueForRecord(
          static_cast<const cc::DrawRecordOp*>(op)->record.get());
    } else {
      if (!op->IsPaintOpWithFlags())
        continue;

      const auto& flags = static_cast<const cc::PaintOpWithFlags*>(op)->flags;
      if (flags.getStyle() != cc::PaintFlags::kFill_Style ||
          flags.getLooper() ||
          (flags.getBlendMode() != SkBlendMode::kSrc &&
           flags.getBlendMode() != SkBlendMode::kSrcOver) ||
          flags.getMaskFilter() || flags.getColorFilter() ||
          flags.getImageFilter() || flags.getAlpha() != SK_AlphaOPAQUE ||
          (flags.getShader() && !flags.getShader()->IsOpaque()))
        continue;

      switch (op->GetType()) {
        case cc::PaintOpType::DrawRect:
          op_opaque_rect =
              EnclosedIntRect(static_cast<const cc::DrawRectOp*>(op)->rect);
          break;
        case cc::PaintOpType::DrawIRect:
          op_opaque_rect =
              IntRect(static_cast<const cc::DrawIRectOp*>(op)->rect);
          break;
        case cc::PaintOpType::DrawImage: {
          const auto* draw_image_op = static_cast<const cc::DrawImageOp*>(op);
          const auto& image = draw_image_op->image;
          if (!image.IsOpaque())
            continue;
          op_opaque_rect = IntRect(draw_image_op->left, draw_image_op->top,
                                   image.width(), image.height());
          break;
        }
        case cc::PaintOpType::DrawImageRect: {
          const auto* draw_image_rect_op =
              static_cast<const cc::DrawImageRectOp*>(op);
          const auto& image = draw_image_rect_op->image;
          DCHECK(SkRect::MakeWH(image.width(), image.height())
                     .contains(draw_image_rect_op->src));
          if (!image.IsOpaque())
            continue;
          op_opaque_rect = EnclosedIntRect(draw_image_rect_op->dst);
          break;
        }
        default:
          continue;
      }
    }

    opaque_rect = MaximumCoveredRect(opaque_rect, op_opaque_rect);
    opaque_rect.Intersect(clip_rect);
    if (opaque_rect == VisualRect())
      break;
  }
  DCHECK(VisualRect().Contains(opaque_rect) || opaque_rect.IsEmpty());
  return opaque_rect;
}

IntRect DrawingDisplayItem::TightenVisualRect(
    const IntRect& visual_rect,
    sk_sp<const PaintRecord>& record) {
  DCHECK(ShouldTightenVisualRect(record));

  const auto* op = record->GetFirstOp();
  if (!op->IsPaintOpWithFlags())
    return visual_rect;

  const auto& flags = static_cast<const cc::PaintOpWithFlags*>(op)->flags;
  // The following can cause the painted output to be outside the paint op rect.
  if (flags.getStyle() != cc::PaintFlags::kFill_Style || flags.getLooper() ||
      flags.getMaskFilter() || flags.getImageFilter() || flags.getShader()) {
    return visual_rect;
  }

  // TODO(pdr): Consider using |PaintOp::GetBounds| which is a more complete
  // implementation of the logic below.

  IntRect item_rect;
  switch (op->GetType()) {
    case cc::PaintOpType::DrawRect:
      item_rect =
          EnclosingIntRect(static_cast<const cc::DrawRectOp*>(op)->rect);
      break;
    case cc::PaintOpType::DrawIRect:
      item_rect = IntRect(static_cast<const cc::DrawIRectOp*>(op)->rect);
      break;
    case cc::PaintOpType::DrawRRect:
      item_rect = EnclosingIntRect(
          static_cast<const cc::DrawRRectOp*>(op)->rrect.rect());
      break;
    // TODO(pdr): Support image PaintOpTypes such as DrawImage{Rect}.
    // TODO(pdr): Consider checking PaintOpType::DrawTextBlob too.
    default:
      return visual_rect;
  }

  // TODO(pdr): Enable this DCHECK which enforces that the original visual rect
  // was correct and fully contains the recording.
  // DCHECK(visual_rect.Contains(item_rect));
  return item_rect;
}

bool DrawingDisplayItem::IsSolidColor() const {
  if (!record_)
    return false;

  // TODO(pdr): We could use SolidColorAnalyzer::DetermineIfSolidColor instead
  // of special-casing just single-op drawrect solid colors.
  if (record_->size() != 1)
    return false;

  const auto* op = record_->GetFirstOp();
  if (!op->IsPaintOpWithFlags())
    return false;

  const auto& flags = static_cast<const cc::PaintOpWithFlags*>(op)->flags;
  // The following can cause the painted output to be outside the paint op rect.
  if (flags.getStyle() != cc::PaintFlags::kFill_Style || flags.getLooper() ||
      flags.getMaskFilter() || flags.getImageFilter() || flags.getShader()) {
    return false;
  }

  FloatRect solid_color_rect;
  switch (op->GetType()) {
    case cc::PaintOpType::DrawRect:
      solid_color_rect =
          FloatRect(static_cast<const cc::DrawRectOp*>(op)->rect);
      break;
    case cc::PaintOpType::DrawIRect:
      solid_color_rect =
          FloatRect(IntRect(static_cast<const cc::DrawIRectOp*>(op)->rect));
      break;
    default:
      return false;
  }

  // The solid color must fully cover the visual rect.
  return solid_color_rect.Contains(VisualRect());
}

}  // namespace blink
