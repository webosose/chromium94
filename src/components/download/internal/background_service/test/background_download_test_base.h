// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_TEST_BACKGROUND_DOWNLOAD_TEST_BASE_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_TEST_BACKGROUND_DOWNLOAD_TEST_BASE_H_

#include <memory>

#include "base/files/scoped_temp_dir.h"
#include "base/sequence_checker.h"
#include "base/test/task_environment.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/platform_test.h"

namespace download {
namespace test {

// Shared test harness for multiple background download tests that send real
// network requests to embedded test server.
class BackgroundDownloadTestBase : public PlatformTest {
 protected:
  static const char* kDefaultResponseContent;
  BackgroundDownloadTestBase();
  ~BackgroundDownloadTestBase() override;

  // PlatformTest overrides.
  void SetUp() override;

  const net::test_server::HttpRequest* request_sent() const {
    return request_sent_.get();
  }
  const base::ScopedTempDir& dir() const { return dir_; }

 protected:
  std::unique_ptr<net::test_server::HttpResponse> DefaultResponse(
      const net::test_server::HttpRequest& request);

  base::test::TaskEnvironment task_environment_;
  net::EmbeddedTestServer server_;
  net::test_server::EmbeddedTestServerHandle server_handle_;
  std::unique_ptr<net::test_server::HttpRequest> request_sent_;
  base::ScopedTempDir dir_;
  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace test
}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_TEST_BACKGROUND_DOWNLOAD_TEST_BASE_H_
