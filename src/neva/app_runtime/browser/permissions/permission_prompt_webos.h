// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copied from
// chrome/browser/ui/views/permission_bubble/permission_prompt_impl.h

#ifndef NEVA_APP_RUNTIME_BROWSER_PERMISSIONS_PERMISSION_PROMPT_WEBOS_H_
#define NEVA_APP_RUNTIME_BROWSER_PERMISSIONS_PERMISSION_PROMPT_WEBOS_H_

#include "base/callback.h"
#include "base/memory/raw_ptr.h"
#include "components/permissions/permission_prompt.h"

namespace content {
class WebContents;
}  // namespace content

// This object will create or trigger system UI to reflect that a website is
// requesting a permission.
class PermissionPromptWebOS : public permissions::PermissionPrompt {
 public:
  PermissionPromptWebOS(content::WebContents* web_contents, Delegate* delegate);

  PermissionPromptWebOS(const PermissionPromptWebOS&) = delete;
  PermissionPromptWebOS& operator=(const PermissionPromptWebOS&) = delete;

  ~PermissionPromptWebOS() override;

  // permissions::PermissionPrompt:
  permissions::PermissionPromptDisposition GetPromptDisposition()
      const override;
  void UpdateAnchor() override {}
  TabSwitchingBehavior GetTabSwitchingBehavior() override;

 private:
  void ShowBubble();

  void AcceptPermission();
  void DenyPermission();
  void ClosingPermission();

  std::string app_id_{};
  content::WebContents* web_contents_;
  const raw_ptr<permissions::PermissionPrompt::Delegate> delegate_;
};

#endif  // NEVA_APP_RUNTIME_BROWSER_PERMISSIONS_PERMISSION_PROMPT_WEBOS_H_
