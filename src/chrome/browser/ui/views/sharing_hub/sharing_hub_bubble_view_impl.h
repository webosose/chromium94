// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_SHARING_HUB_SHARING_HUB_BUBBLE_VIEW_IMPL_H_
#define CHROME_BROWSER_UI_VIEWS_SHARING_HUB_SHARING_HUB_BUBBLE_VIEW_IMPL_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/sharing_hub/sharing_hub_bubble_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"

namespace gfx {
class Canvas;
}  // namespace gfx

namespace content {
class WebContents;
}  // namespace content

namespace sharing_hub {

class SharingHubBubbleController;
class SharingHubBubbleActionButton;
struct SharingHubAction;

// View component of the Sharing Hub bubble that allows users to share/save the
// current page.
class SharingHubBubbleViewImpl : public SharingHubBubbleView,
                                 public LocationBarBubbleDelegateView {
 public:
  // Bubble will be anchored to |anchor_view|.
  SharingHubBubbleViewImpl(views::View* anchor_view,
                           content::WebContents* web_contents,
                           SharingHubBubbleController* controller);

  ~SharingHubBubbleViewImpl() override;

  // SharingHubBubbleView:
  void Hide() override;

  // views::WidgetDelegateView:
  bool ShouldShowCloseButton() const override;
  bool ShouldShowWindowTitle() const override;
  void WindowClosing() override;

  // LocationBarBubbleDelegateView:
  std::u16string GetAccessibleWindowTitle() const override;
  void OnPaint(gfx::Canvas* canvas) override;

  // Shows the bubble view.
  void Show(DisplayReason reason);

  void OnActionSelected(SharingHubBubbleActionButton* button);

  const views::View* GetButtonContainerForTesting() const;

 private:
  // views::BubbleDialogDelegateView:
  void Init() override;

  // Creates the scroll view.
  void CreateScrollView();

  // Populates the scroll view containing sharing actions.
  void PopulateScrollView(
      const std::vector<SharingHubAction>& first_party_actions,
      const std::vector<SharingHubAction>& third_party_actions);

  // Resizes and potentially moves the bubble to fit the content's preferred
  // size.
  void MaybeSizeToContents();

  // A raw pointer is safe since our controller will outlive us (the bubble is
  // lazily created with the controller).
  SharingHubBubbleController* controller_;

  // ScrollView containing the list of share/save actions.
  views::ScrollView* scroll_view_ = nullptr;

  base::WeakPtrFactory<SharingHubBubbleViewImpl> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(SharingHubBubbleViewImpl);
};

}  // namespace sharing_hub

#endif  // CHROME_BROWSER_UI_VIEWS_SHARING_HUB_SHARING_HUB_BUBBLE_VIEW_IMPL_H_
