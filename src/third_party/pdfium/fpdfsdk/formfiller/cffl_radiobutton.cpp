// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fpdfsdk/formfiller/cffl_radiobutton.h"

#include <utility>

#include "constants/ascii.h"
#include "core/fpdfdoc/cpdf_formcontrol.h"
#include "fpdfsdk/cpdfsdk_formfillenvironment.h"
#include "fpdfsdk/cpdfsdk_widget.h"
#include "fpdfsdk/formfiller/cffl_formfield.h"
#include "fpdfsdk/pwl/cpwl_special_button.h"
#include "public/fpdf_fwlevent.h"
#include "third_party/base/check.h"

CFFL_RadioButton::CFFL_RadioButton(CPDFSDK_FormFillEnvironment* pApp,
                                   CPDFSDK_Widget* pWidget)
    : CFFL_Button(pApp, pWidget) {}

CFFL_RadioButton::~CFFL_RadioButton() = default;

std::unique_ptr<CPWL_Wnd> CFFL_RadioButton::NewPWLWindow(
    const CPWL_Wnd::CreateParams& cp,
    std::unique_ptr<IPWL_SystemHandler::PerWindowData> pAttachedData) {
  auto pWnd = std::make_unique<CPWL_RadioButton>(cp, std::move(pAttachedData));
  pWnd->Realize();
  pWnd->SetCheck(m_pWidget->IsChecked());
  return std::move(pWnd);
}

bool CFFL_RadioButton::OnKeyDown(FWL_VKEYCODE nKeyCode,
                                 Mask<FWL_EVENTFLAG> nFlags) {
  switch (nKeyCode) {
    case FWL_VKEY_Return:
    case FWL_VKEY_Space:
      return true;
    default:
      return CFFL_FormField::OnKeyDown(nKeyCode, nFlags);
  }
}

bool CFFL_RadioButton::OnChar(CPDFSDK_Annot* pAnnot,
                              uint32_t nChar,
                              Mask<FWL_EVENTFLAG> nFlags) {
  switch (nChar) {
    case pdfium::ascii::kReturn:
    case pdfium::ascii::kSpace: {
      CPDFSDK_PageView* pPageView = pAnnot->GetPageView();
      DCHECK(pPageView);

      ObservedPtr<CPDFSDK_Annot> pObserved(m_pWidget.Get());
      if (m_pFormFillEnv->GetInteractiveFormFiller()->OnButtonUp(
              &pObserved, pPageView, nFlags) ||
          !pObserved) {
        return true;
      }

      CFFL_FormField::OnChar(pAnnot, nChar, nFlags);
      CPWL_RadioButton* pWnd = CreateOrUpdateRadioButton(pPageView);
      if (pWnd && !pWnd->IsReadOnly())
        pWnd->SetCheck(true);
      return CommitData(pPageView, nFlags);
    }
    default:
      return CFFL_FormField::OnChar(pAnnot, nChar, nFlags);
  }
}

bool CFFL_RadioButton::OnLButtonUp(CPDFSDK_PageView* pPageView,
                                   CPDFSDK_Annot* pAnnot,
                                   Mask<FWL_EVENTFLAG> nFlags,
                                   const CFX_PointF& point) {
  CFFL_Button::OnLButtonUp(pPageView, pAnnot, nFlags, point);

  if (!IsValid())
    return true;

  CPWL_RadioButton* pWnd = CreateOrUpdateRadioButton(pPageView);
  if (pWnd)
    pWnd->SetCheck(true);

  return CommitData(pPageView, nFlags);
}

bool CFFL_RadioButton::IsDataChanged(const CPDFSDK_PageView* pPageView) {
  CPWL_RadioButton* pWnd = GetRadioButton(pPageView);
  return pWnd && pWnd->IsChecked() != m_pWidget->IsChecked();
}

void CFFL_RadioButton::SaveData(const CPDFSDK_PageView* pPageView) {
  CPWL_RadioButton* pWnd = GetRadioButton(pPageView);
  if (!pWnd)
    return;

  bool bNewChecked = pWnd->IsChecked();
  if (bNewChecked) {
    CPDF_FormField* pField = m_pWidget->GetFormField();
    for (int32_t i = 0, sz = pField->CountControls(); i < sz; i++) {
      if (CPDF_FormControl* pCtrl = pField->GetControl(i)) {
        if (pCtrl->IsChecked())
          break;
      }
    }
  }
  ObservedPtr<CPDFSDK_Widget> observed_widget(m_pWidget.Get());
  ObservedPtr<CFFL_RadioButton> observed_this(this);
  m_pWidget->SetCheck(bNewChecked);
  if (!observed_widget)
    return;

  m_pWidget->UpdateField();
  if (!observed_widget || !observed_this)
    return;

  SetChangeMark();
}

CPWL_RadioButton* CFFL_RadioButton::GetRadioButton(
    const CPDFSDK_PageView* pPageView) const {
  return static_cast<CPWL_RadioButton*>(GetPWLWindow(pPageView));
}

CPWL_RadioButton* CFFL_RadioButton::CreateOrUpdateRadioButton(
    const CPDFSDK_PageView* pPageView) {
  return static_cast<CPWL_RadioButton*>(CreateOrUpdatePWLWindow(pPageView));
}
