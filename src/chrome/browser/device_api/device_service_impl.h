// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DEVICE_API_DEVICE_SERVICE_IMPL_H_
#define CHROME_BROWSER_DEVICE_API_DEVICE_SERVICE_IMPL_H_

#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/document_service_base.h"
#include "third_party/blink/public/mojom/device/device.mojom.h"

namespace content {
class RenderFrameHost;
}

// A browser-side mojo service, which corresponds to the navigator.managed Web
// API. Available only to trusted web applications.
class DeviceServiceImpl final
    : public content::DocumentServiceBase<blink::mojom::DeviceAPIService> {
 public:
  using DeviceAttributeCallback =
      base::OnceCallback<void(blink::mojom::DeviceAttributeResultPtr)>;

  // Tries to attach this mojo service to |host| for trusted web applications.
  // Will dynamically disconnect if the trustness status is revoked.
  static void Create(
      content::RenderFrameHost* host,
      mojo::PendingReceiver<blink::mojom::DeviceAPIService> receiver);

  // Register the user prefs.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  DeviceServiceImpl(const DeviceServiceImpl&) = delete;
  DeviceServiceImpl& operator=(const DeviceServiceImpl&) = delete;
  ~DeviceServiceImpl() override;

  // blink::mojom::DeviceAPIService:
  void GetDirectoryId(GetDirectoryIdCallback callback) override;
  void GetHostname(GetHostnameCallback callback) override;
  void GetSerialNumber(GetSerialNumberCallback callback) override;
  void GetAnnotatedAssetId(GetAnnotatedAssetIdCallback callback) override;
  void GetAnnotatedLocation(GetAnnotatedLocationCallback callback) override;

 private:
  DeviceServiceImpl(
      content::RenderFrameHost* host,
      mojo::PendingReceiver<blink::mojom::DeviceAPIService> receiver);

  void GetDeviceAttribute(
      base::OnceCallback<void(DeviceAttributeCallback)> handler,
      DeviceAttributeCallback callback);

  void OnDisposingIfNeeded();

  content::RenderFrameHost* const host_;
  PrefChangeRegistrar pref_change_registrar_;
};

#endif  // CHROME_BROWSER_DEVICE_API_DEVICE_SERVICE_IMPL_H_
