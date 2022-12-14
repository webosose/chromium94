// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_LIBASSISTANT_GRPC_EXTERNAL_SERVICES_GRPC_SERVICES_INITIALIZER_H_
#define CHROMEOS_SERVICES_LIBASSISTANT_GRPC_EXTERNAL_SERVICES_GRPC_SERVICES_INITIALIZER_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "chromeos/services/libassistant/grpc/external_services/customer_registration_client.h"
#include "chromeos/services/libassistant/grpc/grpc_client_thread.h"
#include "chromeos/services/libassistant/grpc/services_initializer_base.h"
#include "third_party/grpc/src/include/grpcpp/server_builder.h"

namespace chromeos {
namespace libassistant {

class GrpcLibassistantClient;

// Component responsible for:
// 1. Set up a gRPC client by establishing a new channel with Libassistant
// server.
// 2. Start and manage Libassistant V2 event observer gRPC services.
class GrpcServicesInitializer : public ServicesInitializerBase {
 public:
  GrpcServicesInitializer(const std::string& libassistant_service_address,
                          const std::string& assistant_service_address);
  GrpcServicesInitializer(const GrpcServicesInitializer&) = delete;
  GrpcServicesInitializer& operator=(const GrpcServicesInitializer&) = delete;
  ~GrpcServicesInitializer() override;

  // Start assistant gRPC server. All supported services must be registered
  // before this method is called. Client functionality is not impacted by this
  // call. Returns false if the attempt to start a gRPC server failed.
  bool Start();

  // Expose a reference to |GrpcLibassistantClient|.
  GrpcLibassistantClient& GrpcLibassistantClient();

 private:
  // ServicesInitializerBase overrides:
  void InitDrivers(grpc::ServerBuilder* server_builder) override;

  // 1. Creates a channel object to obtain a handle to libassistant gRPC server
  // services.
  // 2. Creates a gRPC client using that channel and through which we can invoke
  // service methods implemented in the server.
  void InitLibassistGrpcClient();

  // 1. Adds a listening port where server can receive traffic (via
  // AddListeningPort).
  // 2. Adds a completion queue to handle async operations (via
  // AddCompletionQueue).
  // 3. Registers all supported services (via RegisterService).
  // This should be called before Start().
  void InitAssistantGrpcServer();

  // Address of assistant gRPC server.
  const std::string assistant_service_address_;
  // Address of Libassistant gRPC server.
  const std::string libassistant_service_address_;

  grpc::ServerBuilder server_builder_;
  std::unique_ptr<grpc::Server> assistant_grpc_server_;

  // The entrypoint through which we can query Libassistant V2 APIs.
  std::unique_ptr<chromeos::libassistant::GrpcLibassistantClient>
      libassistant_client_;

  SEQUENCE_CHECKER(sequence_checker_);

  std::unique_ptr<chromeos::libassistant::CustomerRegistrationClient>
      customer_registration_client_;

  base::WeakPtrFactory<GrpcServicesInitializer> weak_factory_{this};
};

}  // namespace libassistant
}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_LIBASSISTANT_GRPC_EXTERNAL_SERVICES_GRPC_SERVICES_INITIALIZER_H_
