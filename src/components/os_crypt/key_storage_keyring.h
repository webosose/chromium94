// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OS_CRYPT_KEY_STORAGE_KEYRING_H_
#define COMPONENTS_OS_CRYPT_KEY_STORAGE_KEYRING_H_

#include <string>

#include "base/component_export.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/os_crypt/key_storage_linux.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

// Specialisation of KeyStorageLinux that uses Libsecret.
class COMPONENT_EXPORT(OS_CRYPT) KeyStorageKeyring : public KeyStorageLinux {
 public:
  KeyStorageKeyring(
      scoped_refptr<base::SingleThreadTaskRunner> main_thread_runner,
      std::string application_name);
  ~KeyStorageKeyring() override;

 protected:
  // KeyStorageLinux
  base::SequencedTaskRunner* GetTaskRunner() override;
  bool Init() override;
  absl::optional<std::string> GetKeyImpl() override;

 private:
  // Generate a random string and store it as OScrypt's new password.
  absl::optional<std::string> AddRandomPasswordInKeyring();

  // Keyring calls need to originate from the main thread.
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_runner_;

  const std::string application_name_;

  DISALLOW_COPY_AND_ASSIGN(KeyStorageKeyring);
};

#endif  // COMPONENTS_OS_CRYPT_KEY_STORAGE_KEYRING_H_
