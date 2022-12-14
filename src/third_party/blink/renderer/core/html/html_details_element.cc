/*
 * Copyright (C) 2010, 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "third_party/blink/renderer/core/html/html_details_element.h"

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/strings/grit/blink_strings.h"
#include "third_party/blink/renderer/core/css/css_property_names.h"
#include "third_party/blink/renderer/core/css_value_keywords.h"
#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/dom/text.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/html/html_div_element.h"
#include "third_party/blink/renderer/core/html/html_slot_element.h"
#include "third_party/blink/renderer/core/html/html_style_element.h"
#include "third_party/blink/renderer/core/html/html_summary_element.h"
#include "third_party/blink/renderer/core/html/shadow/shadow_element_names.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/layout/layout_block_flow.h"
#include "third_party/blink/renderer/core/layout/layout_object_factory.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"
#include "third_party/blink/renderer/platform/text/platform_locale.h"

namespace blink {

HTMLDetailsElement::HTMLDetailsElement(Document& document)
    : HTMLElement(html_names::kDetailsTag, document), is_open_(false) {
  UseCounter::Count(document, WebFeature::kDetailsElement);
  EnsureUserAgentShadowRoot();
}

HTMLDetailsElement::~HTMLDetailsElement() = default;

// static
bool HTMLDetailsElement::IsFirstSummary(const Node& node) {
  DCHECK(IsA<HTMLDetailsElement>(node.parentElement()));
  if (!IsA<HTMLSummaryElement>(node))
    return false;
  return node.parentElement() &&
         &node ==
             Traversal<HTMLSummaryElement>::FirstChild(*node.parentElement());
}

void HTMLDetailsElement::DispatchPendingEvent(
    const AttributeModificationReason reason) {
  if (reason == AttributeModificationReason::kByParser)
    GetDocument().SetToggleDuringParsing(true);
  DispatchEvent(*Event::Create(event_type_names::kToggle));
  if (reason == AttributeModificationReason::kByParser)
    GetDocument().SetToggleDuringParsing(false);
}

LayoutObject* HTMLDetailsElement::CreateLayoutObject(const ComputedStyle& style,
                                                     LegacyLayout legacy) {
  return LayoutObjectFactory::CreateBlockFlow(*this, style, legacy);
}

void HTMLDetailsElement::DidAddUserAgentShadowRoot(ShadowRoot& root) {
  auto* default_summary =
      MakeGarbageCollected<HTMLSummaryElement>(GetDocument());
  default_summary->AppendChild(
      Text::Create(GetDocument(),
                   GetLocale().QueryString(IDS_DETAILS_WITHOUT_SUMMARY_LABEL)));

  HTMLSlotElement* summary_slot =
      HTMLSlotElement::CreateUserAgentCustomAssignSlot(GetDocument());
  summary_slot->SetIdAttribute(shadow_element_names::kIdDetailsSummary);
  summary_slot->AppendChild(default_summary);
  root.AppendChild(summary_slot);

  HTMLSlotElement* content_slot =
      HTMLSlotElement::CreateUserAgentDefaultSlot(GetDocument());
  content_slot->SetIdAttribute(shadow_element_names::kIdDetailsContent);
  if (RuntimeEnabledFeatures::AutoExpandDetailsElementEnabled()) {
    content_slot->SetInlineStyleProperty(CSSPropertyID::kContentVisibility,
                                         CSSValueID::kHidden);
    content_slot->EnsureDisplayLockContext().SetForDetailsElement(true);
    content_slot->SetInlineStyleProperty(CSSPropertyID::kDisplay,
                                         CSSValueID::kBlock);
  } else {
    content_slot->SetInlineStyleProperty(CSSPropertyID::kDisplay,
                                         CSSValueID::kNone);
  }
  root.AppendChild(content_slot);

  auto* default_summary_style = MakeGarbageCollected<HTMLStyleElement>(
      GetDocument(), CreateElementFlags::ByCreateElement());
  // This style is required only if this <details> shows the UA-provided
  // <summary>, not a light child <summary>.
  default_summary_style->setTextContent(R"CSS(
:host summary {
  display: list-item;
  counter-increment: list-item 0;
  list-style: disclosure-closed inside;
}
:host([open]) summary {
  list-style-type: disclosure-open;
}
)CSS");
  root.AppendChild(default_summary_style);
}

Element* HTMLDetailsElement::FindMainSummary() const {
  if (HTMLSummaryElement* summary =
          Traversal<HTMLSummaryElement>::FirstChild(*this))
    return summary;

  auto* element = UserAgentShadowRoot()->firstChild();
  CHECK(!element || IsA<HTMLSlotElement>(element));
  HTMLSlotElement* slot = To<HTMLSlotElement>(element);
  DCHECK(slot->firstChild());
  CHECK(IsA<HTMLSummaryElement>(*slot->firstChild()));
  return To<Element>(slot->firstChild());
}

void HTMLDetailsElement::ParseAttribute(
    const AttributeModificationParams& params) {
  if (params.name == html_names::kOpenAttr) {
    bool old_value = is_open_;
    is_open_ = !params.new_value.IsNull();
    if (is_open_ == old_value)
      return;

    // Dispatch toggle event asynchronously.
    pending_event_ = PostCancellableTask(
        *GetDocument().GetTaskRunner(TaskType::kDOMManipulation), FROM_HERE,
        WTF::Bind(&HTMLDetailsElement::DispatchPendingEvent,
                  WrapPersistent(this), params.reason));

    Element* content = EnsureUserAgentShadowRoot().getElementById(
        shadow_element_names::kIdDetailsContent);
    DCHECK(content);

    if (RuntimeEnabledFeatures::AutoExpandDetailsElementEnabled()) {
      if (is_open_) {
        content->RemoveInlineStyleProperty(CSSPropertyID::kContentVisibility);
        content->RemoveInlineStyleProperty(CSSPropertyID::kDisplay);
      } else {
        content->SetInlineStyleProperty(CSSPropertyID::kDisplay,
                                        CSSValueID::kBlock);
        content->SetInlineStyleProperty(CSSPropertyID::kContentVisibility,
                                        CSSValueID::kHidden);
        content->EnsureDisplayLockContext().SetForDetailsElement(true);
      }
    } else {
      if (is_open_) {
        content->RemoveInlineStyleProperty(CSSPropertyID::kDisplay);
      } else {
        content->SetInlineStyleProperty(CSSPropertyID::kDisplay,
                                        CSSValueID::kNone);
      }
    }

    return;
  }
  HTMLElement::ParseAttribute(params);
}

void HTMLDetailsElement::ToggleOpen() {
  setAttribute(html_names::kOpenAttr, is_open_ ? g_null_atom : g_empty_atom);
}

bool HTMLDetailsElement::IsInteractiveContent() const {
  return true;
}

}  // namespace blink
