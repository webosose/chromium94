// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_LIBASSISTANT_GRPC_EXTERNAL_SERVICES_HEARTBEAT_EVENT_HANDLER_DRIVER_H_
#define CHROMEOS_SERVICES_LIBASSISTANT_GRPC_EXTERNAL_SERVICES_HEARTBEAT_EVENT_HANDLER_DRIVER_H_

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/assistant/internal/proto/shared/proto/v2/delegate/event_handler_service.grpc.pb.h"
#include "chromeos/services/libassistant/grpc/async_service_driver.h"
#include "chromeos/services/libassistant/grpc/rpc_method_driver.h"

namespace assistant {
namespace api {
class OnHeartbeatEventRequest;
class OnHeartbeatEventResponse;
}  // namespace api
}  // namespace assistant

namespace chromeos {
namespace libassistant {

class HeartbeatEventHandlerDriver : public AsyncServiceDriver {
 public:
  explicit HeartbeatEventHandlerDriver(::grpc::ServerBuilder* server_builder);
  HeartbeatEventHandlerDriver(const HeartbeatEventHandlerDriver&) = delete;
  HeartbeatEventHandlerDriver& operator=(const HeartbeatEventHandlerDriver&) =
      delete;
  ~HeartbeatEventHandlerDriver() override;

 private:
  // Generally we should use fully qualified namespace inside classes to avoid
  // potential confliction, we use |using| here to simply the naming thus
  // increase the readability.
  using OnHeartbeatEventRequest = ::assistant::api::OnHeartbeatEventRequest;
  using OnHeartbeatEventResponse = ::assistant::api::OnHeartbeatEventResponse;
  using HeartbeatEventHandlerInterface =
      ::assistant::api::HeartbeatEventHandlerInterface;

  // AsyncServiceDriver overrides:
  void StartCQ(::grpc::ServerCompletionQueue* cq) override;

  // Handles incoming heartbeat RPC event delivery.
  void HandleEvent(
      grpc::ServerContext* context,
      const OnHeartbeatEventRequest* request,
      base::OnceCallback<void(const grpc::Status& status,
                              const OnHeartbeatEventResponse& response)> done);

  HeartbeatEventHandlerInterface::AsyncService service_;

  std::unique_ptr<
      RpcMethodDriver<OnHeartbeatEventRequest, OnHeartbeatEventResponse>>
      on_event_rpc_method_driver_;

  // This sequence checker ensures that all callbacks are called on the main
  // sequence.
  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<HeartbeatEventHandlerInterface::AsyncService>
      async_service_weak_factory_{&service_};
  base::WeakPtrFactory<HeartbeatEventHandlerDriver> weak_factory_{this};
};

}  // namespace libassistant
}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_LIBASSISTANT_GRPC_EXTERNAL_SERVICES_HEARTBEAT_EVENT_HANDLER_DRIVER_H_
