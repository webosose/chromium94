// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/loader/fetch/subresource_web_bundle_list.h"

#include "third_party/blink/renderer/platform/loader/fetch/subresource_web_bundle.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace blink {

void SubresourceWebBundleList::Trace(Visitor* visitor) const {
  visitor->Trace(subresource_web_bundles_);
}

void SubresourceWebBundleList::Add(SubresourceWebBundle& bundle) {
  subresource_web_bundles_.push_back(&bundle);
}

void SubresourceWebBundleList::Remove(SubresourceWebBundle& bundle) {
  subresource_web_bundles_.erase(
      std::remove_if(subresource_web_bundles_.begin(),
                     subresource_web_bundles_.end(),
                     [&bundle](auto& item) { return item == &bundle; }),
      subresource_web_bundles_.end());
}

SubresourceWebBundle* SubresourceWebBundleList::GetMatchingBundle(
    const KURL& url) const {
  for (auto it = subresource_web_bundles_.rbegin();
       it != subresource_web_bundles_.rend(); ++it) {
    if ((*it)->CanHandleRequest(url)) {
      return *it;
    }
  }
  return nullptr;
}

}  // namespace blink
