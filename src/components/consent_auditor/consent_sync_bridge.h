// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CONSENT_AUDITOR_CONSENT_SYNC_BRIDGE_H_
#define COMPONENTS_CONSENT_AUDITOR_CONSENT_SYNC_BRIDGE_H_

#include <memory>

#include "base/memory/weak_ptr.h"

namespace syncer {
class ModelTypeControllerDelegate;
}

namespace sync_pb {
class UserConsentSpecifics;
}

namespace consent_auditor {

class ConsentSyncBridge {
 public:
  ConsentSyncBridge() = default;
  virtual ~ConsentSyncBridge() = default;

  virtual void RecordConsent(
      std::unique_ptr<sync_pb::UserConsentSpecifics> specifics) = 0;

  // Returns the delegate for the controller, i.e. sync integration point.
  virtual base::WeakPtr<syncer::ModelTypeControllerDelegate>
  GetControllerDelegate() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(ConsentSyncBridge);
};

}  // namespace consent_auditor

#endif  // COMPONENTS_CONSENT_AUDITOR_CONSENT_SYNC_BRIDGE_H_
