// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WORKER_HOST_DEDICATED_WORKER_HOST_FACTORY_IMPL_H_
#define CONTENT_BROWSER_WORKER_HOST_DEDICATED_WORKER_HOST_FACTORY_IMPL_H_

#include "content/browser/net/cross_origin_embedder_policy_reporter.h"
#include "content/common/content_export.h"
#include "content/public/browser/global_routing_id.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/base/isolation_info.h"
#include "services/network/public/cpp/cross_origin_embedder_policy.h"
#include "services/network/public/mojom/cross_origin_embedder_policy.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/common/storage_key/storage_key.h"
#include "third_party/blink/public/mojom/worker/dedicated_worker_host_factory.mojom.h"
#include "url/origin.h"

namespace content {

// A factory for creating DedicatedWorkerHosts. Its lifetime is managed by the
// renderer over mojo via SelfOwnedReceiver. It lives on the UI thread.
class CONTENT_EXPORT DedicatedWorkerHostFactoryImpl
    : public blink::mojom::DedicatedWorkerHostFactory {
 public:
  DedicatedWorkerHostFactoryImpl(
      int worker_process_id,
      absl::optional<GlobalRenderFrameHostId> creator_render_frame_host_id,
      absl::optional<blink::DedicatedWorkerToken> creator_worker_token,
      GlobalRenderFrameHostId ancestor_render_frame_host_id,
      const blink::StorageKey& creator_storage_key,
      const net::IsolationInfo& isolation_info,
      const network::CrossOriginEmbedderPolicy& cross_origin_embedder_policy,
      base::WeakPtr<CrossOriginEmbedderPolicyReporter> creator_coep_reporter,
      base::WeakPtr<CrossOriginEmbedderPolicyReporter> ancestor_coep_reporter);
  ~DedicatedWorkerHostFactoryImpl() override;

  // blink::mojom::DedicatedWorkerHostFactory:
  void CreateWorkerHost(
      const blink::DedicatedWorkerToken& token,
      const GURL& script_url,
      mojo::PendingReceiver<blink::mojom::BrowserInterfaceBroker>
          broker_receiver,
      mojo::PendingReceiver<blink::mojom::DedicatedWorkerHost> host_receiver,
      base::OnceCallback<void(const network::CrossOriginEmbedderPolicy&)>
          callback) override;

  // PlzDedicatedWorker:
  void CreateWorkerHostAndStartScriptLoad(
      const blink::DedicatedWorkerToken& token,
      const GURL& script_url,
      network::mojom::CredentialsMode credentials_mode,
      blink::mojom::FetchClientSettingsObjectPtr
          outside_fetch_client_settings_object,
      mojo::PendingRemote<blink::mojom::BlobURLToken> blob_url_token,
      mojo::PendingRemote<blink::mojom::DedicatedWorkerHostFactoryClient>
          client) override;

 private:
  // The ID of the RenderProcessHost where the worker will live.
  const int worker_process_id_;

  // See comments on the corresponding members of DedicatedWorkerHost.
  const absl::optional<GlobalRenderFrameHostId> creator_render_frame_host_id_;
  const absl::optional<blink::DedicatedWorkerToken> creator_worker_token_;
  const GlobalRenderFrameHostId ancestor_render_frame_host_id_;

  // Storage key is used for storage partitioning, and for retrieving the
  // worker's origin.
  const blink::StorageKey creator_storage_key_;
  const net::IsolationInfo isolation_info_;
  const network::CrossOriginEmbedderPolicy cross_origin_embedder_policy_;

  base::WeakPtr<CrossOriginEmbedderPolicyReporter> creator_coep_reporter_;
  base::WeakPtr<CrossOriginEmbedderPolicyReporter> ancestor_coep_reporter_;

  DISALLOW_COPY_AND_ASSIGN(DedicatedWorkerHostFactoryImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WORKER_HOST_DEDICATED_WORKER_HOST_FACTORY_IMPL_H_
