// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/webrtc_desktop_capture_private/webrtc_desktop_capture_private_api.h"

#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/api/tabs.h"
#include "chrome/common/extensions/api/webrtc_desktop_capture_private.h"
#include "content/public/browser/web_contents.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/is_potentially_trustworthy.h"

namespace extensions {

namespace {

const char kTargetNotFoundError[] = "The specified target is not found.";
const char kUrlNotSecure[] =
    "URL scheme for the specified target is not secure.";

}  // namespace

WebrtcDesktopCapturePrivateChooseDesktopMediaFunction::
    WebrtcDesktopCapturePrivateChooseDesktopMediaFunction() {
}

WebrtcDesktopCapturePrivateChooseDesktopMediaFunction::
    ~WebrtcDesktopCapturePrivateChooseDesktopMediaFunction() {
}

ExtensionFunction::ResponseAction
WebrtcDesktopCapturePrivateChooseDesktopMediaFunction::Run() {
  using Params =
      extensions::api::webrtc_desktop_capture_private::ChooseDesktopMedia
          ::Params;
  const auto& list = args_->GetList();
  EXTENSION_FUNCTION_VALIDATE(list.size() > 0);
  const auto& request_id_value = list[0];
  EXTENSION_FUNCTION_VALIDATE(request_id_value.is_int());
  request_id_ = request_id_value.GetInt();
  DesktopCaptureRequestsRegistry::GetInstance()->AddRequest(source_process_id(),
                                                            request_id_, this);

  args_->EraseListIter(list.begin());

  std::unique_ptr<Params> params = Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params.get());

  content::RenderFrameHost* rfh = content::RenderFrameHost::FromID(
      params->request.guest_process_id,
      params->request.guest_render_frame_id);

  if (!rfh) {
    return RespondNow(Error(kTargetNotFoundError));
  }

  GURL origin = rfh->GetLastCommittedURL().GetOrigin();
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          ::switches::kAllowHttpScreenCapture) &&
      !network::IsUrlPotentiallyTrustworthy(origin)) {
    return RespondNow(Error(kUrlNotSecure));
  }
  std::u16string target_name =
      base::UTF8ToUTF16(network::IsUrlPotentiallyTrustworthy(origin)
                            ? net::GetHostAndOptionalPort(origin)
                            : origin.spec());

  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(rfh);
  if (!web_contents) {
    return RespondNow(Error(kTargetNotFoundError));
  }

  using Sources = std::vector<api::desktop_capture::DesktopCaptureSourceType>;
  Sources* sources = reinterpret_cast<Sources*>(&params->sources);
  return Execute(*sources, web_contents, origin, target_name);
}

WebrtcDesktopCapturePrivateCancelChooseDesktopMediaFunction::
    WebrtcDesktopCapturePrivateCancelChooseDesktopMediaFunction() {}

WebrtcDesktopCapturePrivateCancelChooseDesktopMediaFunction::
    ~WebrtcDesktopCapturePrivateCancelChooseDesktopMediaFunction() {}

}  // namespace extensions
