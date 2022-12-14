// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/sharing_hub/screenshot/screenshot_captured_bubble_controller.h"

#include "chrome/browser/image_editor/screenshot_flow.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"

namespace sharing_hub {

ScreenshotCapturedBubbleController::~ScreenshotCapturedBubbleController() {
  HideBubble();
}

// static
ScreenshotCapturedBubbleController* ScreenshotCapturedBubbleController::Get(
    content::WebContents* web_contents) {
  ScreenshotCapturedBubbleController::CreateForWebContents(web_contents);
  ScreenshotCapturedBubbleController* controller =
      ScreenshotCapturedBubbleController::FromWebContents(web_contents);
  return controller;
}

void ScreenshotCapturedBubbleController::ShowBubble(
    const image_editor::ScreenshotCaptureResult& image) {
  const gfx::Image& captured_image = image.image;
  ui::ScopedClipboardWriter(ui::ClipboardBuffer::kCopyPaste)
      .WriteImage(*captured_image.ToSkBitmap());
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents_);
  browser->window()->ShowScreenshotCapturedBubble(web_contents_, captured_image,
                                                  this);
}

void ScreenshotCapturedBubbleController::HideBubble() {
  NOTIMPLEMENTED();
}

void ScreenshotCapturedBubbleController::OnBubbleClosed() {
  NOTIMPLEMENTED();
}

void ScreenshotCapturedBubbleController::Capture(Browser* browser) {
  content::WebContents* web_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  screenshot_flow_ =
      std::make_unique<image_editor::ScreenshotFlow>(web_contents);

  base::OnceCallback<void(const image_editor::ScreenshotCaptureResult&)>
      callback = base::BindOnce(
          [](Browser* browser,
             const image_editor::ScreenshotCaptureResult& image) {
            if (image.image.IsEmpty())
              return;
            content::WebContents* web_contents =
                browser->tab_strip_model()->GetActiveWebContents();
            sharing_hub::ScreenshotCapturedBubbleController* controller =
                sharing_hub::ScreenshotCapturedBubbleController::Get(
                    web_contents);
            controller->ShowBubble(image);
          },
          browser);

  screenshot_flow_->Start(std::move(callback));
}

ScreenshotCapturedBubbleController::ScreenshotCapturedBubbleController() =
    default;

ScreenshotCapturedBubbleController::ScreenshotCapturedBubbleController(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {}

WEB_CONTENTS_USER_DATA_KEY_IMPL(ScreenshotCapturedBubbleController)

}  // namespace sharing_hub
