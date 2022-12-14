// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/url_forwarder_configurator.h"

#include "base/macros.h"
#include "build/build_config.h"

namespace remoting {

UrlForwarderConfigurator::UrlForwarderConfigurator() = default;

UrlForwarderConfigurator::~UrlForwarderConfigurator() = default;

#if !defined(OS_LINUX) && !defined(OS_WIN)

// static
std::unique_ptr<UrlForwarderConfigurator> UrlForwarderConfigurator::Create() {
  // Unsupported platforms.
  NOTREACHED();
  return nullptr;
}

#endif  // !defined(OS_LINUX) && !defined(OS_WIN)

}  // namespace remoting
