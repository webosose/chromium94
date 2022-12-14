// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PERFORMANCE_MANAGER_GRAPH_POLICIES_BFCACHE_POLICY_H_
#define COMPONENTS_PERFORMANCE_MANAGER_GRAPH_POLICIES_BFCACHE_POLICY_H_

#include <map>

#include "base/time/time.h"
#include "base/timer/timer.h"
#include "components/performance_manager/public/graph/graph.h"
#include "components/performance_manager/public/graph/page_node.h"
#include "components/performance_manager/public/graph/system_node.h"

namespace performance_manager {

namespace policies {

// Policies that automatically flush the BFCache of pages that become non
// visible or when the system is under memory pressure.
class BFCachePolicy : public GraphOwned,
                      public PageNode::ObserverDefaultImpl,
                      public SystemNode::ObserverDefaultImpl {
 public:
  BFCachePolicy();
  BFCachePolicy(const BFCachePolicy&) = delete;
  BFCachePolicy(BFCachePolicy&&) = delete;
  BFCachePolicy& operator=(const BFCachePolicy&) = delete;
  BFCachePolicy& operator=(BFCachePolicy&&) = delete;
  ~BFCachePolicy() override;

  static base::TimeDelta
  GetDelayBeforeFlushingBFCacheAfterBackgroundForTesting();

 protected:
  // Try to flush the BFCache associated with |page_node|. This will be a no-op
  // if there's a pending navigation.
  virtual void MaybeFlushBFCache(const PageNode* page_node);

 private:
  // GraphOwned implementation:
  void OnPassedToGraph(Graph* graph) override;
  void OnTakenFromGraph(Graph* graph) override;

  // PageNodeObserver:
  void OnIsVisibleChanged(const PageNode* page_node) override;
  void OnLoadingStateChanged(const PageNode* page_node) override;
  void OnBeforePageNodeRemoved(const PageNode* page_node) override;

  // SystemNodeObserver:
  void OnMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel new_level) override;

  // PageNodes that become non visible will have their BFcache after a small
  // amount of time spent in that state, this map stores the timers for that
  // logic.
  std::map<const PageNode*, base::OneShotTimer> page_to_flush_timer_;

  Graph* graph_;
};

}  // namespace policies
}  // namespace performance_manager

#endif  // COMPONENTS_PERFORMANCE_MANAGER_GRAPH_POLICIES_BFCACHE_POLICY_H_
