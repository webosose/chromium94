// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_PRINTING_PRINTING_SERVICE_H_
#define CHROME_SERVICES_PRINTING_PRINTING_SERVICE_H_

#include "base/macros.h"
#include "build/build_config.h"
#include "build/chromeos_buildflags.h"
#include "chrome/services/printing/public/mojom/printing_service.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace printing {

class PrintingService : public mojom::PrintingService {
 public:
  explicit PrintingService(
      mojo::PendingReceiver<mojom::PrintingService> receiver);
  ~PrintingService() override;

 private:
  // mojom::PrintingService implementation:
  void BindPdfNupConverter(
      mojo::PendingReceiver<mojom::PdfNupConverter> receiver) override;
  void BindPdfToPwgRasterConverter(
      mojo::PendingReceiver<mojom::PdfToPwgRasterConverter> receiver) override;
#if defined(OS_CHROMEOS)
  void BindPdfFlattener(
      mojo::PendingReceiver<mojom::PdfFlattener> receiver) override;
#endif
#if BUILDFLAG(IS_CHROMEOS_ASH)
  void BindPdfThumbnailer(
      mojo::PendingReceiver<mojom::PdfThumbnailer> receiver) override;
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)
#if defined(OS_WIN)
  void BindPdfToEmfConverterFactory(
      mojo::PendingReceiver<mojom::PdfToEmfConverterFactory> receiver) override;
#endif

  mojo::Receiver<mojom::PrintingService> receiver_;

  DISALLOW_COPY_AND_ASSIGN(PrintingService);
};

}  // namespace printing

#endif  // CHROME_SERVICES_PRINTING_PRINTING_SERVICE_H_
