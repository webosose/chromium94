// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_QUICK_PAIR_KEYED_SERVICE_QUICK_PAIR_MEDIATOR_H_
#define ASH_QUICK_PAIR_KEYED_SERVICE_QUICK_PAIR_MEDIATOR_H_

#include <memory>

#include "ash/quick_pair/feature_status_tracker/quick_pair_feature_status_tracker.h"
#include "ash/quick_pair/scanning/scanner_broker.h"
#include "ash/quick_pair/ui/ui_broker.h"
#include "base/memory/scoped_refptr.h"
#include "base/scoped_observation.h"

namespace ash {
namespace quick_pair {

class FastPairRepository;
struct Device;

// Implements the Mediator design pattern for the components in the Quick Pair
// system, e.g. the UI Broker, Scanning Broker and Pairing Broker.
class Mediator final : public FeatureStatusTracker::Observer,
                       public ScannerBroker::Observer,
                       public UIBroker::Observer {
 public:
  class Factory {
   public:
    static std::unique_ptr<Mediator> Create();
    static void SetFactoryForTesting(Factory* factory);
    virtual ~Factory() = default;

   private:
    virtual std::unique_ptr<Mediator> BuildInstance() = 0;
  };

  Mediator(std::unique_ptr<FeatureStatusTracker> feature_status_tracker,
           std::unique_ptr<ScannerBroker> scanner_broker,
           std::unique_ptr<UIBroker> ui_broker,
           std::unique_ptr<FastPairRepository> fast_pair_repository);
  Mediator(const Mediator&) = delete;
  Mediator& operator=(const Mediator&) = delete;
  ~Mediator() override;

  // QuickPairFeatureStatusTracker::Observer
  void OnFastPairEnabledChanged(bool is_enabled) override;

  // SannerBroker::Observer
  void OnDeviceFound(scoped_refptr<Device> device) override;
  void OnDeviceLost(scoped_refptr<Device> device) override;

  // UIBroker::Observer
  void OnDiscoveryAction(scoped_refptr<Device> device,
                         DiscoveryAction action) override;
  void OnPairingFailureAction(scoped_refptr<Device> device,
                              PairingFailedAction action) override;
  void OnCompanionAppAction(scoped_refptr<Device> device,
                            CompanionAppAction action) override;
  void OnAssociateAccountAction(scoped_refptr<Device> device,
                                AssociateAccountAction action) override;

 private:
  void SetFastPairState(bool is_enabled);

  std::unique_ptr<FeatureStatusTracker> feature_status_tracker_;
  std::unique_ptr<ScannerBroker> scanner_broker_;
  std::unique_ptr<UIBroker> ui_broker_;
  std::unique_ptr<FastPairRepository> fast_pair_repository_;

  base::ScopedObservation<FeatureStatusTracker, FeatureStatusTracker::Observer>
      feature_status_tracker_observation_{this};
  base::ScopedObservation<ScannerBroker, ScannerBroker::Observer>
      scanner_broker_observation_{this};
  base::ScopedObservation<UIBroker, UIBroker::Observer> ui_broker_observation_{
      this};
};

}  // namespace quick_pair
}  // namespace ash

#endif  // ASH_QUICK_PAIR_KEYED_SERVICE_QUICK_PAIR_MEDIATOR_H_
