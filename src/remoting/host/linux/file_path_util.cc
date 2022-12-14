// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/linux/file_path_util.h"

#include "base/base_paths.h"
#include "base/hash/md5.h"
#include "base/path_service.h"
#include "net/base/network_interfaces.h"

namespace remoting {

base::FilePath GetConfigDirectoryPath() {
  base::FilePath homedir;
  base::PathService::Get(base::DIR_HOME, &homedir);
  return homedir.Append(".config/chrome-remote-desktop");
}

std::string GetHostHash() {
  return "host#" + base::MD5String(net::GetHostName());
}

}  // namespace remoting
