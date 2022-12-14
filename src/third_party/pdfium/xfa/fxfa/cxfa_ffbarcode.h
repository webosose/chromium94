// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_CXFA_FFBARCODE_H_
#define XFA_FXFA_CXFA_FFBARCODE_H_

#include "fxbarcode/BC_Library.h"
#include "v8/include/cppgc/member.h"
#include "v8/include/cppgc/visitor.h"
#include "xfa/fxfa/cxfa_ffpageview.h"
#include "xfa/fxfa/cxfa_fftextedit.h"

enum class BarcodeType {
  aztec,
  codabar,
  code11,
  code128,
  code128A,
  code128B,
  code128C,
  code128SSCC,
  code2Of5Industrial,
  code2Of5Interleaved,
  code2Of5Matrix,
  code2Of5Standard,
  code3Of9,
  code3Of9extended,
  code49,
  code93,
  dataMatrix,
  ean13,
  ean13add2,
  ean13add5,
  ean13pwcd,
  ean8,
  ean8add2,
  ean8add5,
  fim,
  logmars,
  maxicode,
  msi,
  pdf417,
  pdf417macro,
  plessey,
  postAUSCust2,
  postAUSCust3,
  postAUSReplyPaid,
  postAUSStandard,
  postUKRM4SCC,
  postUS5Zip,
  postUSDPBC,
  postUSIMB,
  postUSStandard,
  QRCode,
  rfid,
  rss14,
  rss14Expanded,
  rss14Limited,
  rss14Stacked,
  rss14StackedOmni,
  rss14Truncated,
  telepen,
  ucc128,
  ucc128random,
  ucc128sscc,
  upcA,
  upcAadd2,
  upcAadd5,
  upcApwcd,
  upcE,
  upcEadd2,
  upcEadd5,
  upcean2,
  upcean5,
  upsMaxicode
};

struct BarCodeInfo {
  // |pName| hashed as if wide string.
  uint32_t uHash;
  // Inline string data reduces size for small strings.
  const char pName[20];
  BarcodeType eName;
  BC_TYPE eBCType;
};

class CXFA_Barcode;

class CXFA_FFBarcode final : public CXFA_FFTextEdit {
 public:
  static const BarCodeInfo* GetBarcodeTypeByName(const WideString& wsName);

  CONSTRUCT_VIA_MAKE_GARBAGE_COLLECTED;
  ~CXFA_FFBarcode() override;

  void Trace(cppgc::Visitor* visitor) const override;

  // CXFA_FFTextEdit
  bool LoadWidget() override;
  void RenderWidget(CFGAS_GEGraphics* pGS,
                    const CFX_Matrix& matrix,
                    HighlightOption highlight) override;
  void UpdateWidgetProperty() override;
  bool AcceptsFocusOnButtonDown(
      Mask<XFA_FWL_KeyFlag> dwFlags,
      const CFX_PointF& point,
      CFWL_MessageMouse::MouseCommand command) override;

 private:
  CXFA_FFBarcode(CXFA_Node* pNode, CXFA_Barcode* barcode);

  cppgc::Member<CXFA_Barcode> const barcode_;
};

#endif  // XFA_FXFA_CXFA_FFBARCODE_H_
