// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ENTERPRISE_REPORTING_EXTENSION_REQUEST_EXTENSION_REQUEST_REPORT_GENERATOR_H_
#define CHROME_BROWSER_ENTERPRISE_REPORTING_EXTENSION_REQUEST_EXTENSION_REQUEST_REPORT_GENERATOR_H_

#include <memory>
#include <string>
#include <vector>

class Profile;

namespace extensions {
class ExtensionManagement;
}

namespace enterprise_reporting {

class ExtensionsWorkflowEvent;

class ExtensionRequestReportGenerator {
 public:
  // Extension request are moved out of the pending list once user confirm the
  // notification. However, there is no need to upload these requests anymore as
  // long as admin made a decision.
  static bool ShouldUploadExtensionRequest(
      const std::string& extension_id,
      const std::string& webstore_update_url,
      extensions::ExtensionManagement* extension_management);

  ExtensionRequestReportGenerator();
  ExtensionRequestReportGenerator(const ExtensionRequestReportGenerator&) =
      delete;
  ExtensionRequestReportGenerator& operator=(
      const ExtensionRequestReportGenerator&) = delete;
  ~ExtensionRequestReportGenerator();

  std::vector<std::unique_ptr<ExtensionsWorkflowEvent>> Generate();

  // Uploads extension request update for |profile|.
  std::vector<std::unique_ptr<ExtensionsWorkflowEvent>> GenerateForProfile(
      Profile* profile);
};

}  // namespace enterprise_reporting

#endif  // CHROME_BROWSER_ENTERPRISE_REPORTING_EXTENSION_REQUEST_EXTENSION_REQUEST_REPORT_GENERATOR_H_
