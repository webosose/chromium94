// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_TRACING_BACKGROUND_TRACING_AGENT_CLIENT_IMPL_H_
#define CONTENT_BROWSER_TRACING_BACKGROUND_TRACING_AGENT_CLIENT_IMPL_H_

#include <string>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/tracing/public/mojom/background_tracing_agent.mojom.h"

namespace content {

class BackgroundTracingAgentClientImpl
    : public tracing::mojom::BackgroundTracingAgentClient {
 public:
  static void Create(
      int child_process_id,
      mojo::Remote<tracing::mojom::BackgroundTracingAgentProvider> provider);

  ~BackgroundTracingAgentClientImpl() override;

  // tracing::mojom::BackgroundTracingAgentClient methods:
  void OnInitialized() override;
  void OnTriggerBackgroundTrace(const std::string& histogram_name) override;
  void OnAbortBackgroundTrace() override;

 private:
  explicit BackgroundTracingAgentClientImpl(
      mojo::Remote<tracing::mojom::BackgroundTracingAgent> agent);

  mojo::Remote<tracing::mojom::BackgroundTracingAgent> agent_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundTracingAgentClientImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_TRACING_BACKGROUND_TRACING_AGENT_CLIENT_IMPL_H_
