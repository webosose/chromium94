// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/mathml/layout_ng_mathml_block_with_anonymous_mrow.h"

#include "third_party/blink/renderer/core/css/resolver/style_resolver.h"

namespace blink {

LayoutNGMathMLBlockWithAnonymousMrow::LayoutNGMathMLBlockWithAnonymousMrow(
    Element* element)
    : LayoutNGMathMLBlock(element) {
  DCHECK(element);
}

void LayoutNGMathMLBlockWithAnonymousMrow::AddChild(
    LayoutObject* new_child,
    LayoutObject* before_child) {
  LayoutBlock* anonymous_mrow = To<LayoutBlock>(FirstChild());
  if (!anonymous_mrow) {
    scoped_refptr<ComputedStyle> new_style =
        GetDocument().GetStyleResolver().CreateAnonymousStyleWithDisplay(
            StyleRef(), EDisplay::kBlockMath);

    UpdateAnonymousChildStyle(nullptr, *new_style);
    anonymous_mrow = new LayoutNGMathMLBlock(nullptr);
    anonymous_mrow->SetDocumentForAnonymous(&GetDocument());
    anonymous_mrow->SetStyle(std::move(new_style));
    LayoutBox::AddChild(anonymous_mrow);
  }
  anonymous_mrow->AddChild(new_child, before_child);
}

}  // namespace blink
