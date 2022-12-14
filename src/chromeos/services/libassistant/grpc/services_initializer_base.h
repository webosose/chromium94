// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_LIBASSISTANT_GRPC_SERVICES_INITIALIZER_BASE_H_
#define CHROMEOS_SERVICES_LIBASSISTANT_GRPC_SERVICES_INITIALIZER_BASE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/threading/thread.h"
#include "chromeos/services/libassistant/grpc/async_service_driver.h"
#include "third_party/grpc/src/include/grpcpp/completion_queue.h"
#include "third_party/grpc/src/include/grpcpp/server_builder.h"

namespace chromeos {
namespace libassistant {

// Initializes all services exposed by libassistant.
class ServicesInitializerBase {
 public:
  ServicesInitializerBase(
      const std::string& cq_thread_name,
      scoped_refptr<base::SequencedTaskRunner> main_task_runner);
  ServicesInitializerBase(const ServicesInitializerBase&) = delete;
  ServicesInitializerBase& operator=(const ServicesInitializerBase&) = delete;
  virtual ~ServicesInitializerBase();

  // Registers all supported services and initializes completion queue.
  // All services share the same completion queue.
  // Does not take ownership of server_builder.
  void RegisterServicesAndInitCQ(grpc::ServerBuilder* server_builder);

  // Start polling the completion queue.
  void StartCQ();
  // Shutdown the CQ, stop CQ thread, then drain CQ
  void StopCQ();

 protected:
  // Registers all services with server_builder.
  // Does not take ownership of server_builder.
  virtual void InitDrivers(grpc::ServerBuilder* server_builder) = 0;

  void ScanCQInternal();

  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  std::vector<std::unique_ptr<AsyncServiceDriver>> service_drivers_;

  // Use a dedicated thread to poll completion queue. Will also responsible
  // for cleaning up the tags returned by calling cq_->Next() after they are
  // executed.
  base::Thread cq_thread_;

  // The task runner for the main thread.
  scoped_refptr<base::SequencedTaskRunner> main_task_runner_;
};

}  // namespace libassistant
}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_LIBASSISTANT_GRPC_SERVICES_INITIALIZER_BASE_H_
