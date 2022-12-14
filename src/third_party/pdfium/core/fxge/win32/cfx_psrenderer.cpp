// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxge/win32/cfx_psrenderer.h"

#include <math.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <utility>

#include "core/fxcrt/maybe_owned.h"
#include "core/fxge/cfx_fillrenderoptions.h"
#include "core/fxge/cfx_fontcache.h"
#include "core/fxge/cfx_gemodule.h"
#include "core/fxge/cfx_glyphcache.h"
#include "core/fxge/cfx_path.h"
#include "core/fxge/cfx_renderdevice.h"
#include "core/fxge/dib/cfx_dibextractor.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/fx_dib.h"
#include "core/fxge/text_char_pos.h"
#include "core/fxge/win32/cpsoutput.h"

struct PSGlyph {
  UnownedPtr<CFX_Font> m_pFont;
  uint32_t m_GlyphIndex;
  bool m_bGlyphAdjust;
  float m_AdjustMatrix[4];
};

class CPSFont {
 public:
  int m_nGlyphs;
  PSGlyph m_Glyphs[256];
};

CFX_PSRenderer::CFX_PSRenderer(const EncoderIface* pEncoderIface)
    : m_pEncoderIface(pEncoderIface) {}

CFX_PSRenderer::~CFX_PSRenderer() = default;

void CFX_PSRenderer::Init(const RetainPtr<IFX_RetainableWriteStream>& pStream,
                          int pslevel,
                          int width,
                          int height) {
  m_PSLevel = pslevel;
  m_pStream = pStream;
  m_ClipBox.left = 0;
  m_ClipBox.top = 0;
  m_ClipBox.right = width;
  m_ClipBox.bottom = height;
}

void CFX_PSRenderer::StartRendering() {
  if (m_bInited)
    return;

  static const char kInitStr[] =
      "\nsave\n/im/initmatrix load def\n"
      "/n/newpath load def/m/moveto load def/l/lineto load def/c/curveto load "
      "def/h/closepath load def\n"
      "/f/fill load def/F/eofill load def/s/stroke load def/W/clip load "
      "def/W*/eoclip load def\n"
      "/rg/setrgbcolor load def/k/setcmykcolor load def\n"
      "/J/setlinecap load def/j/setlinejoin load def/w/setlinewidth load "
      "def/M/setmiterlimit load def/d/setdash load def\n"
      "/q/gsave load def/Q/grestore load def/iM/imagemask load def\n"
      "/Tj/show load def/Ff/findfont load def/Fs/scalefont load def/Sf/setfont "
      "load def\n"
      "/cm/concat load def/Cm/currentmatrix load def/mx/matrix load "
      "def/sm/setmatrix load def\n";
  WriteString(kInitStr);
  m_bInited = true;
}

void CFX_PSRenderer::EndRendering() {
  if (!m_bInited)
    return;

  WriteString("\nrestore\n");
  m_bInited = false;
}

void CFX_PSRenderer::SaveState() {
  StartRendering();
  WriteString("q\n");
  m_ClipBoxStack.push_back(m_ClipBox);
}

void CFX_PSRenderer::RestoreState(bool bKeepSaved) {
  StartRendering();
  WriteString("Q\n");
  if (bKeepSaved)
    WriteString("q\n");

  m_bColorSet = false;
  m_bGraphStateSet = false;
  if (m_ClipBoxStack.empty())
    return;

  m_ClipBox = m_ClipBoxStack.back();
  if (!bKeepSaved)
    m_ClipBoxStack.pop_back();
}

void CFX_PSRenderer::OutputPath(const CFX_Path* pPath,
                                const CFX_Matrix* pObject2Device) {
  std::ostringstream buf;
  size_t size = pPath->GetPoints().size();

  for (size_t i = 0; i < size; i++) {
    CFX_Path::Point::Type type = pPath->GetType(i);
    bool closing = pPath->IsClosingFigure(i);
    CFX_PointF pos = pPath->GetPoint(i);
    if (pObject2Device)
      pos = pObject2Device->Transform(pos);

    buf << pos.x << " " << pos.y;
    switch (type) {
      case CFX_Path::Point::Type::kMove:
        buf << " m ";
        break;
      case CFX_Path::Point::Type::kLine:
        buf << " l ";
        if (closing)
          buf << "h ";
        break;
      case CFX_Path::Point::Type::kBezier: {
        CFX_PointF pos1 = pPath->GetPoint(i + 1);
        CFX_PointF pos2 = pPath->GetPoint(i + 2);
        if (pObject2Device) {
          pos1 = pObject2Device->Transform(pos1);
          pos2 = pObject2Device->Transform(pos2);
        }
        buf << " " << pos1.x << " " << pos1.y << " " << pos2.x << " " << pos2.y
            << " c";
        if (closing)
          buf << " h";
        buf << "\n";
        i += 2;
        break;
      }
    }
  }
  WriteStream(buf);
}

void CFX_PSRenderer::SetClip_PathFill(
    const CFX_Path* pPath,
    const CFX_Matrix* pObject2Device,
    const CFX_FillRenderOptions& fill_options) {
  StartRendering();
  OutputPath(pPath, pObject2Device);
  CFX_FloatRect rect = pPath->GetBoundingBox();
  if (pObject2Device)
    rect = pObject2Device->TransformRect(rect);

  m_ClipBox.left = static_cast<int>(rect.left);
  m_ClipBox.right = static_cast<int>(rect.left + rect.right);
  m_ClipBox.top = static_cast<int>(rect.top + rect.bottom);
  m_ClipBox.bottom = static_cast<int>(rect.bottom);

  WriteString("W");
  if (fill_options.fill_type != CFX_FillRenderOptions::FillType::kWinding)
    WriteString("*");
  WriteString(" n\n");
}

void CFX_PSRenderer::SetClip_PathStroke(const CFX_Path* pPath,
                                        const CFX_Matrix* pObject2Device,
                                        const CFX_GraphStateData* pGraphState) {
  StartRendering();
  SetGraphState(pGraphState);

  std::ostringstream buf;
  buf << "mx Cm [" << pObject2Device->a << " " << pObject2Device->b << " "
      << pObject2Device->c << " " << pObject2Device->d << " "
      << pObject2Device->e << " " << pObject2Device->f << "]cm ";
  WriteStream(buf);

  OutputPath(pPath, nullptr);
  CFX_FloatRect rect = pPath->GetBoundingBoxForStrokePath(
      pGraphState->m_LineWidth, pGraphState->m_MiterLimit);
  m_ClipBox.Intersect(pObject2Device->TransformRect(rect).GetOuterRect());

  WriteString("strokepath W n sm\n");
}

bool CFX_PSRenderer::DrawPath(const CFX_Path* pPath,
                              const CFX_Matrix* pObject2Device,
                              const CFX_GraphStateData* pGraphState,
                              uint32_t fill_color,
                              uint32_t stroke_color,
                              const CFX_FillRenderOptions& fill_options) {
  StartRendering();
  int fill_alpha = FXARGB_A(fill_color);
  int stroke_alpha = FXARGB_A(stroke_color);
  if (fill_alpha && fill_alpha < 255)
    return false;
  if (stroke_alpha && stroke_alpha < 255)
    return false;
  if (fill_alpha == 0 && stroke_alpha == 0)
    return false;

  if (stroke_alpha) {
    SetGraphState(pGraphState);
    if (pObject2Device) {
      std::ostringstream buf;
      buf << "mx Cm [" << pObject2Device->a << " " << pObject2Device->b << " "
          << pObject2Device->c << " " << pObject2Device->d << " "
          << pObject2Device->e << " " << pObject2Device->f << "]cm ";
      WriteStream(buf);
    }
  }

  OutputPath(pPath, stroke_alpha ? nullptr : pObject2Device);
  if (fill_options.fill_type != CFX_FillRenderOptions::FillType::kNoFill &&
      fill_alpha) {
    SetColor(fill_color);
    if (fill_options.fill_type == CFX_FillRenderOptions::FillType::kWinding) {
      if (stroke_alpha)
        WriteString("q f Q ");
      else
        WriteString("f");
    } else if (fill_options.fill_type ==
               CFX_FillRenderOptions::FillType::kEvenOdd) {
      if (stroke_alpha)
        WriteString("q F Q ");
      else
        WriteString("F");
    }
  }

  if (stroke_alpha) {
    SetColor(stroke_color);
    WriteString("s");
    if (pObject2Device)
      WriteString(" sm");
  }

  WriteString("\n");
  return true;
}

void CFX_PSRenderer::SetGraphState(const CFX_GraphStateData* pGraphState) {
  std::ostringstream buf;
  if (!m_bGraphStateSet ||
      m_CurGraphState.m_LineCap != pGraphState->m_LineCap) {
    buf << static_cast<int>(pGraphState->m_LineCap) << " J\n";
  }
  if (!m_bGraphStateSet ||
      m_CurGraphState.m_DashArray != pGraphState->m_DashArray) {
    buf << "[";
    for (const auto& dash : pGraphState->m_DashArray)
      buf << dash << " ";
    buf << "]" << pGraphState->m_DashPhase << " d\n";
  }
  if (!m_bGraphStateSet ||
      m_CurGraphState.m_LineJoin != pGraphState->m_LineJoin) {
    buf << static_cast<int>(pGraphState->m_LineJoin) << " j\n";
  }
  if (!m_bGraphStateSet ||
      m_CurGraphState.m_LineWidth != pGraphState->m_LineWidth) {
    buf << pGraphState->m_LineWidth << " w\n";
  }
  if (!m_bGraphStateSet ||
      m_CurGraphState.m_MiterLimit != pGraphState->m_MiterLimit) {
    buf << pGraphState->m_MiterLimit << " M\n";
  }
  m_CurGraphState = *pGraphState;
  m_bGraphStateSet = true;
  WriteStream(buf);
}

bool CFX_PSRenderer::SetDIBits(const RetainPtr<CFX_DIBBase>& pSource,
                               uint32_t color,
                               int left,
                               int top) {
  StartRendering();
  CFX_Matrix matrix = CFX_RenderDevice::GetFlipMatrix(
      pSource->GetWidth(), pSource->GetHeight(), left, top);
  return DrawDIBits(pSource, color, matrix, FXDIB_ResampleOptions());
}

bool CFX_PSRenderer::StretchDIBits(const RetainPtr<CFX_DIBBase>& pSource,
                                   uint32_t color,
                                   int dest_left,
                                   int dest_top,
                                   int dest_width,
                                   int dest_height,
                                   const FXDIB_ResampleOptions& options) {
  StartRendering();
  CFX_Matrix matrix = CFX_RenderDevice::GetFlipMatrix(dest_width, dest_height,
                                                      dest_left, dest_top);
  return DrawDIBits(pSource, color, matrix, options);
}

bool CFX_PSRenderer::DrawDIBits(const RetainPtr<CFX_DIBBase>& pSource,
                                uint32_t color,
                                const CFX_Matrix& matrix,
                                const FXDIB_ResampleOptions& options) {
  StartRendering();
  if ((matrix.a == 0 && matrix.b == 0) || (matrix.c == 0 && matrix.d == 0))
    return true;

  if (pSource->IsAlphaFormat())
    return false;

  int alpha = FXARGB_A(color);
  if (pSource->IsMaskFormat() && (alpha < 255 || pSource->GetBPP() != 1))
    return false;

  WriteString("q\n");

  std::ostringstream buf;
  buf << "[" << matrix.a << " " << matrix.b << " " << matrix.c << " "
      << matrix.d << " " << matrix.e << " " << matrix.f << "]cm ";

  int width = pSource->GetWidth();
  int height = pSource->GetHeight();
  buf << width << " " << height;

  if (pSource->GetBPP() == 1 && !pSource->HasPalette()) {
    int pitch = (width + 7) / 8;
    uint32_t src_size = height * pitch;
    std::unique_ptr<uint8_t, FxFreeDeleter> src_buf(
        FX_Alloc(uint8_t, src_size));
    for (int row = 0; row < height; row++) {
      const uint8_t* src_scan = pSource->GetScanline(row);
      memcpy(src_buf.get() + row * pitch, src_scan, pitch);
    }

    std::unique_ptr<uint8_t, FxFreeDeleter> output_buf;
    uint32_t output_size;
    bool compressed = FaxCompressData(std::move(src_buf), width, height,
                                      &output_buf, &output_size);
    if (pSource->IsMaskFormat()) {
      SetColor(color);
      m_bColorSet = false;
      buf << " true[";
    } else {
      buf << " 1[";
    }
    buf << width << " 0 0 -" << height << " 0 " << height
        << "]currentfile/ASCII85Decode filter ";

    if (compressed) {
      buf << "<</K -1/EndOfBlock false/Columns " << width << "/Rows " << height
          << ">>/CCITTFaxDecode filter ";
    }
    if (pSource->IsMaskFormat())
      buf << "iM\n";
    else
      buf << "false 1 colorimage\n";

    WriteStream(buf);
    WritePSBinary({output_buf.get(), output_size});
  } else {
    CFX_DIBExtractor source_extractor(pSource);
    RetainPtr<CFX_DIBBase> pConverted = source_extractor.GetBitmap();
    if (!pConverted)
      return false;
    switch (pSource->GetFormat()) {
      case FXDIB_Format::k1bppRgb:
      case FXDIB_Format::kRgb32:
        pConverted = pConverted->CloneConvert(FXDIB_Format::kRgb);
        break;
      case FXDIB_Format::k8bppRgb:
        if (pSource->HasPalette())
          pConverted = pConverted->CloneConvert(FXDIB_Format::kRgb);
        break;
      default:
        break;
    }
    if (!pConverted) {
      WriteString("\nQ\n");
      return false;
    }

    int bpp = pConverted->GetBPP() / 8;
    uint8_t* output_buf = nullptr;
    size_t output_size = 0;
    const char* filter = nullptr;
    if ((m_PSLevel == 2 || options.bLossy) &&
        m_pEncoderIface->pJpegEncodeFunc(pConverted, &output_buf,
                                         &output_size)) {
      filter = "/DCTDecode filter ";
    }
    if (!filter) {
      int src_pitch = width * bpp;
      output_size = height * src_pitch;
      output_buf = FX_Alloc(uint8_t, output_size);
      for (int row = 0; row < height; row++) {
        const uint8_t* src_scan = pConverted->GetScanline(row);
        uint8_t* dest_scan = output_buf + row * src_pitch;
        if (bpp == 3) {
          for (int col = 0; col < width; col++) {
            *dest_scan++ = src_scan[2];
            *dest_scan++ = src_scan[1];
            *dest_scan++ = *src_scan;
            src_scan += 3;
          }
        } else {
          memcpy(dest_scan, src_scan, src_pitch);
        }
      }
      uint8_t* compressed_buf;
      uint32_t compressed_size;
      PSCompressData(output_buf, output_size, &compressed_buf, &compressed_size,
                     &filter);
      if (output_buf != compressed_buf)
        FX_Free(output_buf);

      output_buf = compressed_buf;
      output_size = compressed_size;
    }
    buf << " 8[";
    buf << width << " 0 0 -" << height << " 0 " << height << "]";
    buf << "currentfile/ASCII85Decode filter ";
    if (filter)
      buf << filter;

    buf << "false " << bpp;
    buf << " colorimage\n";
    WriteStream(buf);

    WritePSBinary({output_buf, output_size});
    FX_Free(output_buf);
  }
  WriteString("\nQ\n");
  return true;
}

void CFX_PSRenderer::SetColor(uint32_t color) {
  if (m_bColorSet && m_LastColor == color)
    return;

  std::ostringstream buf;
  buf << FXARGB_R(color) / 255.0 << " " << FXARGB_G(color) / 255.0 << " "
      << FXARGB_B(color) / 255.0 << " rg\n";
  m_bColorSet = true;
  m_LastColor = color;
  WriteStream(buf);
}

void CFX_PSRenderer::FindPSFontGlyph(CFX_GlyphCache* pGlyphCache,
                                     CFX_Font* pFont,
                                     const TextCharPos& charpos,
                                     int* ps_fontnum,
                                     int* ps_glyphindex) {
  int i = 0;
  for (const auto& pPSFont : m_PSFontList) {
    for (int j = 0; j < pPSFont->m_nGlyphs; j++) {
      if (pPSFont->m_Glyphs[j].m_pFont == pFont &&
          pPSFont->m_Glyphs[j].m_GlyphIndex == charpos.m_GlyphIndex &&
          ((!pPSFont->m_Glyphs[j].m_bGlyphAdjust && !charpos.m_bGlyphAdjust) ||
           (pPSFont->m_Glyphs[j].m_bGlyphAdjust && charpos.m_bGlyphAdjust &&
            (fabs(pPSFont->m_Glyphs[j].m_AdjustMatrix[0] -
                  charpos.m_AdjustMatrix[0]) < 0.01 &&
             fabs(pPSFont->m_Glyphs[j].m_AdjustMatrix[1] -
                  charpos.m_AdjustMatrix[1]) < 0.01 &&
             fabs(pPSFont->m_Glyphs[j].m_AdjustMatrix[2] -
                  charpos.m_AdjustMatrix[2]) < 0.01 &&
             fabs(pPSFont->m_Glyphs[j].m_AdjustMatrix[3] -
                  charpos.m_AdjustMatrix[3]) < 0.01)))) {
        *ps_fontnum = i;
        *ps_glyphindex = j;
        return;
      }
    }
    ++i;
  }

  if (m_PSFontList.empty() || m_PSFontList.back()->m_nGlyphs == 256) {
    m_PSFontList.push_back(std::make_unique<CPSFont>());
    m_PSFontList.back()->m_nGlyphs = 0;
    std::ostringstream buf;
    buf << "8 dict begin/FontType 3 def/FontMatrix[1 0 0 1 0 0]def\n"
           "/FontBBox[0 0 0 0]def/Encoding 256 array def 0 1 255{Encoding "
           "exch/.notdef put}for\n"
           "/CharProcs 1 dict def CharProcs begin/.notdef {} def end\n"
           "/BuildGlyph{1 0 -10 -10 10 10 setcachedevice exch/CharProcs get "
           "exch 2 copy known not{pop/.notdef}if get exec}bind def\n"
           "/BuildChar{1 index/Encoding get exch get 1 index/BuildGlyph get "
           "exec}bind def\n"
           "currentdict end\n";
    buf << "/X" << static_cast<uint32_t>(m_PSFontList.size() - 1)
        << " exch definefont pop\n";
    WriteStream(buf);
  }

  *ps_fontnum = m_PSFontList.size() - 1;
  CPSFont* pPSFont = m_PSFontList[*ps_fontnum].get();
  int glyphindex = pPSFont->m_nGlyphs;
  *ps_glyphindex = glyphindex;
  pPSFont->m_Glyphs[glyphindex].m_GlyphIndex = charpos.m_GlyphIndex;
  pPSFont->m_Glyphs[glyphindex].m_pFont = pFont;
  pPSFont->m_Glyphs[glyphindex].m_bGlyphAdjust = charpos.m_bGlyphAdjust;
  if (charpos.m_bGlyphAdjust) {
    pPSFont->m_Glyphs[glyphindex].m_AdjustMatrix[0] = charpos.m_AdjustMatrix[0];
    pPSFont->m_Glyphs[glyphindex].m_AdjustMatrix[1] = charpos.m_AdjustMatrix[1];
    pPSFont->m_Glyphs[glyphindex].m_AdjustMatrix[2] = charpos.m_AdjustMatrix[2];
    pPSFont->m_Glyphs[glyphindex].m_AdjustMatrix[3] = charpos.m_AdjustMatrix[3];
  }
  pPSFont->m_nGlyphs++;

  CFX_Matrix matrix;
  if (charpos.m_bGlyphAdjust) {
    matrix =
        CFX_Matrix(charpos.m_AdjustMatrix[0], charpos.m_AdjustMatrix[1],
                   charpos.m_AdjustMatrix[2], charpos.m_AdjustMatrix[3], 0, 0);
  }
  const CFX_Path* pPath = pGlyphCache->LoadGlyphPath(
      pFont, charpos.m_GlyphIndex, charpos.m_FontCharWidth);
  if (!pPath)
    return;

  CFX_Path TransformedPath(*pPath);
  if (charpos.m_bGlyphAdjust)
    TransformedPath.Transform(matrix);

  std::ostringstream buf;
  buf << "/X" << *ps_fontnum << " Ff/CharProcs get begin/" << glyphindex
      << "{n ";
  for (size_t p = 0; p < TransformedPath.GetPoints().size(); p++) {
    CFX_PointF point = TransformedPath.GetPoint(p);
    switch (TransformedPath.GetType(p)) {
      case CFX_Path::Point::Type::kMove: {
        buf << point.x << " " << point.y << " m\n";
        break;
      }
      case CFX_Path::Point::Type::kLine: {
        buf << point.x << " " << point.y << " l\n";
        break;
      }
      case CFX_Path::Point::Type::kBezier: {
        CFX_PointF point1 = TransformedPath.GetPoint(p + 1);
        CFX_PointF point2 = TransformedPath.GetPoint(p + 2);
        buf << point.x << " " << point.y << " " << point1.x << " " << point1.y
            << " " << point2.x << " " << point2.y << " c\n";
        p += 2;
        break;
      }
    }
  }
  buf << "f}bind def end\n";
  buf << "/X" << *ps_fontnum << " Ff/Encoding get " << glyphindex << "/"
      << glyphindex << " put\n";
  WriteStream(buf);
}

bool CFX_PSRenderer::DrawText(int nChars,
                              const TextCharPos* pCharPos,
                              CFX_Font* pFont,
                              const CFX_Matrix& mtObject2Device,
                              float font_size,
                              uint32_t color) {
  // Check object to device matrix first, since it can scale the font size.
  if ((mtObject2Device.a == 0 && mtObject2Device.b == 0) ||
      (mtObject2Device.c == 0 && mtObject2Device.d == 0)) {
    return true;
  }

  // Do not send near zero font sizes to printers. See crbug.com/767343.
  float scale =
      std::min(mtObject2Device.GetXUnit(), mtObject2Device.GetYUnit());
  static constexpr float kEpsilon = 0.01f;
  if (fabsf(font_size * scale) < kEpsilon)
    return true;

  StartRendering();
  int alpha = FXARGB_A(color);
  if (alpha < 255)
    return false;

  SetColor(color);
  std::ostringstream buf;
  buf << "q[" << mtObject2Device.a << " " << mtObject2Device.b << " "
      << mtObject2Device.c << " " << mtObject2Device.d << " "
      << mtObject2Device.e << " " << mtObject2Device.f << "]cm\n";

  CFX_FontCache* pCache = CFX_GEModule::Get()->GetFontCache();
  RetainPtr<CFX_GlyphCache> pGlyphCache = pCache->GetGlyphCache(pFont);
  int last_fontnum = -1;
  for (int i = 0; i < nChars; i++) {
    int ps_fontnum;
    int ps_glyphindex;
    FindPSFontGlyph(pGlyphCache.Get(), pFont, pCharPos[i], &ps_fontnum,
                    &ps_glyphindex);
    if (last_fontnum != ps_fontnum) {
      buf << "/X" << ps_fontnum << " Ff " << font_size << " Fs Sf ";
      last_fontnum = ps_fontnum;
    }
    buf << pCharPos[i].m_Origin.x << " " << pCharPos[i].m_Origin.y << " m";
    ByteString hex = ByteString::Format("<%02X>", ps_glyphindex);
    buf << hex.AsStringView() << "Tj\n";
  }
  buf << "Q\n";
  WriteStream(buf);
  return true;
}

bool CFX_PSRenderer::FaxCompressData(
    std::unique_ptr<uint8_t, FxFreeDeleter> src_buf,
    int width,
    int height,
    std::unique_ptr<uint8_t, FxFreeDeleter>* dest_buf,
    uint32_t* dest_size) const {
  if (width * height <= 128) {
    *dest_buf = std::move(src_buf);
    *dest_size = (width + 7) / 8 * height;
    return false;
  }

  m_pEncoderIface->pFaxEncodeFunc(src_buf.get(), width, height, (width + 7) / 8,
                                  dest_buf, dest_size);
  return true;
}

void CFX_PSRenderer::PSCompressData(uint8_t* src_buf,
                                    uint32_t src_size,
                                    uint8_t** output_buf,
                                    uint32_t* output_size,
                                    const char** filter) const {
  *output_buf = src_buf;
  *output_size = src_size;
  *filter = "";
  if (src_size < 1024)
    return;

  uint8_t* dest_buf = nullptr;
  uint32_t dest_size = src_size;
  if (m_PSLevel >= 3) {
    std::unique_ptr<uint8_t, FxFreeDeleter> dest_buf_unique;
    if (m_pEncoderIface->pFlateEncodeFunc(src_buf, src_size, &dest_buf_unique,
                                          &dest_size)) {
      dest_buf = dest_buf_unique.release();
      *filter = "/FlateDecode filter ";
    }
  } else {
    std::unique_ptr<uint8_t, FxFreeDeleter> dest_buf_unique;
    if (m_pEncoderIface->pRunLengthEncodeFunc({src_buf, src_size},
                                              &dest_buf_unique, &dest_size)) {
      dest_buf = dest_buf_unique.release();
      *filter = "/RunLengthDecode filter ";
    }
  }
  if (dest_size < src_size) {
    *output_buf = dest_buf;
    *output_size = dest_size;
  } else {
    *filter = nullptr;
    FX_Free(dest_buf);
  }
}

void CFX_PSRenderer::WritePSBinary(pdfium::span<const uint8_t> data) {
  std::unique_ptr<uint8_t, FxFreeDeleter> dest_buf;
  uint32_t dest_size;
  if (m_pEncoderIface->pA85EncodeFunc(data, &dest_buf, &dest_size)) {
    m_pStream->WriteBlock(dest_buf.get(), dest_size);
  } else {
    m_pStream->WriteBlock(data.data(), data.size());
  }
}

void CFX_PSRenderer::WriteStream(std::ostringstream& stream) {
  if (stream.tellp() > 0)
    m_pStream->WriteBlock(stream.str().c_str(), stream.tellp());
}

void CFX_PSRenderer::WriteString(ByteStringView str) {
  m_pStream->WriteString(str);
}
