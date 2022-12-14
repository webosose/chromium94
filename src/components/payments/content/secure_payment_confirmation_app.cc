// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/secure_payment_confirmation_app.h"

#include <sstream>
#include <utility>

#include "base/base64.h"
#include "base/base64url.h"
#include "base/check.h"
#include "base/containers/flat_tree.h"
#include "base/feature_list.h"
#include "base/json/json_writer.h"
#include "base/metrics/histogram_functions.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/payments/content/payment_request_spec.h"
#include "components/payments/core/method_strings.h"
#include "components/payments/core/payer_data.h"
#include "components/webauthn/core/browser/internal_authenticator.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "crypto/sha2.h"
#include "device/fido/fido_transport_protocol.h"
#include "device/fido/fido_types.h"
#include "device/fido/public_key_credential_descriptor.h"
#include "third_party/blink/public/mojom/webauthn/authenticator.mojom.h"
#include "url/url_constants.h"

namespace payments {
namespace {

static constexpr int kDefaultTimeoutMinutes = 3;

// Records UMA metric for the system prompt result.
void RecordSystemPromptResult(
    const SecurePaymentConfirmationSystemPromptResult result) {
  base::UmaHistogramEnumeration(
      "PaymentRequest.SecurePaymentConfirmation.Funnel.SystemPromptResult",
      result);
}

}  // namespace

SecurePaymentConfirmationApp::SecurePaymentConfirmationApp(
    content::WebContents* web_contents_to_observe,
    const std::string& effective_relying_party_identity,
    std::unique_ptr<SkBitmap> icon,
    const std::u16string& label,
    std::vector<uint8_t> credential_id,
    const url::Origin& merchant_origin,
    base::WeakPtr<PaymentRequestSpec> spec,
    mojom::SecurePaymentConfirmationRequestPtr request,
    std::unique_ptr<autofill::InternalAuthenticator> authenticator)
    : PaymentApp(/*icon_resource_id=*/0, PaymentApp::Type::INTERNAL),
      content::WebContentsObserver(web_contents_to_observe),
      authenticator_frame_routing_id_(
          authenticator->GetRenderFrameHost()->GetGlobalId()),
      effective_relying_party_identity_(effective_relying_party_identity),
      icon_(std::move(icon)),
      label_(label),
      credential_id_(std::move(credential_id)),
      merchant_origin_(merchant_origin),
      spec_(spec),
      request_(std::move(request)),
      authenticator_(std::move(authenticator)) {
  DCHECK(web_contents_to_observe->GetMainFrame()->GetGlobalId() ==
         authenticator_frame_routing_id_);
  DCHECK(!credential_id_.empty());

  app_method_names_.insert(methods::kSecurePaymentConfirmation);
}

SecurePaymentConfirmationApp::~SecurePaymentConfirmationApp() = default;

void SecurePaymentConfirmationApp::InvokePaymentApp(
    base::WeakPtr<Delegate> delegate) {
  if (!authenticator_ || !spec_)
    return;

  DCHECK(spec_->IsInitialized());

  auto options = blink::mojom::PublicKeyCredentialRequestOptions::New();
  options->relying_party_id = effective_relying_party_identity_;
  options->timeout = request_->timeout.has_value()
                         ? request_->timeout.value()
                         : base::TimeDelta::FromMinutes(kDefaultTimeoutMinutes);
  options->user_verification = device::UserVerificationRequirement::kRequired;
  std::vector<device::PublicKeyCredentialDescriptor> credentials;

  if (base::FeatureList::IsEnabled(features::kSecurePaymentConfirmationDebug)) {
    options->user_verification =
        device::UserVerificationRequirement::kPreferred;
    // The `device::PublicKeyCredentialDescriptor` constructor with 2 parameters
    // enables authentication through all protocols.
    credentials.emplace_back(device::CredentialType::kPublicKey,
                             credential_id_);
  } else {
    // Enable authentication only through internal authenticators by default.
    credentials.emplace_back(device::CredentialType::kPublicKey, credential_id_,
                             base::flat_set<device::FidoTransportProtocol>{
                                 device::FidoTransportProtocol::kInternal});
  }

  options->allow_credentials = std::move(credentials);

  options->challenge = request_->challenge;
  authenticator_->SetPaymentOptions(blink::mojom::PaymentOptions::New(
      spec_->GetTotal(/*selected_app=*/this)->amount.Clone(),
      request_->instrument.Clone()));

  // We are nullifying the security check by design, and the origin that created
  // the credential isn't saved anywhere.
  authenticator_->SetEffectiveOrigin(url::Origin::Create(
      GURL(base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                         effective_relying_party_identity_}))));

  authenticator_->GetAssertion(
      std::move(options),
      base::BindOnce(&SecurePaymentConfirmationApp::OnGetAssertion,
                     weak_ptr_factory_.GetWeakPtr(), delegate));
}

bool SecurePaymentConfirmationApp::IsCompleteForPayment() const {
  return true;
}

uint32_t SecurePaymentConfirmationApp::GetCompletenessScore() const {
  // This value is used for sorting multiple apps, but this app always appears
  // on its own.
  return 0;
}

bool SecurePaymentConfirmationApp::CanPreselect() const {
  return true;
}

std::u16string SecurePaymentConfirmationApp::GetMissingInfoLabel() const {
  NOTREACHED();
  return std::u16string();
}

bool SecurePaymentConfirmationApp::HasEnrolledInstrument() const {
  // If there's no platform authenticator, then the factory should not create
  // this app. Therefore, this function can always return true.
  return true;
}

void SecurePaymentConfirmationApp::RecordUse() {
  NOTIMPLEMENTED();
}

bool SecurePaymentConfirmationApp::NeedsInstallation() const {
  return false;
}

std::string SecurePaymentConfirmationApp::GetId() const {
  return base::Base64Encode(credential_id_);
}

std::u16string SecurePaymentConfirmationApp::GetLabel() const {
  return label_;
}

std::u16string SecurePaymentConfirmationApp::GetSublabel() const {
  return std::u16string();
}

const SkBitmap* SecurePaymentConfirmationApp::icon_bitmap() const {
  return icon_.get();
}

bool SecurePaymentConfirmationApp::IsValidForModifier(
    const std::string& method,
    bool supported_networks_specified,
    const std::set<std::string>& supported_networks) const {
  bool is_valid = false;
  IsValidForPaymentMethodIdentifier(method, &is_valid);
  return is_valid;
}

base::WeakPtr<PaymentApp> SecurePaymentConfirmationApp::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

bool SecurePaymentConfirmationApp::HandlesShippingAddress() const {
  return false;
}

bool SecurePaymentConfirmationApp::HandlesPayerName() const {
  return false;
}

bool SecurePaymentConfirmationApp::HandlesPayerEmail() const {
  return false;
}

bool SecurePaymentConfirmationApp::HandlesPayerPhone() const {
  return false;
}

bool SecurePaymentConfirmationApp::IsWaitingForPaymentDetailsUpdate() const {
  return false;
}

void SecurePaymentConfirmationApp::UpdateWith(
    mojom::PaymentRequestDetailsUpdatePtr details_update) {
  NOTREACHED();
}

void SecurePaymentConfirmationApp::OnPaymentDetailsNotUpdated() {
  NOTREACHED();
}

void SecurePaymentConfirmationApp::AbortPaymentApp(
    base::OnceCallback<void(bool)> abort_callback) {
  std::move(abort_callback).Run(/*abort_success=*/false);
}

mojom::PaymentResponsePtr
SecurePaymentConfirmationApp::SetAppSpecificResponseFields(
    mojom::PaymentResponsePtr response) const {
  response->secure_payment_confirmation =
      mojom::SecurePaymentConfirmationResponse::New(response_->info.Clone(),
                                                    response_->signature,
                                                    response_->user_handle);
  return response;
}

void SecurePaymentConfirmationApp::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  if (content::RenderFrameHost::FromID(authenticator_frame_routing_id_) ==
      render_frame_host) {
    // The authenticator requires to be deleted before the render frame.
    authenticator_.reset();
  }
}

void SecurePaymentConfirmationApp::OnGetAssertion(
    base::WeakPtr<Delegate> delegate,
    blink::mojom::AuthenticatorStatus status,
    blink::mojom::GetAssertionAuthenticatorResponsePtr response) {
  if (!delegate)
    return;

  if (status != blink::mojom::AuthenticatorStatus::SUCCESS || !response) {
    std::stringstream status_string_stream;
    status_string_stream << status;
    delegate->OnInstrumentDetailsError(base::StringPrintf(
        "Authenticator returned %s.", status_string_stream.str().c_str()));
    RecordSystemPromptResult(
        SecurePaymentConfirmationSystemPromptResult::kCanceled);
    return;
  }

  RecordSystemPromptResult(
      SecurePaymentConfirmationSystemPromptResult::kAccepted);

  response_ = std::move(response);

  delegate->OnInstrumentDetailsReady(methods::kSecurePaymentConfirmation, "{}",
                                     PayerData());
}

}  // namespace payments
