// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/permissions/permission_request_manager_test_api.h"

#include <memory>

#include "base/bind.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/permission_bubble/permission_prompt_bubble_view.h"
#include "chrome/browser/ui/views/permission_bubble/permission_prompt_impl.h"
#include "components/permissions/permission_request.h"
#include "ui/views/widget/widget.h"

namespace test {
namespace {

// Wraps a PermissionRequest so that it can pass a closure to itself to the
// PermissionRequest constructor. Without this wrapper, there's no way to
// handle all destruction paths.
class TestPermissionRequestOwner {
 public:
  explicit TestPermissionRequestOwner(permissions::RequestType type) {
    const bool user_gesture = true;
    auto decided = [](ContentSetting, bool) {};
    request_ = std::make_unique<permissions::PermissionRequest>(
        GURL("https://example.com"), type, user_gesture,
        base::BindOnce(decided),
        base::BindOnce(&TestPermissionRequestOwner::DeleteThis,
                       base::Unretained(this)));
  }

  permissions::PermissionRequest* request() { return request_.get(); }

 private:
  void DeleteThis() { delete this; }

  std::unique_ptr<permissions::PermissionRequest> request_;

  DISALLOW_COPY_AND_ASSIGN(TestPermissionRequestOwner);
};

}  // namespace

PermissionRequestManagerTestApi::PermissionRequestManagerTestApi(
    permissions::PermissionRequestManager* manager)
    : manager_(manager) {}

PermissionRequestManagerTestApi::PermissionRequestManagerTestApi(
    Browser* browser)
    : PermissionRequestManagerTestApi(
          permissions::PermissionRequestManager::FromWebContents(
              browser->tab_strip_model()->GetActiveWebContents())) {}

void PermissionRequestManagerTestApi::AddSimpleRequest(
    content::RenderFrameHost* source_frame,
    permissions::RequestType type) {
  TestPermissionRequestOwner* request_owner =
      new TestPermissionRequestOwner(type);
  manager_->AddRequest(source_frame, request_owner->request());
}

views::Widget* PermissionRequestManagerTestApi::GetPromptWindow() {
  PermissionPromptImpl* prompt =
      static_cast<PermissionPromptImpl*>(manager_->view_.get());
  return prompt ? prompt->GetPromptBubbleWidgetForTesting() : nullptr;
}

void PermissionRequestManagerTestApi::SimulateWebContentsDestroyed() {
  manager_->WebContentsDestroyed();
}

}  // namespace test
