// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/test/model/fake_sync_change_processor.h"

#include "components/sync/model/sync_change.h"
#include "components/sync/model/sync_data.h"

namespace syncer {

FakeSyncChangeProcessor::FakeSyncChangeProcessor() {}

FakeSyncChangeProcessor::~FakeSyncChangeProcessor() {}

absl::optional<ModelError> FakeSyncChangeProcessor::ProcessSyncChanges(
    const base::Location& from_here,
    const SyncChangeList& change_list) {
  changes_.insert(changes_.end(), change_list.begin(), change_list.end());
  return absl::nullopt;
}

const SyncChangeList& FakeSyncChangeProcessor::changes() const {
  return changes_;
}

SyncChangeList& FakeSyncChangeProcessor::changes() {
  return changes_;
}

}  // namespace syncer
