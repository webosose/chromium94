// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/document_transition/document_transition.h"

#include "base/trace_event/trace_event.h"
#include "cc/document_transition/document_transition_request.h"
#include "cc/trees/paint_holding_reason.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_document_transition_prepare_options.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_document_transition_start_options.h"
#include "third_party/blink/renderer/core/dom/abort_signal.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/layout/layout_box_model_object.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {
namespace {

const char kAbortedFromPrepare[] = "Aborted due to prepare() call";
const char kAbortedFromSignal[] = "Aborted due to abortSignal";

DocumentTransition::Request::Effect ParseEffect(const String& input) {
  using MapType = HashMap<String, DocumentTransition::Request::Effect>;
  DEFINE_STATIC_LOCAL(
      MapType*, lookup_map,
      (new MapType{
          {"cover-down", DocumentTransition::Request::Effect::kCoverDown},
          {"cover-left", DocumentTransition::Request::Effect::kCoverLeft},
          {"cover-right", DocumentTransition::Request::Effect::kCoverRight},
          {"cover-up", DocumentTransition::Request::Effect::kCoverUp},
          {"explode", DocumentTransition::Request::Effect::kExplode},
          {"fade", DocumentTransition::Request::Effect::kFade},
          {"implode", DocumentTransition::Request::Effect::kImplode},
          {"reveal-down", DocumentTransition::Request::Effect::kRevealDown},
          {"reveal-left", DocumentTransition::Request::Effect::kRevealLeft},
          {"reveal-right", DocumentTransition::Request::Effect::kRevealRight},
          {"reveal-up", DocumentTransition::Request::Effect::kRevealUp}}));

  auto it = lookup_map->find(input);
  return it != lookup_map->end() ? it->value
                                 : DocumentTransition::Request::Effect::kNone;
}

DocumentTransition::Request::Effect ParseRootTransition(
    const DocumentTransitionPrepareOptions* options) {
  return options->hasRootTransition()
             ? ParseEffect(options->rootTransition())
             : DocumentTransition::Request::Effect::kNone;
}

uint32_t NextDocumentTag() {
  static uint32_t next_document_tag = 1u;
  return next_document_tag++;
}

}  // namespace

DocumentTransition::DocumentTransition(Document* document)
    : ExecutionContextLifecycleObserver(document->GetExecutionContext()),
      document_(document),
      document_tag_(NextDocumentTag()) {}

void DocumentTransition::Trace(Visitor* visitor) const {
  visitor->Trace(document_);
  visitor->Trace(prepare_promise_resolver_);
  visitor->Trace(start_promise_resolver_);
  visitor->Trace(active_shared_elements_);
  visitor->Trace(signal_);

  ScriptWrappable::Trace(visitor);
  ActiveScriptWrappable::Trace(visitor);
  ExecutionContextLifecycleObserver::Trace(visitor);
}

void DocumentTransition::ContextDestroyed() {
  if (prepare_promise_resolver_) {
    prepare_promise_resolver_->Detach();
    prepare_promise_resolver_ = nullptr;
  }
  if (start_promise_resolver_) {
    start_promise_resolver_->Detach();
    start_promise_resolver_ = nullptr;
  }
  active_shared_elements_.clear();
  signal_ = nullptr;
  StopDeferringCommits();
}

bool DocumentTransition::HasPendingActivity() const {
  if (prepare_promise_resolver_ || start_promise_resolver_)
    return true;
  return false;
}

ScriptPromise DocumentTransition::prepare(
    ScriptState* script_state,
    const DocumentTransitionPrepareOptions* options,
    ExceptionState& exception_state) {
  // Reject any previous prepare promises.
  if (state_ == State::kPreparing || state_ == State::kPrepared) {
    CancelPendingTransition(kAbortedFromPrepare);
  }

  // Get the sequence id before any early outs so we will correctly process
  // callbacks from previous requests.
  last_prepare_sequence_id_ = next_sequence_id_++;

  // If we are not attached to a view, then we can't prepare a transition.
  // Reject the promise.
  if (!document_ || !document_->View()) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kInvalidStateError,
        "The document must be connected to a window.");
    return ScriptPromise();
  }
  // We also reject the promise if we're in any state other than idle.
  if (state_ != State::kIdle) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kInvalidStateError,
        "The document is already executing a transition.");
    return ScriptPromise();
  }

  if (options->hasAbortSignal()) {
    if (options->abortSignal()->aborted()) {
      exception_state.ThrowDOMException(DOMExceptionCode::kAbortError,
                                        kAbortedFromSignal);
      return ScriptPromise();
    }

    signal_ = options->abortSignal();
    signal_->AddAlgorithm(WTF::Bind(&DocumentTransition::Abort,
                                    WrapWeakPersistent(this),
                                    WrapWeakPersistent(signal_.Get())));
  }

  // We're going to be creating a new transition, parse the options.
  auto effect = ParseRootTransition(options);
  if (options->hasSharedElements())
    SetActiveSharedElements(options->sharedElements());
  prepare_shared_element_count_ = active_shared_elements_.size();

  prepare_promise_resolver_ =
      MakeGarbageCollected<ScriptPromiseResolver>(script_state);

  state_ = State::kPreparing;
  pending_request_ = Request::CreatePrepare(
      effect, document_tag_, prepare_shared_element_count_,
      ConvertToBaseOnceCallback(CrossThreadBindOnce(
          &DocumentTransition::NotifyPrepareFinished,
          WrapCrossThreadWeakPersistent(this), last_prepare_sequence_id_)));

  NotifyHasChangesToCommit();
  return prepare_promise_resolver_->Promise();
}

void DocumentTransition::Abort(AbortSignal* signal) {
  // There is no RemoveAlgorithm() method on AbortSignal so compare the signal
  // bound to this callback to the one last passed to start().
  if (signal_ != signal)
    return;

  CancelPendingTransition(kAbortedFromSignal);
}

ScriptPromise DocumentTransition::start(
    ScriptState* script_state,
    const DocumentTransitionStartOptions* options,
    ExceptionState& exception_state) {
  if (state_ != State::kPrepared) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kInvalidStateError,
        "Transition must be prepared before it can be started.");
    return ScriptPromise();
  }

  signal_ = nullptr;
  StopDeferringCommits();
  if (options->hasSharedElements())
    SetActiveSharedElements(options->sharedElements());

  // We need to have the same amount of shared elements (even if null) as the
  // prepared ones.
  if (prepare_shared_element_count_ != active_shared_elements_.size()) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kInvalidStateError,
        String::Format("Start request sharedElement count (%u) must match the "
                       "prepare sharedElement count (%u).",
                       active_shared_elements_.size(),
                       prepare_shared_element_count_));
    SetActiveSharedElements({});
    return ScriptPromise();
  }

  last_start_sequence_id_ = next_sequence_id_++;
  state_ = State::kStarted;
  start_promise_resolver_ =
      MakeGarbageCollected<ScriptPromiseResolver>(script_state);
  pending_request_ = Request::CreateStart(
      document_tag_, prepare_shared_element_count_,
      ConvertToBaseOnceCallback(CrossThreadBindOnce(
          &DocumentTransition::NotifyStartFinished,
          WrapCrossThreadWeakPersistent(this), last_start_sequence_id_)));
  NotifyHasChangesToCommit();
  return start_promise_resolver_->Promise();
}

void DocumentTransition::NotifyHasChangesToCommit() {
  if (!document_ || !document_->GetPage() || !document_->View())
    return;

  // Schedule a new frame.
  document_->GetPage()->Animator().ScheduleVisualUpdate(document_->GetFrame());

  // Ensure paint artifact compositor does an update, since that's the mechanism
  // we use to pass transition requests to the compositor.
  document_->View()->SetPaintArtifactCompositorNeedsUpdate();
}

void DocumentTransition::NotifyPrepareFinished(uint32_t sequence_id) {
  // This notification is for a different sequence id.
  if (sequence_id != last_prepare_sequence_id_)
    return;

  // We could have detached the resolver if the execution context was destroyed.
  if (!prepare_promise_resolver_)
    return;

  DCHECK(state_ == State::kPreparing);
  DCHECK(prepare_promise_resolver_);

  // Defer commits before resolving the promise to ensure any updates made in
  // the callback are deferred.
  StartDeferringCommits();
  prepare_promise_resolver_->Resolve();
  prepare_promise_resolver_ = nullptr;
  state_ = State::kPrepared;
  SetActiveSharedElements({});
}

void DocumentTransition::NotifyStartFinished(uint32_t sequence_id) {
  // This notification is for a different sequence id.
  if (sequence_id != last_start_sequence_id_)
    return;

  // We could have detached the resolver if the execution context was destroyed.
  if (!start_promise_resolver_)
    return;

  DCHECK(state_ == State::kStarted);
  DCHECK(start_promise_resolver_);

  start_promise_resolver_->Resolve();
  start_promise_resolver_ = nullptr;
  state_ = State::kIdle;
  SetActiveSharedElements({});
}

std::unique_ptr<DocumentTransition::Request>
DocumentTransition::TakePendingRequest() {
  return std::move(pending_request_);
}

bool DocumentTransition::IsActiveElement(const Element* element) const {
  return active_shared_elements_.Contains(element);
}

DocumentTransitionSharedElementId DocumentTransition::GetSharedElementId(
    const Element* element) const {
  DCHECK(IsActiveElement(element));
  DocumentTransitionSharedElementId result(document_tag_);
  for (wtf_size_t i = 0; i < active_shared_elements_.size(); ++i) {
    if (active_shared_elements_[i] == element)
      result.AddIndex(i);
  }
  DCHECK(result.valid());
  return result;
}

void DocumentTransition::VerifySharedElements() {
  for (auto& active_element : active_shared_elements_) {
    if (!active_element)
      continue;

    auto* object = active_element->GetLayoutObject();

    // TODO(vmpstr): Should this work for replaced elements as well?
    if (object) {
      if (object->ShouldApplyPaintContainment())
        continue;

      auto* console_message = MakeGarbageCollected<ConsoleMessage>(
          mojom::ConsoleMessageSource::kRendering,
          mojom::ConsoleMessageLevel::kError,
          "Dropping element from transition. Shared element must have "
          "containt:paint");
      console_message->SetNodes(document_->GetFrame(),
                                {DOMNodeIds::IdForNode(active_element)});
      document_->AddConsoleMessage(console_message);
    }

    // Clear the shared element. Note that we don't remove the element from the
    // vector, since we need to preserve the order of the elements and we
    // support nulls as a valid active element.

    // Invalidate the element since we should no longer be compositing it.
    auto* box = active_element->GetLayoutBox();
    if (box && box->HasSelfPaintingLayer()) {
      box->SetNeedsPaintPropertyUpdate();
      box->Layer()->SetNeedsCompositingInputsUpdate();
    }
    active_element = nullptr;
  }
}

void DocumentTransition::SetActiveSharedElements(
    HeapVector<Member<Element>> elements) {
  // The way this is used, we should never be overriding a non-empty set with
  // another non-empty set of elements.
  DCHECK(elements.IsEmpty() || active_shared_elements_.IsEmpty());

  InvalidateActiveElements();
  active_shared_elements_ = std::move(elements);
  InvalidateActiveElements();
}

void DocumentTransition::InvalidateActiveElements() {
  for (auto& element : active_shared_elements_) {
    // We allow nulls.
    if (!element)
      continue;

    auto* box = element->GetLayoutBox();
    if (!box || !box->HasSelfPaintingLayer())
      continue;

    // We propagate the shared element id on an effect node for the object. This
    // means that we should update the paint properties to update the shared
    // element id.
    box->SetNeedsPaintPropertyUpdate();

    // We might need to composite or decomposite this layer.
    box->Layer()->SetNeedsCompositingInputsUpdate();
  }
}

void DocumentTransition::StartDeferringCommits() {
  DCHECK(!deferring_commits_);

  if (!document_->GetPage() || !document_->View())
    return;

  // Don't do paint holding if it could already be in progress for first
  // contentful paint.
  if (document_->View()->WillDoPaintHoldingForFCP())
    return;

  // Based on the viz side timeout to hold snapshots for 5 seconds.
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(
      "blink", "DocumentTransition::DeferringCommits", this);
  constexpr base::TimeDelta kTimeout = base::TimeDelta::FromSeconds(4);
  deferring_commits_ =
      document_->GetPage()->GetChromeClient().StartDeferringCommits(
          *document_->GetFrame(), kTimeout,
          cc::PaintHoldingReason::kDocumentTransition);
}

void DocumentTransition::StopDeferringCommits() {
  if (!deferring_commits_)
    return;

  TRACE_EVENT_NESTABLE_ASYNC_END0("blink",
                                  "DocumentTransition::DeferringCommits", this);
  deferring_commits_ = false;
  if (!document_->GetPage())
    return;

  document_->GetPage()->GetChromeClient().StopDeferringCommits(
      *document_->GetFrame(),
      cc::PaintHoldingCommitTrigger::kDocumentTransition);
}

void DocumentTransition::CancelPendingTransition(const char* abort_message) {
  DCHECK(state_ == State::kPreparing || state_ == State::kPrepared)
      << "Can not cancel transition at state : " << static_cast<int>(state_);

  if (prepare_promise_resolver_) {
    prepare_promise_resolver_->Reject(MakeGarbageCollected<DOMException>(
        DOMExceptionCode::kAbortError, abort_message));
    prepare_promise_resolver_ = nullptr;
  }
  StopDeferringCommits();
  state_ = State::kIdle;
  signal_ = nullptr;
}

}  // namespace blink
