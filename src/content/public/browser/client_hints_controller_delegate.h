// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_CLIENT_HINTS_CONTROLLER_DELEGATE_H_
#define CONTENT_PUBLIC_BROWSER_CLIENT_HINTS_CONTROLLER_DELEGATE_H_

#include <memory>

#include "base/time/time.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "third_party/blink/public/common/client_hints/enabled_client_hints.h"
#include "url/origin.h"

class GURL;

namespace blink {
class EnabledClientHints;
struct UserAgentMetadata;
}  // namespace blink

namespace network {
class NetworkQualityTracker;
}  // namespace network

namespace content {

class CONTENT_EXPORT ClientHintsControllerDelegate {
 public:
  virtual ~ClientHintsControllerDelegate() = default;

  virtual network::NetworkQualityTracker* GetNetworkQualityTracker() = 0;

  // Get which client hints opt-ins were persisted on current origin.
  virtual void GetAllowedClientHintsFromSource(
      const GURL& url,
      blink::EnabledClientHints* client_hints) = 0;

  virtual bool IsJavaScriptAllowed(const GURL& url) = 0;

  virtual blink::UserAgentMetadata GetUserAgentMetadata() = 0;

  virtual void PersistClientHints(
      const url::Origin& primary_origin,
      const std::vector<network::mojom::WebClientHintsType>& client_hints,
      base::TimeDelta expiration_duration) = 0;

  // Optionally implemented by implementations used in tests. Clears all hints
  // that would have been returned by GetAllowedClientHintsFromSource(),
  // regardless of whether they were added via PersistClientHints() or
  // SetAdditionalHints().
  virtual void ResetForTesting() {}

  // Sets additional `hints` that this delegate should add to the
  // blink::EnabledClientHints object affected by
  // |GetAllowedClientHintsFromSource|. This is for when there are additional
  // client hints to be added to a request that are not in storage.
  virtual void SetAdditionalClientHints(
      const std::vector<network::mojom::WebClientHintsType>&) = 0;

  // Clears the additional hints set by |SetAdditionalHints|.
  virtual void ClearAdditionalClientHints() = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_CLIENT_HINTS_CONTROLLER_DELEGATE_H_
