// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/win/dcomp_texture_host.h"

#include "base/unguessable_token.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "gpu/ipc/common/command_buffer_id.h"
#include "gpu/ipc/common/gpu_channel.mojom.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_mojo_bootstrap.h"
#include "media/base/win/mf_helpers.h"

namespace content {

DCOMPTextureHost::DCOMPTextureHost(
    scoped_refptr<gpu::GpuChannelHost> channel,
    int32_t route_id,
    scoped_refptr<base::SingleThreadTaskRunner> media_task_runner,
    mojo::PendingAssociatedRemote<gpu::mojom::DCOMPTexture> texture,
    Listener* listener)
    : channel_(std::move(channel)), route_id_(route_id), listener_(listener) {
  DVLOG_FUNC(1);
  DCHECK(media_task_runner->BelongsToCurrentThread());
  DCHECK(channel_);
  DCHECK(route_id_);
  DCHECK(listener_);

  texture_remote_.Bind(std::move(texture));
  IPC::ScopedAllowOffSequenceChannelAssociatedBindings allow_binding;
  texture_remote_->StartListening(
      receiver_.BindNewEndpointAndPassRemote(media_task_runner));
  texture_remote_.set_disconnect_handler(base::BindOnce(
      &DCOMPTextureHost::OnDisconnectedFromGpuProcess, base::Unretained(this)));
}

DCOMPTextureHost::~DCOMPTextureHost() {
  DVLOG_FUNC(1);

  if (!channel_)
    return;

  // We destroy the DCOMPTexture as a deferred message followed by a flush
  // to ensure this is ordered correctly with regards to previous deferred
  // messages, such as CreateSharedImage.
  uint32_t flush_id = channel_->EnqueueDeferredMessage(
      gpu::mojom::DeferredRequestParams::NewDestroyDcompTexture(route_id_));
  channel_->EnsureFlush(flush_id);
}

void DCOMPTextureHost::SetTextureSize(const gfx::Size& size) {
  DVLOG_FUNC(3);
  if (texture_remote_)
    texture_remote_->SetTextureSize(size);
}

void DCOMPTextureHost::SetDCOMPSurface(
    const base::UnguessableToken& surface_token) {
  DVLOG_FUNC(1);
  if (texture_remote_)
    texture_remote_->SetSurfaceHandle(surface_token);
}

void DCOMPTextureHost::OnDisconnectedFromGpuProcess() {
  DVLOG_FUNC(1);
  channel_ = nullptr;
  texture_remote_.reset();
  receiver_.reset();
}

void DCOMPTextureHost::OnSharedImageMailboxBound(const gpu::Mailbox& mailbox) {
  DVLOG_FUNC(1);
  listener_->OnSharedImageMailboxBound(mailbox);
}

void DCOMPTextureHost::OnDCOMPSurfaceHandleBound(bool success) {
  DVLOG_FUNC(1) << "success=" << success;
  listener_->OnDCOMPSurfaceHandleBound(success);
}

void DCOMPTextureHost::OnCompositionParamsChanged(
    const gfx::Rect& output_rect) {
  DVLOG_FUNC(3);
  listener_->OnCompositionParamsReceived(output_rect);
}

}  // namespace content
