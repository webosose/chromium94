// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_CACHED_NAVIGATION_URL_LOADER_H_
#define CONTENT_BROWSER_LOADER_CACHED_NAVIGATION_URL_LOADER_H_

#include "content/browser/loader/navigation_url_loader.h"

namespace content {

// NavigationURLLoader for navigations that activate a back-forward cached or
// prerendered page. Unlike a normal navigation, no actual URL loading occurs.
// This is because loading already happened the first time this URL was
// navigated to (before it was added to the back-forward cache or when it was
// prerendered).
class CachedNavigationURLLoader : public NavigationURLLoader {
 public:
  CachedNavigationURLLoader(
      LoaderType loader_type,
      std::unique_ptr<NavigationRequestInfo> request_info,
      NavigationURLLoaderDelegate* delegate,
      network::mojom::URLResponseHeadPtr cached_response_head);
  ~CachedNavigationURLLoader() override;

  static std::unique_ptr<NavigationURLLoader> Create(
      LoaderType loader_type,
      std::unique_ptr<NavigationRequestInfo> request_info,
      NavigationURLLoaderDelegate* delegate,
      network::mojom::URLResponseHeadPtr cached_response_head);

  // NavigationURLLoader implementation.
  void Start() override;
  void FollowRedirect(
      const std::vector<std::string>& removed_headers,
      const net::HttpRequestHeaders& modified_headers,
      const net::HttpRequestHeaders& modified_cors_exempt_headers,
      blink::PreviewsState new_previews_state) override;

 private:
  void OnResponseStarted();

  const LoaderType loader_type_;
  std::unique_ptr<NavigationRequestInfo> request_info_;
  NavigationURLLoaderDelegate* delegate_;
  network::mojom::URLResponseHeadPtr cached_response_head_;
  base::WeakPtrFactory<CachedNavigationURLLoader> weak_factory_{this};
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_CACHED_NAVIGATION_URL_LOADER_H_
