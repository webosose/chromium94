// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/manifest/manifest_manager_host.h"

#include <stdint.h>

#include "base/bind.h"
#include "content/browser/renderer_host/render_frame_host_impl.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/mojom/manifest/manifest.mojom.h"
#include "url/gurl.h"

namespace content {

ManifestManagerHost::ManifestManagerHost(RenderFrameHost* render_frame_host)
    : manifest_manager_frame_(
          static_cast<RenderFrameHostImpl*>(render_frame_host)) {
  // Check that |manifest_manager_frame_| is a main frame.
  DCHECK(!manifest_manager_frame_->GetParent());
}

ManifestManagerHost::~ManifestManagerHost() {
  DispatchPendingCallbacks();
}

void ManifestManagerHost::BindObserver(
    mojo::PendingAssociatedReceiver<blink::mojom::ManifestUrlChangeObserver>
        receiver) {
  manifest_url_change_observer_receiver_.Bind(std::move(receiver));
  manifest_url_change_observer_receiver_.SetFilter(
      manifest_manager_frame_->CreateMessageFilterForAssociatedReceiver(
          blink::mojom::ManifestUrlChangeObserver::Name_));
}

void ManifestManagerHost::GetManifest(GetManifestCallback callback) {
  auto& manifest_manager = GetManifestManager();
  int request_id = callbacks_.Add(
      std::make_unique<GetManifestCallback>(std::move(callback)));
  manifest_manager.RequestManifest(
      base::BindOnce(&ManifestManagerHost::OnRequestManifestResponse,
                     base::Unretained(this), request_id));
}

void ManifestManagerHost::RequestManifestDebugInfo(
    blink::mojom::ManifestManager::RequestManifestDebugInfoCallback callback) {
  GetManifestManager().RequestManifestDebugInfo(std::move(callback));
}

blink::mojom::ManifestManager& ManifestManagerHost::GetManifestManager() {
  if (!manifest_manager_) {
    manifest_manager_frame_->GetRemoteInterfaces()->GetInterface(
        manifest_manager_.BindNewPipeAndPassReceiver());
    manifest_manager_.set_disconnect_handler(base::BindOnce(
        &ManifestManagerHost::OnConnectionError, base::Unretained(this)));
  }
  return *manifest_manager_;
}

void ManifestManagerHost::DispatchPendingCallbacks() {
  std::vector<GetManifestCallback> callbacks;
  for (CallbackMap::iterator it(&callbacks_); !it.IsAtEnd(); it.Advance()) {
    callbacks.push_back(std::move(*it.GetCurrentValue()));
  }
  callbacks_.Clear();
  for (auto& callback : callbacks)
    std::move(callback).Run(GURL(), blink::mojom::Manifest::New());
}

void ManifestManagerHost::OnConnectionError() {
  DispatchPendingCallbacks();
  if (GetForCurrentDocument(manifest_manager_frame_)) {
    DeleteForCurrentDocument(manifest_manager_frame_);
  }
}

void ManifestManagerHost::OnRequestManifestResponse(
    int request_id,
    const GURL& url,
    blink::mojom::ManifestPtr manifest) {
  auto callback = std::move(*callbacks_.Lookup(request_id));
  callbacks_.Remove(request_id);
  std::move(callback).Run(url, std::move(manifest));
}

void ManifestManagerHost::ManifestUrlChanged(const GURL& manifest_url) {
  manifest_manager_frame_->GetPage().UpdateManifestUrl(manifest_url);
}

RENDER_DOCUMENT_HOST_USER_DATA_KEY_IMPL(ManifestManagerHost)
}  // namespace content
