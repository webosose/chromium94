// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copied from
// chrome/browser/ui/views/permission_bubble/permission_prompt_impl.cc

#include "neva/app_runtime/browser/permissions/permission_prompt_webos.h"

#include <iterator>
#include <memory>

#include "base/callback.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "components/permissions/permission_uma_util.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

#include "neva/app_runtime/browser/permissions/permission_prompt.h"
#include "neva/pal_service/luna/luna_names.h"

std::unique_ptr<permissions::PermissionPrompt> CreatePermissionPrompt(
    content::WebContents* web_contents,
    permissions::PermissionPrompt::Delegate* delegate) {
  return std::make_unique<PermissionPromptWebOS>(web_contents, delegate);
}

PermissionPromptWebOS::PermissionPromptWebOS(
    content::WebContents* web_contents,
    permissions::PermissionPrompt::Delegate* delegate)
    : web_contents_(web_contents), delegate_(delegate) {
  content::WebContentsImpl* web_contents_impl =
      static_cast<content::WebContentsImpl*>(web_contents);
  std::string application_id =
      web_contents_impl->GetRendererPrefs().application_id;
  if (!application_id.empty())
    app_id_ = std::move(application_id);

  ShowBubble();
}

PermissionPromptWebOS::~PermissionPromptWebOS() {}

permissions::PermissionPromptDisposition
PermissionPromptWebOS::GetPromptDisposition() const {
  return permissions::PermissionPromptDisposition::NOT_APPLICABLE;
}

permissions::PermissionPrompt::TabSwitchingBehavior
PermissionPromptWebOS::GetTabSwitchingBehavior() {
  return permissions::PermissionPrompt::TabSwitchingBehavior::
      kDestroyPromptButKeepRequestPending;
}

void PermissionPromptWebOS::ShowBubble() {
  AcceptPermission();
}

void PermissionPromptWebOS::AcceptPermission() {
  delegate_->Accept();
}

void PermissionPromptWebOS::DenyPermission() {
  delegate_->Deny();
}

void PermissionPromptWebOS::ClosingPermission() {
  delegate_->Closing();
}
