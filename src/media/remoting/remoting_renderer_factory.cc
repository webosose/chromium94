// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/remoting/remoting_renderer_factory.h"

#include "media/base/demuxer.h"
#include "media/remoting/receiver.h"
#include "media/remoting/receiver_controller.h"
#include "media/remoting/stream_provider.h"

namespace media {
namespace remoting {

RemotingRendererFactory::RemotingRendererFactory(
    mojo::PendingRemote<mojom::Remotee> remotee,
    std::unique_ptr<RendererFactory> renderer_factory,
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner)
    : receiver_controller_(ReceiverController::GetInstance()),
      rpc_broker_(receiver_controller_->rpc_broker()),
      renderer_handle_(rpc_broker_->GetUniqueHandle()),
      waiting_for_remote_handle_receiver_(nullptr),
      real_renderer_factory_(std::move(renderer_factory)),
      media_task_runner_(media_task_runner) {
  DVLOG(2) << __func__;
  DCHECK(receiver_controller_);

  // Register the callback to listen RPC_ACQUIRE_RENDERER message.
  rpc_broker_->RegisterMessageReceiverCallback(
      RpcBroker::kAcquireRendererHandle,
      base::BindRepeating(&RemotingRendererFactory::OnAcquireRenderer,
                          weak_factory_.GetWeakPtr()));
  receiver_controller_->Initialize(std::move(remotee));
}

RemotingRendererFactory::~RemotingRendererFactory() {
  rpc_broker_->UnregisterMessageReceiverCallback(
      RpcBroker::kAcquireRendererHandle);
}

std::unique_ptr<Renderer> RemotingRendererFactory::CreateRenderer(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    const scoped_refptr<base::TaskRunner>& worker_task_runner,
    AudioRendererSink* audio_renderer_sink,
    VideoRendererSink* video_renderer_sink,
    RequestOverlayInfoCB request_overlay_info_cb,
    const gfx::ColorSpace& target_color_space) {
  DVLOG(2) << __func__;

  auto receiver = std::make_unique<Receiver>(
      renderer_handle_, remote_renderer_handle_, receiver_controller_,
      media_task_runner,
      real_renderer_factory_->CreateRenderer(
          media_task_runner, worker_task_runner, audio_renderer_sink,
          video_renderer_sink, request_overlay_info_cb, target_color_space),
      base::BindOnce(&RemotingRendererFactory::OnAcquireRendererDone,
                     base::Unretained(this)));

  // If we haven't received a RPC_ACQUIRE_RENDERER yet, keep a reference to
  // |receiver|, and set its remote handle when we get the call to
  // OnAcquireRenderer().
  if (remote_renderer_handle_ == RpcBroker::kInvalidHandle)
    waiting_for_remote_handle_receiver_ = receiver->GetWeakPtr();

  return std::move(receiver);
}

void RemotingRendererFactory::OnReceivedRpc(
    std::unique_ptr<openscreen::cast::RpcMessage> message) {
  DCHECK(message);
  if (message->proc() == openscreen::cast::RpcMessage::RPC_ACQUIRE_RENDERER)
    OnAcquireRenderer(std::move(message));
  else
    VLOG(1) << __func__ << ": Unknow RPC message. proc=" << message->proc();
}

void RemotingRendererFactory::OnAcquireRenderer(
    std::unique_ptr<openscreen::cast::RpcMessage> message) {
  DCHECK(message->has_integer_value());
  DCHECK(message->integer_value() != RpcBroker::kInvalidHandle);

  remote_renderer_handle_ = message->integer_value();

  // If CreateRenderer() was called before we had a valid
  // |remote_renderer_handle_|, set it on the already created Receiver.
  if (waiting_for_remote_handle_receiver_) {
    // |waiting_for_remote_handle_receiver_| is the WeakPtr of the Receiver
    // instance and should be deref in the media thread.
    media_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&Receiver::SetRemoteHandle,
                                  waiting_for_remote_handle_receiver_,
                                  remote_renderer_handle_));
  }
}

void RemotingRendererFactory::OnAcquireRendererDone(int receiver_rpc_handle) {
  // RPC_ACQUIRE_RENDERER_DONE should be sent only once.
  //
  // WebMediaPlayerImpl might destroy and re-create the Receiver instance
  // several times for saving resources. However, RPC_ACQUIRE_RENDERER_DONE
  // shouldn't be sent multiple times whenever a Receiver instance is created.
  if (is_acquire_renderer_done_sent_)
    return;

  DVLOG(3) << __func__
           << ": Issues RPC_ACQUIRE_RENDERER_DONE RPC message. remote_handle="
           << remote_renderer_handle_ << " rpc_handle=" << receiver_rpc_handle;
  auto rpc = std::make_unique<openscreen::cast::RpcMessage>();
  rpc->set_handle(remote_renderer_handle_);
  rpc->set_proc(openscreen::cast::RpcMessage::RPC_ACQUIRE_RENDERER_DONE);
  rpc->set_integer_value(receiver_rpc_handle);
  rpc_broker_->SendMessageToRemote(std::move(rpc));

  // Once RPC_ACQUIRE_RENDERER_DONE is sent, it implies there is no Receiver
  // instance that is waiting the remote handle.
  waiting_for_remote_handle_receiver_ = nullptr;

  is_acquire_renderer_done_sent_ = true;
}

}  // namespace remoting
}  // namespace media
