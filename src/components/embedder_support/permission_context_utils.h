// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EMBEDDER_SUPPORT_PERMISSION_CONTEXT_UTILS_H_
#define COMPONENTS_EMBEDDER_SUPPORT_PERMISSION_CONTEXT_UTILS_H_

#include "build/build_config.h"
#include "components/permissions/contexts/geolocation_permission_context.h"
#include "components/permissions/contexts/nfc_permission_context.h"
#include "components/permissions/permission_manager.h"

namespace content {
class BrowserContext;
}  // namespace content

#if defined(OS_MAC)
namespace device {
class GeolocationManager;
}  // namespace device
#endif  // defined(OS_MAC)

namespace webrtc {
class MediaStreamDeviceEnumerator;
}  // namespace webrtc

namespace embedder_support {

// Contains all delegates & helper classes needed to construct all default
// permission contexts via CreateDefaultPermissionContexts().
struct PermissionContextDelegates {
  PermissionContextDelegates();
  PermissionContextDelegates(PermissionContextDelegates&&);
  PermissionContextDelegates& operator=(PermissionContextDelegates&&);
  ~PermissionContextDelegates();

  std::unique_ptr<permissions::GeolocationPermissionContext::Delegate>
      geolocation_permission_context_delegate;
#if defined(OS_MAC)
  device::GeolocationManager* geolocation_manager;
#endif  // defined(OS_MAC)
  webrtc::MediaStreamDeviceEnumerator* media_stream_device_enumerator;
  std::unique_ptr<permissions::NfcPermissionContext::Delegate>
      nfc_permission_context_delegate;
};

// Creates default permission contexts shared between Content embedders.
// Embedders are expected to populate all fields of |delegates| which are then
// being used to create the specific permission contexts.
permissions::PermissionManager::PermissionContextMap
CreateDefaultPermissionContexts(content::BrowserContext* browser_context,
                                PermissionContextDelegates delegates);

}  // namespace embedder_support

#endif  // COMPONENTS_EMBEDDER_SUPPORT_PERMISSION_CONTEXT_UTILS_H_
