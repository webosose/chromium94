// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DESKS_STORAGE_CORE_DESK_MODEL_OBSERVER_H_
#define COMPONENTS_DESKS_STORAGE_CORE_DESK_MODEL_OBSERVER_H_

#include <string>
#include <vector>

namespace ash {
class DeskTemplate;
}

namespace desks_storage {

// Observer for the Desk model. In the observer methods care should
// be taken to not modify the model.
class DeskModelObserver {
 public:
  DeskModelObserver() = default;
  DeskModelObserver(const DeskModelObserver&) = delete;
  DeskModelObserver& operator=(const DeskModelObserver&) = delete;

  // Invoked when desk templates are added/updated, removed remotely via sync.
  // This is the mechanism for the sync server to push changes in the state of
  // the model to clients.
  virtual void EntriesAddedOrUpdatedRemotely(
      const std::vector<const ash::DeskTemplate*>& new_entries) = 0;
  virtual void EntriesRemovedRemotely(
      const std::vector<std::string>& uuids) = 0;

  // Invoked when desk templates are added/updated, removed locally.
  virtual void EntriesAddedOrUpdatedLocally(
      const std::vector<const ash::DeskTemplate*>& new_entries) = 0;
  virtual void EntriesRemovedLocally(const std::vector<std::string>& uuids) = 0;

 protected:
  virtual ~DeskModelObserver() = default;
};

}  // namespace desks_storage

#endif  // COMPONENTS_DESKS_STORAGE_CORE_DESK_MODEL_OBSERVER_H_