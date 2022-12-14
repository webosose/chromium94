// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/heap/unified_heap_controller.h"

#include "base/logging.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/renderer/platform/bindings/dom_wrapper_world.h"
#include "third_party/blink/renderer/platform/bindings/script_forbidden_scope.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/wrapper_type_info.h"
#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/heap/heap_stats_collector.h"
#include "third_party/blink/renderer/platform/heap/impl/marking_visitor.h"
#include "third_party/blink/renderer/platform/heap/thread_state.h"

namespace blink {

namespace {

constexpr BlinkGC::StackState ToBlinkGCStackState(
    v8::EmbedderHeapTracer::EmbedderStackState stack_state) {
  return stack_state ==
                 v8::EmbedderHeapTracer::EmbedderStackState::kNoHeapPointers
             ? BlinkGC::kNoHeapPointersOnStack
             : BlinkGC::kHeapPointersOnStack;
}

}  // namespace

UnifiedHeapController::UnifiedHeapController(ThreadState* thread_state)
    : thread_state_(thread_state) {
  thread_state->Heap().stats_collector()->RegisterObserver(this);
}

UnifiedHeapController::~UnifiedHeapController() {
  thread_state_->Heap().stats_collector()->UnregisterObserver(this);
}

void UnifiedHeapController::TracePrologue(
    v8::EmbedderHeapTracer::TraceFlags v8_flags) {
  VLOG(2) << "UnifiedHeapController::TracePrologue";
  ThreadHeapStatsCollector::BlinkGCInV8Scope nested_scope(
      thread_state_->Heap().stats_collector());

  // Be conservative here as a new garbage collection gets started right away.
  thread_state_->FinishIncrementalMarkingIfRunning(
      BlinkGC::CollectionType::kMajor, BlinkGC::kHeapPointersOnStack,
      BlinkGC::kIncrementalAndConcurrentMarking,
      BlinkGC::kConcurrentAndLazySweeping,
      thread_state_->current_gc_data_.reason);

  thread_state_->SetGCState(ThreadState::kNoGCScheduled);
  BlinkGC::GCReason gc_reason;
  if (v8_flags & v8::EmbedderHeapTracer::TraceFlags::kForced) {
    gc_reason = BlinkGC::GCReason::kUnifiedHeapForcedForTestingGC;
  } else if (v8_flags & v8::EmbedderHeapTracer::TraceFlags::kReduceMemory) {
    gc_reason = BlinkGC::GCReason::kUnifiedHeapForMemoryReductionGC;
  } else {
    gc_reason = BlinkGC::GCReason::kUnifiedHeapGC;
  }
  thread_state_->StartIncrementalMarking(gc_reason);

  is_tracing_done_ = false;
}

void UnifiedHeapController::EnterFinalPause(EmbedderStackState stack_state) {
  VLOG(2) << "UnifiedHeapController::EnterFinalPause";
  ThreadHeapStatsCollector::BlinkGCInV8Scope nested_scope(
      thread_state_->Heap().stats_collector());
  thread_state_->AtomicPauseMarkPrologue(
      BlinkGC::CollectionType::kMajor, ToBlinkGCStackState(stack_state),
      BlinkGC::kIncrementalAndConcurrentMarking,
      thread_state_->current_gc_data_.reason);
  thread_state_->AtomicPauseMarkRoots(ToBlinkGCStackState(stack_state),
                                      BlinkGC::kIncrementalAndConcurrentMarking,
                                      thread_state_->current_gc_data_.reason);
}

void UnifiedHeapController::TraceEpilogue(
    v8::EmbedderHeapTracer::TraceSummary* summary) {
  VLOG(2) << "UnifiedHeapController::TraceEpilogue";
  {
    ThreadHeapStatsCollector::BlinkGCInV8Scope nested_scope(
        thread_state_->Heap().stats_collector());
    thread_state_->AtomicPauseMarkEpilogue(
        BlinkGC::kIncrementalAndConcurrentMarking);
    const BlinkGC::SweepingType sweeping_type =
        thread_state_->IsForcedGC() ? BlinkGC::kEagerSweeping
                                    : BlinkGC::kConcurrentAndLazySweeping;
    thread_state_->AtomicPauseSweepAndCompact(
        BlinkGC::CollectionType::kMajor,
        BlinkGC::kIncrementalAndConcurrentMarking, sweeping_type);

    ThreadHeapStatsCollector* const stats_collector =
        thread_state_->Heap().stats_collector();
    summary->allocated_size =
        static_cast<size_t>(stats_collector->marked_bytes());
    summary->time = stats_collector->marking_time_so_far().InMillisecondsF();
    buffered_allocated_size_ = 0;
  }
  thread_state_->AtomicPauseEpilogue();
}

void UnifiedHeapController::RegisterV8References(
    const std::vector<std::pair<void*, void*>>&
        internal_fields_of_potential_wrappers) {
  VLOG(2) << "UnifiedHeapController::RegisterV8References";
  DCHECK(thread_state()->IsMarkingInProgress());

  const bool was_in_atomic_pause = thread_state()->in_atomic_pause();
  if (!was_in_atomic_pause)
    ThreadState::Current()->EnterAtomicPause();
  for (const auto& internal_fields : internal_fields_of_potential_wrappers) {
    const WrapperTypeInfo* wrapper_type_info =
        reinterpret_cast<const WrapperTypeInfo*>(internal_fields.first);
    if (wrapper_type_info->gin_embedder != gin::GinEmbedder::kEmbedderBlink) {
      continue;
    }
    is_tracing_done_ = false;
    wrapper_type_info->Trace(thread_state_->CurrentVisitor(),
                             internal_fields.second);
  }
  if (!was_in_atomic_pause)
    ThreadState::Current()->LeaveAtomicPause();
}

bool UnifiedHeapController::AdvanceTracing(double deadline_in_ms) {
  VLOG(2) << "UnifiedHeapController::AdvanceTracing";
  ThreadHeapStatsCollector::BlinkGCInV8Scope nested_scope(
      thread_state_->Heap().stats_collector());
  if (!thread_state_->in_atomic_pause()) {
    ThreadHeapStatsCollector::EnabledScope advance_tracing_scope(
        thread_state_->Heap().stats_collector(),
        ThreadHeapStatsCollector::kUnifiedMarkingStep);
    // V8 calls into embedder tracing from its own marking to ensure
    // progress. Oilpan will additionally schedule marking steps.
    ThreadState::AtomicPauseScope atomic_pause_scope(thread_state_);
    ScriptForbiddenScope script_forbidden_scope;
    is_tracing_done_ = thread_state_->MarkPhaseAdvanceMarkingBasedOnSchedule(
        base::TimeDelta::FromMillisecondsD(deadline_in_ms),
        ThreadState::EphemeronProcessing::kPartialProcessing);
    if (!is_tracing_done_) {
      if (base::FeatureList::IsEnabled(
              blink::features::kBlinkHeapConcurrentMarking)) {
        thread_state_->ConcurrentMarkingStep();
      }
      thread_state_->RestartIncrementalMarkingIfPaused();
    }
    return is_tracing_done_;
  }
  thread_state_->AtomicPauseMarkTransitiveClosure();
  is_tracing_done_ = true;
  return true;
}

bool UnifiedHeapController::IsTracingDone() {
  return is_tracing_done_;
}

void UnifiedHeapController::ReportBufferedAllocatedSizeIfPossible() {
  // Avoid reporting to V8 in the following conditions as that may trigger GC
  // finalizations where not allowed.
  // - Recursive sweeping.
  // - GC forbidden scope.
  if ((thread_state()->IsSweepingInProgress() &&
       thread_state()->SweepForbidden()) ||
      thread_state()->IsGCForbidden()) {
    return;
  }

  if (buffered_allocated_size_ < 0) {
    DecreaseAllocatedSize(static_cast<size_t>(-buffered_allocated_size_));
  } else {
    IncreaseAllocatedSize(static_cast<size_t>(buffered_allocated_size_));
  }
  buffered_allocated_size_ = 0;
}

void UnifiedHeapController::IncreaseAllocatedObjectSize(size_t delta_bytes) {
  buffered_allocated_size_ += static_cast<int64_t>(delta_bytes);
  ReportBufferedAllocatedSizeIfPossible();
}

void UnifiedHeapController::DecreaseAllocatedObjectSize(size_t delta_bytes) {
  buffered_allocated_size_ -= static_cast<int64_t>(delta_bytes);
  ReportBufferedAllocatedSizeIfPossible();
}

}  // namespace blink
