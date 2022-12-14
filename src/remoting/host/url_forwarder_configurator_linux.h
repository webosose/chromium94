// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_URL_FORWARDER_CONFIGURATOR_LINUX_H_
#define REMOTING_HOST_URL_FORWARDER_CONFIGURATOR_LINUX_H_

#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "remoting/host/url_forwarder_configurator.h"

namespace remoting {

// Linux implementation of UrlForwarderConfigurator.
class UrlForwarderConfiguratorLinux final : public UrlForwarderConfigurator {
 public:
  UrlForwarderConfiguratorLinux();
  ~UrlForwarderConfiguratorLinux() override;

  void IsUrlForwarderSetUp(IsUrlForwarderSetUpCallback callback) override;
  void SetUpUrlForwarder(const SetUpUrlForwarderCallback& callback) override;

  UrlForwarderConfiguratorLinux(const UrlForwarderConfiguratorLinux&) = delete;
  UrlForwarderConfiguratorLinux& operator=(
      const UrlForwarderConfiguratorLinux&) = delete;

 private:
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;
};

}  // namespace remoting

#endif  // REMOTING_HOST_URL_FORWARDER_CONFIGURATOR_LINUX_H_
