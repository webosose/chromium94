// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_FILE_SYSTEM_FILE_SYSTEM_QUOTA_CLIENT_H_
#define STORAGE_BROWSER_FILE_SYSTEM_FILE_SYSTEM_QUOTA_CLIENT_H_

#include <string>

#include "base/component_export.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "base/thread_annotations.h"
#include "components/services/storage/public/cpp/storage_key_quota_client.h"
#include "storage/browser/file_system/file_system_quota_util.h"
#include "storage/browser/quota/quota_client_type.h"
#include "storage/common/file_system/file_system_types.h"
#include "third_party/blink/public/mojom/quota/quota_types.mojom.h"

namespace base {
class SequencedTaskRunner;
}

namespace blink {
class StorageKey;
}  // namespace blink

namespace storage {

class FileSystemContext;

// All of the public methods of this class are called by the quota manager
// (except for the constructor/destructor).
class COMPONENT_EXPORT(STORAGE_BROWSER) FileSystemQuotaClient
    : public StorageKeyQuotaClient {
 public:
  explicit FileSystemQuotaClient(FileSystemContext* file_system_context);
  ~FileSystemQuotaClient() override;

  FileSystemQuotaClient(const FileSystemQuotaClient&) = delete;
  FileSystemQuotaClient& operator=(const FileSystemQuotaClient&) = delete;

  // QuotaClient methods.
  void GetStorageKeyUsage(const blink::StorageKey& storage_key,
                          blink::mojom::StorageType type,
                          GetStorageKeyUsageCallback callback) override;
  void GetStorageKeysForType(blink::mojom::StorageType type,
                             GetStorageKeysForTypeCallback callback) override;
  void GetStorageKeysForHost(blink::mojom::StorageType type,
                             const std::string& host,
                             GetStorageKeysForHostCallback callback) override;
  void DeleteStorageKeyData(const blink::StorageKey& storage_key,
                            blink::mojom::StorageType type,
                            DeleteStorageKeyDataCallback callback) override;
  void PerformStorageCleanup(blink::mojom::StorageType type,
                             PerformStorageCleanupCallback callback) override;

  // Converts FileSystemType `type` to/from the StorageType `storage_type` that
  // is used for the unified quota system.
  // (Basically this naively maps TEMPORARY storage type to TEMPORARY filesystem
  // type, PERSISTENT storage type to PERSISTENT filesystem type and vice
  // versa.)
  static FileSystemType QuotaStorageTypeToFileSystemType(
      blink::mojom::StorageType storage_type);

 private:
  base::SequencedTaskRunner* file_task_runner() const;

  SEQUENCE_CHECKER(sequence_checker_);

  // Raw pointer usage is safe because `file_system_context_` owns this.
  //
  // The FileSystemQuotaClient implementation mints scoped_refptrs from this
  // raw pointer in order to ensure that the FileSystemContext remains alive
  // while tasks are posted to the FileSystemContext's file sequence.
  //
  // So, it would be tempting to use scoped_refptr<FileSystemContext> here.
  // However, using scoped_refptr here creates a cycle, because
  // `file_system_context_` owns this. We could break the cycle in
  // FileSystemContext::Shutdown(), but then we would have to ensure that
  // Shutdown() is called by all FileSystemContext users.
  FileSystemContext* const file_system_context_
      GUARDED_BY_CONTEXT(sequence_checker_);
};

}  // namespace storage

#endif  // STORAGE_BROWSER_FILE_SYSTEM_FILE_SYSTEM_QUOTA_CLIENT_H_
