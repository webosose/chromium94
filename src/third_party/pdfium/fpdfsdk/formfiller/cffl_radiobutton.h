// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_FORMFILLER_CFFL_RADIOBUTTON_H_
#define FPDFSDK_FORMFILLER_CFFL_RADIOBUTTON_H_

#include <memory>

#include "fpdfsdk/formfiller/cffl_button.h"

class CPWL_RadioButton;

class CFFL_RadioButton final : public CFFL_Button {
 public:
  CFFL_RadioButton(CPDFSDK_FormFillEnvironment* pApp, CPDFSDK_Widget* pWidget);
  ~CFFL_RadioButton() override;

  // CFFL_Button:
  std::unique_ptr<CPWL_Wnd> NewPWLWindow(
      const CPWL_Wnd::CreateParams& cp,
      std::unique_ptr<IPWL_SystemHandler::PerWindowData> pAttachedData)
      override;
  bool OnKeyDown(FWL_VKEYCODE nKeyCode, Mask<FWL_EVENTFLAG> nFlags) override;
  bool OnChar(CPDFSDK_Annot* pAnnot,
              uint32_t nChar,
              Mask<FWL_EVENTFLAG> nFlags) override;
  bool OnLButtonUp(CPDFSDK_PageView* pPageView,
                   CPDFSDK_Annot* pAnnot,
                   Mask<FWL_EVENTFLAG> nFlags,
                   const CFX_PointF& point) override;
  bool IsDataChanged(const CPDFSDK_PageView* pPageView) override;
  void SaveData(const CPDFSDK_PageView* pPageView) override;

 private:
  CPWL_RadioButton* GetRadioButton(const CPDFSDK_PageView* pPageView) const;
  CPWL_RadioButton* CreateOrUpdateRadioButton(
      const CPDFSDK_PageView* pPageView);
};

#endif  // FPDFSDK_FORMFILLER_CFFL_RADIOBUTTON_H_
