// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/quick_pair/repository/http_fetcher.h"

#include "ash/quick_pair/common/logging.h"
#include "ash/quick_pair/common/quick_pair_browser_delegate.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace {

// Max size set to 2MB.  This is well over the expected maximum for our
// expected responses, however it can be increased if needed in the future.
constexpr int kMaxDownloadBytes = 2 * 1024 * 1024;

// Helper function to extract response code from |SimpleURLLoader|.
int GetResponseCode(network::SimpleURLLoader* simple_loader) {
  if (simple_loader->ResponseInfo() && simple_loader->ResponseInfo()->headers) {
    return simple_loader->ResponseInfo()->headers->response_code();
  }

  return -1;
}

}  // namespace

namespace ash {
namespace quick_pair {

HttpFetcher::HttpFetcher(
    const net::NetworkTrafficAnnotationTag& traffic_annotation)
    : traffic_annotation_(traffic_annotation) {}

HttpFetcher::~HttpFetcher() = default;

void HttpFetcher::ExecuteGetRequest(const GURL& url,
                                    FetchCompleteCallback callback) {
  QP_LOG(VERBOSE) << __func__ << ": executing request to: " << url;

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = url;
  resource_request->method = "GET";
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;

  auto loader = network::SimpleURLLoader::Create(std::move(resource_request),
                                                 traffic_annotation_);
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory =
      QuickPairBrowserDelegate::Get()->GetURLLoaderFactory();
  if (!url_loader_factory) {
    QP_LOG(WARNING) << __func__ << ": No SharedURLLoaderFactory is available.";
    std::move(callback).Run(nullptr);
    return;
  }

  auto* loader_ptr = loader.get();
  loader_ptr->DownloadToString(
      url_loader_factory.get(),
      base::BindOnce(&HttpFetcher::OnComplete, weak_ptr_factory_.GetWeakPtr(),
                     std::move(loader), std::move(callback)),
      kMaxDownloadBytes);
}

void HttpFetcher::OnComplete(
    std::unique_ptr<network::SimpleURLLoader> simple_loader,
    FetchCompleteCallback callback,
    std::unique_ptr<std::string> response_body) {
  if (simple_loader->NetError() == net::OK && response_body) {
    QP_LOG(VERBOSE) << "Successfully fetched "
                    << simple_loader->GetContentSize() << " bytes from "
                    << simple_loader->GetFinalURL();
    std::move(callback).Run(std::move(response_body));
    return;
  }

  QP_LOG(WARNING) << "Downloading to string from "
                  << simple_loader->GetFinalURL() << " failed with error code: "
                  << GetResponseCode(simple_loader.get())
                  << " with network error: " << simple_loader->NetError();

  // TODO(jonmann): Implement retries with back-off.
  std::move(callback).Run(nullptr);
}

}  // namespace quick_pair
}  // namespace ash
