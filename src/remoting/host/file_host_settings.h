// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_FILE_HOST_SETTINGS_H_
#define REMOTING_HOST_FILE_HOST_SETTINGS_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/sequenced_task_runner.h"
#include "remoting/host/host_settings.h"

namespace base {
class Value;
}

namespace remoting {

// An implementation of HostSettings that reads settings from a JSON file.
// Note that this class currently doesn't watch for changes in the file.
class FileHostSettings final : public HostSettings {
 public:
  explicit FileHostSettings(const base::FilePath& settings_file);
  ~FileHostSettings() override;

  // HostSettings implementation.
  void InitializeInstance() override;
  std::string GetString(const HostSettingKey key) const override;
  void SetString(const HostSettingKey key, const std::string& value) override;

  FileHostSettings(const FileHostSettings&) = delete;
  FileHostSettings& operator=(const FileHostSettings&) = delete;

 private:
  base::FilePath settings_file_;

#if !defined(NDEBUG)
  // Used to make sure the instance is only used on the same sequenced once
  // SetString() is called.
  scoped_refptr<base::SequencedTaskRunner> task_runner_for_checking_sequence_;
#endif

  // TODO(yuweih): This needs to be guarded with a lock if we detect changes of
  // the settings file.
  std::unique_ptr<base::Value> settings_;
};

}  // namespace remoting

#endif  // REMOTING_HOST_FILE_HOST_SETTINGS_H_
