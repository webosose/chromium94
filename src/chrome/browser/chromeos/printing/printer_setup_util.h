// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_PRINTING_PRINTER_SETUP_UTIL_H_
#define CHROME_BROWSER_CHROMEOS_PRINTING_PRINTER_SETUP_UTIL_H_

#include "base/callback_forward.h"
#include "chrome/browser/chromeos/printing/cups_printers_manager.h"
#include "chrome/browser/chromeos/printing/printer_configurer.h"
#include "chromeos/printing/printer_configuration.h"
#include "printing/backend/print_backend.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace printing {

using GetPrinterCapabilitiesCallback = base::OnceCallback<void(
    const absl::optional<PrinterSemanticCapsAndDefaults>&)>;

// Sets up a printer (if necessary) and runs a callback with the printer
// capabilities once printer setup is complete. The callback is run
// regardless of whether or not the printer needed to be set up.
// This function must be called from the UI thread.
// This function is called when setting up a printer from Print Preview
// and records a metric with the printer setup result code.
void SetUpPrinter(chromeos::CupsPrintersManager* printers_manager,
                  chromeos::PrinterConfigurer* printer_configurer,
                  const chromeos::Printer& printer,
                  GetPrinterCapabilitiesCallback cb);

}  // namespace printing

#endif  // CHROME_BROWSER_CHROMEOS_PRINTING_PRINTER_SETUP_UTIL_H_
