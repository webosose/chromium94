// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/render/cpdf_type3cache.h"

#include <math.h>
#include <stdarg.h>

#include <memory>
#include <utility>

#include "core/fpdfapi/font/cpdf_type3char.h"
#include "core/fpdfapi/font/cpdf_type3font.h"
#include "core/fpdfapi/render/cpdf_type3glyphmap.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxge/cfx_glyphbitmap.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/fx_dib.h"

namespace {

struct UniqueKeyGen {
  void Generate(int count, ...);

  int m_KeyLen;
  char m_Key[128];
};

void UniqueKeyGen::Generate(int count, ...) {
  va_list argList;
  va_start(argList, count);
  for (int i = 0; i < count; i++) {
    int p = va_arg(argList, int);
    (reinterpret_cast<uint32_t*>(m_Key))[i] = p;
  }
  va_end(argList);
  m_KeyLen = count * sizeof(uint32_t);
}

bool IsScanLine1bpp(const uint8_t* pBuf, int width) {
  int size = width / 8;
  for (int i = 0; i < size; i++) {
    if (pBuf[i])
      return true;
  }
  return (width % 8) && (pBuf[width / 8] & (0xff << (8 - width % 8)));
}

bool IsScanLine8bpp(const uint8_t* pBuf, int width) {
  for (int i = 0; i < width; i++) {
    if (pBuf[i] > 0x40)
      return true;
  }
  return false;
}

bool IsScanLineBpp(int bpp, const uint8_t* pBuf, int width) {
  if (bpp == 1)
    return IsScanLine1bpp(pBuf, width);
  if (bpp > 8)
    width *= bpp / 8;
  return IsScanLine8bpp(pBuf, width);
}

int DetectFirstScan(const RetainPtr<CFX_DIBitmap>& pBitmap) {
  const int height = pBitmap->GetHeight();
  const int pitch = pBitmap->GetPitch();
  const int width = pBitmap->GetWidth();
  const int bpp = pBitmap->GetBPP();
  const uint8_t* pBuf = pBitmap->GetBuffer();
  for (int line = 0; line < height; ++line) {
    if (IsScanLineBpp(bpp, pBuf + line * pitch, width))
      return line;
  }
  return -1;
}

int DetectLastScan(const RetainPtr<CFX_DIBitmap>& pBitmap) {
  const int height = pBitmap->GetHeight();
  const int pitch = pBitmap->GetPitch();
  const int bpp = pBitmap->GetBPP();
  const int width = pBitmap->GetWidth();
  const uint8_t* pBuf = pBitmap->GetBuffer();
  for (int line = height - 1; line >= 0; --line) {
    if (IsScanLineBpp(bpp, pBuf + line * pitch, width))
      return line;
  }
  return -1;
}

}  // namespace

CPDF_Type3Cache::CPDF_Type3Cache(CPDF_Type3Font* pFont) : m_pFont(pFont) {}

CPDF_Type3Cache::~CPDF_Type3Cache() = default;

const CFX_GlyphBitmap* CPDF_Type3Cache::LoadGlyph(uint32_t charcode,
                                                  const CFX_Matrix& mtMatrix) {
  UniqueKeyGen keygen;
  keygen.Generate(
      4, FXSYS_roundf(mtMatrix.a * 10000), FXSYS_roundf(mtMatrix.b * 10000),
      FXSYS_roundf(mtMatrix.c * 10000), FXSYS_roundf(mtMatrix.d * 10000));
  ByteString FaceGlyphsKey(keygen.m_Key, keygen.m_KeyLen);
  CPDF_Type3GlyphMap* pSizeCache;
  auto it = m_SizeMap.find(FaceGlyphsKey);
  if (it == m_SizeMap.end()) {
    auto pNew = std::make_unique<CPDF_Type3GlyphMap>();
    pSizeCache = pNew.get();
    m_SizeMap[FaceGlyphsKey] = std::move(pNew);
  } else {
    pSizeCache = it->second.get();
  }
  const CFX_GlyphBitmap* pExisting = pSizeCache->GetBitmap(charcode);
  if (pExisting)
    return pExisting;

  std::unique_ptr<CFX_GlyphBitmap> pNewBitmap =
      RenderGlyph(pSizeCache, charcode, mtMatrix);
  CFX_GlyphBitmap* pGlyphBitmap = pNewBitmap.get();
  pSizeCache->SetBitmap(charcode, std::move(pNewBitmap));
  return pGlyphBitmap;
}

std::unique_ptr<CFX_GlyphBitmap> CPDF_Type3Cache::RenderGlyph(
    CPDF_Type3GlyphMap* pSize,
    uint32_t charcode,
    const CFX_Matrix& mtMatrix) {
  const CPDF_Type3Char* pChar = m_pFont->LoadChar(charcode);
  if (!pChar || !pChar->GetBitmap())
    return nullptr;

  CFX_Matrix text_matrix(mtMatrix.a, mtMatrix.b, mtMatrix.c, mtMatrix.d, 0, 0);
  CFX_Matrix image_matrix = pChar->matrix() * text_matrix;

  RetainPtr<CFX_DIBitmap> pBitmap = pChar->GetBitmap();
  RetainPtr<CFX_DIBitmap> pResBitmap;
  int left = 0;
  int top = 0;
  if (fabs(image_matrix.b) < fabs(image_matrix.a) / 100 &&
      fabs(image_matrix.c) < fabs(image_matrix.d) / 100) {
    int top_line = DetectFirstScan(pBitmap);
    int bottom_line = DetectLastScan(pBitmap);
    if (top_line == 0 && bottom_line == pBitmap->GetHeight() - 1) {
      float top_y = image_matrix.d + image_matrix.f;
      float bottom_y = image_matrix.f;
      bool bFlipped = top_y > bottom_y;
      if (bFlipped)
        std::swap(top_y, bottom_y);
      std::tie(top_line, bottom_line) = pSize->AdjustBlue(top_y, bottom_y);
      FX_SAFE_INT32 safe_height = bFlipped ? top_line : bottom_line;
      safe_height -= bFlipped ? bottom_line : top_line;
      if (!safe_height.IsValid())
        return nullptr;

      pResBitmap = pBitmap->StretchTo(static_cast<int>(image_matrix.a),
                                      safe_height.ValueOrDie(),
                                      FXDIB_ResampleOptions(), nullptr);
      top = top_line;
      if (image_matrix.a < 0)
        left = FXSYS_roundf(image_matrix.e + image_matrix.a);
      else
        left = FXSYS_roundf(image_matrix.e);
    }
  }
  if (!pResBitmap)
    pResBitmap = pBitmap->TransformTo(image_matrix, &left, &top);
  if (!pResBitmap)
    return nullptr;

  auto pGlyph = std::make_unique<CFX_GlyphBitmap>(left, -top);
  pGlyph->GetBitmap()->TakeOver(std::move(pResBitmap));
  return pGlyph;
}
