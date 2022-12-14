// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/cxfa_ffbarcode.h"

#include <utility>

#include "core/fxcrt/fx_extension.h"
#include "third_party/base/check.h"
#include "xfa/fwl/cfwl_app.h"
#include "xfa/fwl/cfwl_barcode.h"
#include "xfa/fwl/cfwl_notedriver.h"
#include "xfa/fxfa/cxfa_fffield.h"
#include "xfa/fxfa/cxfa_ffpageview.h"
#include "xfa/fxfa/cxfa_ffwidget.h"
#include "xfa/fxfa/cxfa_fwladapterwidgetmgr.h"
#include "xfa/fxfa/parser/cxfa_barcode.h"
#include "xfa/fxfa/parser/cxfa_border.h"

namespace {

const BarCodeInfo kBarCodeData[] = {
    {0x7fb4a18, "ean13", BarcodeType::ean13, BC_EAN13},
    {0x8d13a3d, "code11", BarcodeType::code11, BC_UNKNOWN},
    {0x8d149a8, "code49", BarcodeType::code49, BC_UNKNOWN},
    {0x8d16347, "code93", BarcodeType::code93, BC_UNKNOWN},
    {0x91a92e2, "upsMaxicode", BarcodeType::upsMaxicode, BC_UNKNOWN},
    {0xa7d48dc, "fim", BarcodeType::fim, BC_UNKNOWN},
    {0xb359fe9, "msi", BarcodeType::msi, BC_UNKNOWN},
    {0x121f738c, "code2Of5Matrix", BarcodeType::code2Of5Matrix, BC_UNKNOWN},
    {0x15358616, "ucc128", BarcodeType::ucc128, BC_UNKNOWN},
    {0x1f4bfa05, "rfid", BarcodeType::rfid, BC_UNKNOWN},
    {0x1fda71bc, "rss14Stacked", BarcodeType::rss14Stacked, BC_UNKNOWN},
    {0x22065087, "ean8add2", BarcodeType::ean8add2, BC_UNKNOWN},
    {0x2206508a, "ean8add5", BarcodeType::ean8add5, BC_UNKNOWN},
    {0x2278366c, "codabar", BarcodeType::codabar, BC_CODABAR},
    {0x2a039a8d, "telepen", BarcodeType::telepen, BC_UNKNOWN},
    {0x323ed337, "upcApwcd", BarcodeType::upcApwcd, BC_UNKNOWN},
    {0x347a1846, "postUSIMB", BarcodeType::postUSIMB, BC_UNKNOWN},
    {0x391bb836, "code128", BarcodeType::code128, BC_CODE128},
    {0x398eddaf, "dataMatrix", BarcodeType::dataMatrix, BC_DATAMATRIX},
    {0x3cff60a8, "upcEadd2", BarcodeType::upcEadd2, BC_UNKNOWN},
    {0x3cff60ab, "upcEadd5", BarcodeType::upcEadd5, BC_UNKNOWN},
    {0x402cb188, "code2Of5Standard", BarcodeType::code2Of5Standard, BC_UNKNOWN},
    {0x411764f7, "aztec", BarcodeType::aztec, BC_UNKNOWN},
    {0x44d4e84c, "ean8", BarcodeType::ean8, BC_EAN8},
    {0x48468902, "ucc128sscc", BarcodeType::ucc128sscc, BC_UNKNOWN},
    {0x4880aea4, "upcAadd2", BarcodeType::upcAadd2, BC_UNKNOWN},
    {0x4880aea7, "upcAadd5", BarcodeType::upcAadd5, BC_UNKNOWN},
    {0x54f18256, "code2Of5Industrial", BarcodeType::code2Of5Industrial,
     BC_UNKNOWN},
    {0x58e15f25, "rss14Limited", BarcodeType::rss14Limited, BC_UNKNOWN},
    {0x5c08d1b9, "postAUSReplyPaid", BarcodeType::postAUSReplyPaid, BC_UNKNOWN},
    {0x5fa700bd, "rss14", BarcodeType::rss14, BC_UNKNOWN},
    {0x631a7e35, "logmars", BarcodeType::logmars, BC_UNKNOWN},
    {0x6a236236, "pdf417", BarcodeType::pdf417, BC_PDF417},
    {0x6d098ece, "upcean2", BarcodeType::upcean2, BC_UNKNOWN},
    {0x6d098ed1, "upcean5", BarcodeType::upcean5, BC_UNKNOWN},
    {0x76b04eed, "code3Of9extended", BarcodeType::code3Of9extended, BC_UNKNOWN},
    {0x7c7db84a, "maxicode", BarcodeType::maxicode, BC_UNKNOWN},
    {0x8266f7f7, "ucc128random", BarcodeType::ucc128random, BC_UNKNOWN},
    {0x83eca147, "postUSDPBC", BarcodeType::postUSDPBC, BC_UNKNOWN},
    {0x8dd71de0, "postAUSStandard", BarcodeType::postAUSStandard, BC_UNKNOWN},
    {0x98adad85, "plessey", BarcodeType::plessey, BC_UNKNOWN},
    {0x9f84cce6, "ean13pwcd", BarcodeType::ean13pwcd, BC_UNKNOWN},
    {0xb514fbe9, "upcA", BarcodeType::upcA, BC_UPCA},
    {0xb514fbed, "upcE", BarcodeType::upcE, BC_UNKNOWN},
    {0xb5c6a853, "ean13add2", BarcodeType::ean13add2, BC_UNKNOWN},
    {0xb5c6a856, "ean13add5", BarcodeType::ean13add5, BC_UNKNOWN},
    {0xb81fc512, "postUKRM4SCC", BarcodeType::postUKRM4SCC, BC_UNKNOWN},
    {0xbad34b22, "code128SSCC", BarcodeType::code128SSCC, BC_UNKNOWN},
    {0xbfbe0cf6, "postUS5Zip", BarcodeType::postUS5Zip, BC_UNKNOWN},
    {0xc56618e8, "pdf417macro", BarcodeType::pdf417macro, BC_UNKNOWN},
    {0xca730f8a, "code2Of5Interleaved", BarcodeType::code2Of5Interleaved,
     BC_UNKNOWN},
    {0xd0097ac6, "rss14Expanded", BarcodeType::rss14Expanded, BC_UNKNOWN},
    {0xd25a0240, "postAUSCust2", BarcodeType::postAUSCust2, BC_UNKNOWN},
    {0xd25a0241, "postAUSCust3", BarcodeType::postAUSCust3, BC_UNKNOWN},
    {0xd53ed3e7, "rss14Truncated", BarcodeType::rss14Truncated, BC_UNKNOWN},
    {0xe72bcd57, "code128A", BarcodeType::code128A, BC_UNKNOWN},
    {0xe72bcd58, "code128B", BarcodeType::code128B, BC_CODE128_B},
    {0xe72bcd59, "code128C", BarcodeType::code128C, BC_CODE128_C},
    {0xee83c50f, "rss14StackedOmni", BarcodeType::rss14StackedOmni, BC_UNKNOWN},
    {0xf2a18f7e, "QRCode", BarcodeType::QRCode, BC_QR_CODE},
    {0xfaeaf37f, "postUSStandard", BarcodeType::postUSStandard, BC_UNKNOWN},
    {0xfb48155c, "code3Of9", BarcodeType::code3Of9, BC_CODE39},
};

Optional<BC_CHAR_ENCODING> CharEncodingFromString(const WideString& value) {
  if (value.CompareNoCase(L"UTF-16"))
    return CHAR_ENCODING_UNICODE;
  if (value.CompareNoCase(L"UTF-8"))
    return CHAR_ENCODING_UTF8;
  return pdfium::nullopt;
}

Optional<BC_TEXT_LOC> TextLocFromAttribute(XFA_AttributeValue value) {
  switch (value) {
    case XFA_AttributeValue::None:
      return BC_TEXT_LOC_NONE;
    case XFA_AttributeValue::Above:
      return BC_TEXT_LOC_ABOVE;
    case XFA_AttributeValue::Below:
      return BC_TEXT_LOC_BELOW;
    case XFA_AttributeValue::AboveEmbedded:
      return BC_TEXT_LOC_ABOVEEMBED;
    case XFA_AttributeValue::BelowEmbedded:
      return BC_TEXT_LOC_BELOWEMBED;
    default:
      return pdfium::nullopt;
  }
}

}  // namespace.

// static
const BarCodeInfo* CXFA_FFBarcode::GetBarcodeTypeByName(
    const WideString& wsName) {
  if (wsName.IsEmpty())
    return nullptr;

  auto* it = std::lower_bound(
      std::begin(kBarCodeData), std::end(kBarCodeData),
      FX_HashCode_GetLoweredW(wsName.AsStringView()),
      [](const BarCodeInfo& arg, uint32_t hash) { return arg.uHash < hash; });

  if (it != std::end(kBarCodeData) && wsName.EqualsASCII(it->pName))
    return it;

  return nullptr;
}

CXFA_FFBarcode::CXFA_FFBarcode(CXFA_Node* pNode, CXFA_Barcode* barcode)
    : CXFA_FFTextEdit(pNode), barcode_(barcode) {}

CXFA_FFBarcode::~CXFA_FFBarcode() = default;

void CXFA_FFBarcode::Trace(cppgc::Visitor* visitor) const {
  CXFA_FFTextEdit::Trace(visitor);
  visitor->Trace(barcode_);
}

bool CXFA_FFBarcode::LoadWidget() {
  DCHECK(!IsLoaded());

  CFWL_Barcode* pFWLBarcode = cppgc::MakeGarbageCollected<CFWL_Barcode>(
      GetFWLApp()->GetHeap()->GetAllocationHandle(), GetFWLApp());
  SetNormalWidget(pFWLBarcode);
  pFWLBarcode->SetAdapterIface(this);

  CFWL_NoteDriver* pNoteDriver = pFWLBarcode->GetFWLApp()->GetNoteDriver();
  pNoteDriver->RegisterEventTarget(pFWLBarcode, pFWLBarcode);
  m_pOldDelegate = pFWLBarcode->GetDelegate();
  pFWLBarcode->SetDelegate(this);

  {
    CFWL_Widget::ScopedUpdateLock update_lock(pFWLBarcode);
    pFWLBarcode->SetText(m_pNode->GetValue(XFA_ValuePicture::kDisplay));
    UpdateWidgetProperty();
  }

  return CXFA_FFField::LoadWidget();
}

void CXFA_FFBarcode::RenderWidget(CFGAS_GEGraphics* pGS,
                                  const CFX_Matrix& matrix,
                                  HighlightOption highlight) {
  if (!HasVisibleStatus())
    return;

  CFX_Matrix mtRotate = GetRotateMatrix();
  mtRotate.Concat(matrix);

  CXFA_FFWidget::RenderWidget(pGS, mtRotate, highlight);
  DrawBorder(pGS, m_pNode->GetUIBorder(), m_UIRect, mtRotate);
  RenderCaption(pGS, mtRotate);
  CFX_RectF rtWidget = GetNormalWidget()->GetWidgetRect();

  CFX_Matrix mt(1, 0, 0, 1, rtWidget.left, rtWidget.top);
  mt.Concat(mtRotate);
  GetNormalWidget()->DrawWidget(pGS, mt);
}

void CXFA_FFBarcode::UpdateWidgetProperty() {
  CXFA_FFTextEdit::UpdateWidgetProperty();

  const BarCodeInfo* info = GetBarcodeTypeByName(barcode_->GetBarcodeType());
  if (!info)
    return;

  auto* pBarCodeWidget = static_cast<CFWL_Barcode*>(GetNormalWidget());
  pBarCodeWidget->SetType(info->eBCType);

  Optional<WideString> encoding_string = barcode_->GetCharEncoding();
  if (encoding_string.has_value()) {
    Optional<BC_CHAR_ENCODING> encoding =
        CharEncodingFromString(encoding_string.value());
    if (encoding.has_value())
      pBarCodeWidget->SetCharEncoding(encoding.value());
  }

  Optional<bool> calcChecksum = barcode_->GetChecksum();
  if (calcChecksum.has_value())
    pBarCodeWidget->SetCalChecksum(calcChecksum.value());

  Optional<int32_t> dataLen = barcode_->GetDataLength();
  if (dataLen.has_value())
    pBarCodeWidget->SetDataLength(dataLen.value());

  Optional<char> startChar = barcode_->GetStartChar();
  if (startChar.has_value())
    pBarCodeWidget->SetStartChar(startChar.value());

  Optional<char> endChar = barcode_->GetEndChar();
  if (endChar.has_value())
    pBarCodeWidget->SetEndChar(endChar.value());

  Optional<int32_t> ecLevel = barcode_->GetECLevel();
  if (ecLevel.has_value())
    pBarCodeWidget->SetErrorCorrectionLevel(ecLevel.value());

  Optional<int32_t> width = barcode_->GetModuleWidth();
  if (width.has_value())
    pBarCodeWidget->SetModuleWidth(width.value());

  Optional<int32_t> height = barcode_->GetModuleHeight();
  if (height.has_value())
    pBarCodeWidget->SetModuleHeight(height.value());

  Optional<bool> printCheck = barcode_->GetPrintChecksum();
  if (printCheck.has_value())
    pBarCodeWidget->SetPrintChecksum(printCheck.value());

  Optional<XFA_AttributeValue> text_attr = barcode_->GetTextLocation();
  if (text_attr.has_value()) {
    Optional<BC_TEXT_LOC> textLoc = TextLocFromAttribute(text_attr.value());
    if (textLoc.has_value())
      pBarCodeWidget->SetTextLocation(textLoc.value());
  }

  // Truncated is currently not a supported flag.

  Optional<int8_t> ratio = barcode_->GetWideNarrowRatio();
  if (ratio.has_value())
    pBarCodeWidget->SetWideNarrowRatio(ratio.value());

  if (info->eName == BarcodeType::code3Of9 ||
      info->eName == BarcodeType::ean8 || info->eName == BarcodeType::ean13 ||
      info->eName == BarcodeType::upcA) {
    pBarCodeWidget->SetPrintChecksum(true);
  }
}

bool CXFA_FFBarcode::AcceptsFocusOnButtonDown(
    Mask<XFA_FWL_KeyFlag> dwFlags,
    const CFX_PointF& point,
    CFWL_MessageMouse::MouseCommand command) {
  auto* pBarCodeWidget = static_cast<CFWL_Barcode*>(GetNormalWidget());
  if (!pBarCodeWidget || pBarCodeWidget->IsProtectedType())
    return false;
  if (command == CFWL_MessageMouse::MouseCommand::kLeftButtonDown &&
      !m_pNode->IsOpenAccess()) {
    return false;
  }
  return CXFA_FFTextEdit::AcceptsFocusOnButtonDown(dwFlags, point, command);
}
