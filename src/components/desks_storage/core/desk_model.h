// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DESKS_STORAGE_CORE_DESK_MODEL_H_
#define COMPONENTS_DESKS_STORAGE_CORE_DESK_MODEL_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/observer_list.h"
#include "base/time/time.h"

namespace ash {
class DeskTemplate;
}

namespace desks_storage {

class DeskModelObserver;
// The DeskModel is an interface for accessing desk templates.
// Actual desk template storage backend (e.g. local file system backend and Sync
// backend) classes should implement this interface. Desk template accessor
// methods return results using callbacks to allow backend implementation
// to use asynchronous I/O.
class DeskModel {
 public:
  // Status codes for listing all desk template entries.
  // kPartialFailure indicates that one or more entries failed to load.
  enum class GetAllEntriesStatus {
    kOk,
    kPartialFailure,
    kFailure,
  };

  // Status codes for getting desk template by UUID.
  enum class GetEntryByUuidStatus {
    kOk,
    kFailure,
    kNotFound,
    kInvalidUuid,
  };

  // Status codes for adding or updating a desk template.
  enum class AddOrUpdateEntryStatus {
    kOk,
    kFailure,
    kInvalidArgument,
    kHitMaximumLimit,
  };

  // Status codes for deleting desk templates.
  enum class DeleteEntryStatus {
    kOk,
    kFailure,
  };

  DeskModel();
  DeskModel(const DeskModel&) = delete;
  DeskModel& operator=(const DeskModel&) = delete;
  virtual ~DeskModel();

  using GetAllEntriesCallback =
      base::OnceCallback<void(GetAllEntriesStatus status,
                              std::vector<ash::DeskTemplate*> entries)>;
  // Returns a vector of entries in the model.
  virtual void GetAllEntries(GetAllEntriesCallback callback) = 0;

  using GetEntryByUuidCallback =
      base::OnceCallback<void(GetEntryByUuidStatus status,
                              std::unique_ptr<ash::DeskTemplate> entry)>;
  // Get a specific desk template by |uuid|. Actual storage backend does not
  // need to keep desk templates in memory. The storage backend could load the
  // specified desk template into memory and then call the |callback| with a
  // unique_ptr to the loaded desk template.
  // If the specified desk template does not exist, |callback| will be called
  // with |kNotFound| and an empty unique_ptr. If the specified desk template
  // exists, but could not be loaded/parsed, |callback| will be called with
  // |kFailure| and an empty unique_ptr. An asynchronous |callback| is used here
  // to accommodate storage backend that need to perform asynchronous I/O.
  virtual void GetEntryByUUID(const std::string& uuid,
                              GetEntryByUuidCallback callback) = 0;

  using AddOrUpdateEntryCallback =
      base::OnceCallback<void(AddOrUpdateEntryStatus status)>;
  // Add or update a desk template by |new_entry|'s UUID.
  // The given template's name could be cleaned (e.g. removing trailing
  // whitespace) and truncated to a reasonable length before saving. This method
  // will also validate the given |new_entry|. If the |new_entry| is missing
  // critical information, such as |uuid|, |callback| will be called with
  // |kInvalidArgument|. If the given desk template could not be persisted due
  // to any backend error, |callback| will be called with |kFailure|.
  virtual void AddOrUpdateEntry(std::unique_ptr<ash::DeskTemplate> new_entry,
                                AddOrUpdateEntryCallback callback) = 0;

  using DeleteEntryCallback =
      base::OnceCallback<void(DeleteEntryStatus status)>;
  // Remove entry with |uuid| from entries. If the entry with |uuid| does not
  // exist, then the deletion is considered a success.
  virtual void DeleteEntry(const std::string& uuid,
                           DeleteEntryCallback callback) = 0;

  // Delete all entries.
  virtual void DeleteAllEntries(DeleteEntryCallback callback) = 0;

  // Observer registration methods. The model will remove all observers upon
  // destruction automatically.
  void AddObserver(DeskModelObserver* observer);
  void RemoveObserver(DeskModelObserver* observer);

 protected:
  // The observers.
  base::ObserverList<DeskModelObserver>::Unchecked observers_;
};

}  // namespace desks_storage

#endif  // COMPONENTS_DESKS_STORAGE_CORE_DESK_MODEL_H_