// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_SYNC_CLIENT_MOCK_H_
#define COMPONENTS_SYNC_DRIVER_SYNC_CLIENT_MOCK_H_

#include "base/files/file_path.h"
#include "components/sync/driver/sync_client.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace syncer {

class SyncClientMock : public SyncClient {
 public:
  SyncClientMock();
  ~SyncClientMock() override;
  MOCK_METHOD(PrefService*, GetPrefService, (), (override));
  MOCK_METHOD(signin::IdentityManager*, GetIdentityManager, (), (override));
  MOCK_METHOD(base::FilePath, GetLocalSyncBackendFolder, (), (override));
  MOCK_METHOD(DataTypeController::TypeVector,
              CreateDataTypeControllers,
              (SyncService * sync_service),
              (override));
  MOCK_METHOD(invalidation::InvalidationService*,
              GetInvalidationService,
              (),
              (override));
  MOCK_METHOD(syncer::SyncInvalidationsService*,
              GetSyncInvalidationsService,
              (),
              (override));
  MOCK_METHOD(TrustedVaultClient*, GetTrustedVaultClient, (), (override));
  MOCK_METHOD(scoped_refptr<ExtensionsActivity>,
              GetExtensionsActivity,
              (),
              (override));
  MOCK_METHOD(SyncApiComponentFactory*,
              GetSyncApiComponentFactory,
              (),
              (override));
  MOCK_METHOD(SyncTypePreferenceProvider*,
              GetPreferenceProvider,
              (),
              (override));
  MOCK_METHOD(void, OnLocalSyncTransportDataCleared, (), (override));

 private:
  DISALLOW_COPY_AND_ASSIGN(SyncClientMock);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DRIVER_SYNC_CLIENT_MOCK_H_
