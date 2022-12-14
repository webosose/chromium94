// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_PRINTING_HISTORY_PRINT_JOB_REPORTING_SERVICE_FACTORY_H_
#define CHROME_BROWSER_CHROMEOS_PRINTING_HISTORY_PRINT_JOB_REPORTING_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace chromeos {
class PrintJobReportingService;

// Singleton that owns all PrintJobReportingServices and associates them with
// Profiles.
class PrintJobReportingServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the PrintJobReportingService for |context|, creating it if it is
  // not yet created.
  static PrintJobReportingService* GetForBrowserContext(
      content::BrowserContext* context);

  static PrintJobReportingServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<PrintJobReportingServiceFactory>;

  PrintJobReportingServiceFactory();
  PrintJobReportingServiceFactory(const PrintJobReportingServiceFactory&) =
      delete;
  PrintJobReportingServiceFactory& operator=(
      const PrintJobReportingServiceFactory&) = delete;
  ~PrintJobReportingServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_PRINTING_HISTORY_PRINT_JOB_REPORTING_SERVICE_FACTORY_H_
