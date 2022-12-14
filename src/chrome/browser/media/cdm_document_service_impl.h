// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_CDM_DOCUMENT_SERVICE_IMPL_H_
#define CHROME_BROWSER_MEDIA_CDM_DOCUMENT_SERVICE_IMPL_H_

#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "build/chromeos_buildflags.h"
#include "content/public/browser/document_service_base.h"
#include "media/mojo/mojom/cdm_document_service.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

#if BUILDFLAG(IS_CHROMEOS_ASH)
#include "chrome/browser/ash/attestation/platform_verification_flow.h"
#endif

#if BUILDFLAG(IS_CHROMEOS_LACROS)
#include "chromeos/crosapi/mojom/content_protection.mojom.h"
#endif  // BUILDFLAG(IS_CHROMEOS_LACROS)

// Implements media::mojom::CdmDocumentService. Can only be used on the
// UI thread because PlatformVerificationFlow and the pref service lives on the
// UI thread.
class CdmDocumentServiceImpl final
    : public content::DocumentServiceBase<media::mojom::CdmDocumentService> {
 public:
  static void Create(
      content::RenderFrameHost* render_frame_host,
      mojo::PendingReceiver<media::mojom::CdmDocumentService> receiver);

  CdmDocumentServiceImpl(
      content::RenderFrameHost* render_frame_host,
      mojo::PendingReceiver<media::mojom::CdmDocumentService> receiver);

  // media::mojom::CdmDocumentService implementation.
  void ChallengePlatform(const std::string& service_id,
                         const std::string& challenge,
                         ChallengePlatformCallback callback) final;
  void GetStorageId(uint32_t version, GetStorageIdCallback callback) final;
#if defined(OS_CHROMEOS)
  void IsVerifiedAccessEnabled(IsVerifiedAccessEnabledCallback callback) final;
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)
#if defined(OS_WIN)
  void GetCdmPreferenceData(GetCdmPreferenceDataCallback callback) final;
  void SetCdmClientToken(const std::vector<uint8_t>& client_token) final;
#endif  // defined(OS_WIN)

 private:
  // |this| can only be destructed as a DocumentServiceBase.
  ~CdmDocumentServiceImpl() final;

#if BUILDFLAG(IS_CHROMEOS_ASH)
  using PlatformVerificationResult =
      ash::attestation::PlatformVerificationFlow::Result;

  void OnPlatformChallenged(ChallengePlatformCallback callback,
                            PlatformVerificationResult result,
                            const std::string& signed_data,
                            const std::string& signature,
                            const std::string& platform_key_certificate);
#endif

#if BUILDFLAG(IS_CHROMEOS_LACROS)
  void OnPlatformChallenged(ChallengePlatformCallback callback,
                            crosapi::mojom::ChallengePlatformResultPtr result);
#endif

  void OnStorageIdResponse(GetStorageIdCallback callback,
                           const std::vector<uint8_t>& storage_id);

#if BUILDFLAG(IS_CHROMEOS_ASH)
  scoped_refptr<ash::attestation::PlatformVerificationFlow>
      platform_verification_flow_;
#endif

  content::RenderFrameHost* const render_frame_host_;
  base::WeakPtrFactory<CdmDocumentServiceImpl> weak_factory_{this};
};

#endif  // CHROME_BROWSER_MEDIA_CDM_DOCUMENT_SERVICE_IMPL_H_
