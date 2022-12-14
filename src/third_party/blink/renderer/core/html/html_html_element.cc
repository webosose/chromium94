/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann (hausmann@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2006, 2010 Apple Inc. All rights reserved.
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
 */

#include "third_party/blink/renderer/core/html/html_html_element.h"

#include "third_party/blink/renderer/core/css/style_engine.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/document_parser.h"
#include "third_party/blink/renderer/core/dom/node_computed_style.h"
#include "third_party/blink/renderer/core/frame/deprecation.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/html/html_body_element.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/loader/appcache/application_cache_host_for_frame.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/heap/heap.h"

namespace blink {

HTMLHtmlElement::HTMLHtmlElement(Document& document)
    : HTMLElement(html_names::kHTMLTag, document) {}

bool HTMLHtmlElement::IsURLAttribute(const Attribute& attribute) const {
  return attribute.GetName() == html_names::kManifestAttr ||
         HTMLElement::IsURLAttribute(attribute);
}

void HTMLHtmlElement::InsertedByParser() {
  // When parsing a fragment, its dummy document has a null parser.
  if (!GetDocument().Parser())
    return;

  MaybeSetupApplicationCache();
  if (!GetDocument().Parser())
    return;

  GetDocument().Parser()->DocumentElementAvailable();
  if (GetDocument().GetFrame()) {
    GetDocument().GetFrame()->Loader().DispatchDocumentElementAvailable();
    GetDocument().GetFrame()->Loader().RunScriptsAtDocumentElementAvailable();
    // RunScriptsAtDocumentElementAvailable might have invalidated
    // GetDocument().
  }
}

void HTMLHtmlElement::MaybeSetupApplicationCache() {
  if (!GetDocument().GetFrame())
    return;

  DocumentLoader* document_loader =
      GetDocument().GetFrame()->Loader().GetDocumentLoader();
  if (!document_loader ||
      !GetDocument().Parser()->DocumentWasLoadedAsPartOfNavigation())
    return;

  if (!GetExecutionContext()->IsSecureContext())
    return;

  ApplicationCacheHostForFrame* host =
      document_loader->GetApplicationCacheHost();
  DCHECK(host);

  const AtomicString& manifest = FastGetAttribute(html_names::kManifestAttr);
  if (manifest.IsEmpty())
    host->SelectCacheWithoutManifest();
  else
    host->SelectCacheWithManifest(GetDocument().CompleteURL(manifest));
  bool app_cache_installed =
      host->GetStatus() !=
      blink::mojom::AppCacheStatus::APPCACHE_STATUS_UNCACHED;
  if (app_cache_installed && manifest.IsEmpty()) {
    UseCounter::Count(GetDocument(),
                      WebFeature::kApplicationCacheInstalledButNoManifest);
  }
}

const CSSPropertyValueSet*
HTMLHtmlElement::AdditionalPresentationAttributeStyle() {
  if (const CSSValue* color_scheme =
          GetDocument().GetStyleEngine().GetMetaColorSchemeValue()) {
    DEFINE_STATIC_LOCAL(
        Persistent<MutableCSSPropertyValueSet>, color_scheme_style,
        (MakeGarbageCollected<MutableCSSPropertyValueSet>(kHTMLStandardMode)));
    color_scheme_style->SetProperty(CSSPropertyID::kColorScheme, *color_scheme);
    return color_scheme_style;
  }
  return nullptr;
}

namespace {

bool NeedsLayoutStylePropagation(const ComputedStyle& layout_style,
                                 const ComputedStyle& propagated_style) {
  return layout_style.GetWritingMode() != propagated_style.GetWritingMode() ||
         layout_style.Direction() != propagated_style.Direction();
}

scoped_refptr<const ComputedStyle> CreateLayoutStyle(
    const ComputedStyle& style,
    const ComputedStyle& propagated_style) {
  scoped_refptr<ComputedStyle> layout_style = ComputedStyle::Clone(style);
  layout_style->SetDirection(propagated_style.Direction());
  layout_style->SetWritingMode(propagated_style.GetWritingMode());
  return layout_style;
}

}  // namespace

scoped_refptr<const ComputedStyle> HTMLHtmlElement::LayoutStyleForElement(
    scoped_refptr<const ComputedStyle> style) {
  DCHECK(style);
  DCHECK(GetDocument().InStyleRecalc());
  DCHECK(GetLayoutObject());
  StyleResolver& resolver = GetDocument().GetStyleResolver();
  if (resolver.ShouldStopBodyPropagation(*this))
    return style;
  if (const Element* body_element = GetDocument().FirstBodyElement()) {
    if (resolver.ShouldStopBodyPropagation(*body_element))
      return style;
    if (const ComputedStyle* body_style = body_element->GetComputedStyle()) {
      if (NeedsLayoutStylePropagation(*style, *body_style))
        return CreateLayoutStyle(*style, *body_style);
    }
  }
  return style;
}

void HTMLHtmlElement::PropagateWritingModeAndDirectionFromBody() {
  if (NeedsReattachLayoutTree()) {
    // This means we are being called from RecalcStyle(). Since we need to
    // reattach the layout tree, we will re-enter this method from
    // RebuildLayoutTree().
    return;
  }
  if (Element* body_element = GetDocument().FirstBodyElement()) {
    // Same as above.
    if (body_element->NeedsReattachLayoutTree())
      return;
  }

  if (auto* layout_object = GetLayoutObject()) {
    scoped_refptr<const ComputedStyle> new_style =
        LayoutStyleForElement(layout_object->Style());
    layout_object->SetStyle(new_style);
    // We need to propagate the style to text children because the used
    // writing-mode and direction affects text children. Child elements,
    // however, inherit the computed value, which is unaffected by the
    // propagated used value from body.
    for (Node* node = firstChild(); node; node = node->nextSibling()) {
      if (!node->IsTextNode() || node->NeedsReattachLayoutTree())
        continue;
      if (LayoutObject* layout_text = node->GetLayoutObject())
        layout_text->SetStyle(new_style);
    }
  }
}

}  // namespace blink
