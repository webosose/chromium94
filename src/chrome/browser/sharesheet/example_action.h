// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SHARESHEET_EXAMPLE_ACTION_H_
#define CHROME_BROWSER_SHARESHEET_EXAMPLE_ACTION_H_

#include "chrome/browser/sharesheet/share_action.h"

namespace sharesheet {

class ExampleAction : public ShareAction {
 public:
  ExampleAction();
  ~ExampleAction() override;
  ExampleAction(const ExampleAction&) = delete;
  ExampleAction& operator=(const ExampleAction&) = delete;

  // ShareAction:
  const std::u16string GetActionName() override;
  const gfx::VectorIcon& GetActionIcon() override;
  void LaunchAction(SharesheetController* controller,
                    views::View* root_view,
                    apps::mojom::IntentPtr intent) override;
  void OnClosing(SharesheetController* controller) override;

 private:
  SharesheetController* controller_ = nullptr;
  std::string name_;
};

}  // namespace sharesheet

#endif  // CHROME_BROWSER_SHARESHEET_EXAMPLE_ACTION_H_
