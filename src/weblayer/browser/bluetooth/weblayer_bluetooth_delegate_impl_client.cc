// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "weblayer/browser/bluetooth/weblayer_bluetooth_delegate_impl_client.h"

#include "build/build_config.h"
#include "content/public/browser/render_frame_host.h"
#include "weblayer/browser/bluetooth/weblayer_bluetooth_chooser_context_factory.h"

#if defined(OS_ANDROID)
#include "components/permissions/android/bluetooth_chooser_android.h"
#include "components/permissions/android/bluetooth_scanning_prompt_android.h"
#include "weblayer/browser/bluetooth/weblayer_bluetooth_chooser_android_delegate.h"
#include "weblayer/browser/bluetooth/weblayer_bluetooth_scanning_prompt_android_delegate.h"
#endif  // OS_ANDROID

namespace weblayer {

WebLayerBluetoothDelegateImplClient::WebLayerBluetoothDelegateImplClient() =
    default;

WebLayerBluetoothDelegateImplClient::~WebLayerBluetoothDelegateImplClient() =
    default;

permissions::BluetoothChooserContext*
WebLayerBluetoothDelegateImplClient::GetBluetoothChooserContext(
    content::RenderFrameHost* frame) {
  return WebLayerBluetoothChooserContextFactory::GetForBrowserContext(
      frame->GetBrowserContext());
}

std::unique_ptr<content::BluetoothChooser>
WebLayerBluetoothDelegateImplClient::RunBluetoothChooser(
    content::RenderFrameHost* frame,
    const content::BluetoothChooser::EventHandler& event_handler) {
#if defined(OS_ANDROID)
  // TODO(https://crbug.com/1231932): Return nullptr if suppressed in vr.
  return std::make_unique<permissions::BluetoothChooserAndroid>(
      frame, event_handler,
      std::make_unique<WebLayerBluetoothChooserAndroidDelegate>());
#else
  // Web Bluetooth is not supported for desktop in WebLayer.
  return nullptr;
#endif
}

std::unique_ptr<content::BluetoothScanningPrompt>
WebLayerBluetoothDelegateImplClient::ShowBluetoothScanningPrompt(
    content::RenderFrameHost* frame,
    const content::BluetoothScanningPrompt::EventHandler& event_handler) {
#if defined(OS_ANDROID)
  return std::make_unique<permissions::BluetoothScanningPromptAndroid>(
      frame, event_handler,
      std::make_unique<WebLayerBluetoothScanningPromptAndroidDelegate>());
#else
  // Web Bluetooth is not supported for desktop in WebLayer.
  return nullptr;
#endif
}

}  // namespace weblayer
