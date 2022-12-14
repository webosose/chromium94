// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/canvas/canvas2d/canvas_filter.h"

#include "third_party/blink/renderer/bindings/modules/v8/v8_union_canvasfilterdictionary_canvasfilterdictionaryarray.h"
#include "third_party/blink/renderer/core/css/parser/css_parser.h"
#include "third_party/blink/renderer/modules/canvas/canvas2d/canvas_filter_operation_resolver.h"

namespace blink {

CanvasFilter::CanvasFilter(FilterOperations filter_operations)
    : filter_operations_(filter_operations) {}

CanvasFilter* CanvasFilter::Create(
    const V8UnionCanvasFilterDictionaryOrCanvasFilterDictionaryArray* init,
    ExceptionState& exception_state) {
  HeapVector<Member<CanvasFilterDictionary>> filter_array;

  switch (init->GetContentType()) {
    case V8UnionCanvasFilterDictionaryOrCanvasFilterDictionaryArray::
        ContentType::kCanvasFilterDictionary:
      filter_array.push_back(init->GetAsCanvasFilterDictionary());
      break;
    case V8UnionCanvasFilterDictionaryOrCanvasFilterDictionaryArray::
        ContentType::kCanvasFilterDictionaryArray:
      filter_array = init->GetAsCanvasFilterDictionaryArray();
      break;
  }

  FilterOperations filter_operations =
      CanvasFilterOperationResolver::CreateFilterOperations(filter_array,
                                                            exception_state);

  return MakeGarbageCollected<CanvasFilter>(filter_operations);
}

void CanvasFilter::Trace(Visitor* visitor) const {
  visitor->Trace(filter_operations_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
