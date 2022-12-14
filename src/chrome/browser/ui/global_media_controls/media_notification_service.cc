// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/global_media_controls/media_notification_service.h"

#include <memory>

#include "base/callback_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_functions.h"
#include "base/ranges/algorithm.h"
#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/global_media_controls/media_dialog_delegate.h"
#include "chrome/browser/ui/global_media_controls/media_notification_container_impl.h"
#include "chrome/browser/ui/global_media_controls/media_notification_device_provider_impl.h"
#include "chrome/browser/ui/global_media_controls/media_notification_service_observer.h"
#include "chrome/browser/ui/global_media_controls/media_session_notification_item.h"
#include "chrome/browser/ui/global_media_controls/media_session_notification_producer.h"
#include "chrome/browser/ui/global_media_controls/overlay_media_notification.h"
#include "chrome/browser/ui/media_router/media_router_ui.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/media_message_center/media_notification_item.h"
#include "components/media_message_center/media_notification_util.h"
#include "components/media_router/browser/presentation/start_presentation_context.h"
#include "content/public/browser/audio_service.h"
#include "content/public/browser/media_session.h"
#include "content/public/browser/media_session_service.h"
#include "media/base/media_switches.h"
#include "services/media_session/public/mojom/media_session.mojom.h"

namespace {
void CancelRequest(
    std::unique_ptr<media_router::StartPresentationContext> context,
    const std::string& message) {
  context->InvokeErrorCallback(blink::mojom::PresentationError(
      blink::mojom::PresentationErrorType::PRESENTATION_REQUEST_CANCELLED,
      message));
}
}  // namespace

MediaNotificationService::MediaNotificationService(
    Profile* profile,
    bool show_from_all_profiles) {
  media_session_notification_producer_ =
      std::make_unique<MediaSessionNotificationProducer>(
          this, profile, show_from_all_profiles);
  notification_producers_.insert(media_session_notification_producer_.get());

  if (media_router::MediaRouterEnabled(profile)) {
    if (base::FeatureList::IsEnabled(media::kGlobalMediaControlsForCast)) {
      cast_notification_producer_ =
          std::make_unique<CastMediaNotificationProducer>(
              profile, this,
              base::BindRepeating(
                  &MediaNotificationService::OnNotificationChanged,
                  base::Unretained(this)));
      notification_producers_.insert(cast_notification_producer_.get());
    }
    if (media_router::GlobalMediaControlsCastStartStopEnabled()) {
      presentation_request_notification_producer_ =
          std::make_unique<PresentationRequestNotificationProducer>(this);
      notification_producers_.insert(
          presentation_request_notification_producer_.get());
    }
  }
}

MediaNotificationService::~MediaNotificationService() = default;

void MediaNotificationService::AddObserver(
    MediaNotificationServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void MediaNotificationService::RemoveObserver(
    MediaNotificationServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

void MediaNotificationService::Shutdown() {
  // |cast_notification_producer_| and
  // |presentation_request_notification_producer_| depend on MediaRouter,
  // which is another keyed service.
  cast_notification_producer_.reset();
  presentation_request_notification_producer_.reset();
}

void MediaNotificationService::ShowItem(const std::string& id) {
  // If new notifications come up while the dialog is open for a
  // PresentationRequest, do not show the new notifications.
  if (!HasOpenDialogForPresentationRequest()) {
    ShowAndObserveContainer(id);
  }
}

void MediaNotificationService::HideItem(const std::string& id) {
  OnNotificationChanged();
  if (!dialog_delegate_) {
    return;
  }
  dialog_delegate_->HideMediaSession(id);
}

void MediaNotificationService::OnOverlayNotificationClosed(
    const std::string& id) {
  if (!media_session_notification_producer_->OnOverlayNotificationClosed(id)) {
    return;
  }
  ShowAndObserveContainer(id);
}

void MediaNotificationService::OnNotificationChanged() {
  for (auto& observer : observers_)
    observer.OnNotificationListChanged();
}

void MediaNotificationService::SetDialogDelegate(
    MediaDialogDelegate* delegate) {
  dialog_opened_from_presentation_ = false;
  SetDialogDelegateCommon(delegate);
  if (!dialog_delegate_)
    return;

  auto notification_ids = GetActiveControllableNotificationIds();
  std::list<std::string> sorted_notification_ids;
  for (const std::string& id : notification_ids) {
    if (media_session_notification_producer_->IsSessionPlaying(id)) {
      sorted_notification_ids.push_front(id);
    } else {
      sorted_notification_ids.push_back(id);
    }
  }

  for (const std::string& id : sorted_notification_ids) {
    base::WeakPtr<media_message_center::MediaNotificationItem> item =
        GetNotificationItem(id);
    MediaNotificationContainerImpl* container =
        dialog_delegate_->ShowMediaSession(id, item);
    auto* notification_producer = GetNotificationProducer(id);
    if (notification_producer)
      notification_producer->OnItemShown(id, container);
  }

  media_message_center::RecordConcurrentNotificationCount(
      notification_ids.size());

  if (cast_notification_producer_) {
    media_message_center::RecordConcurrentCastNotificationCount(
        cast_notification_producer_->GetActiveItemCount());
  }
}

void MediaNotificationService::SetDialogDelegateForWebContents(
    MediaDialogDelegate* delegate,
    content::WebContents* contents) {
  dialog_opened_from_presentation_ = true;
  SetDialogDelegateCommon(delegate);
  if (!dialog_delegate_)
    return;

  // When the dialog is opened by a PresentationRequest, there should be only
  // one notification, in the following priority order:
  // 1. A cast session associated with |contents|.
  // 2. A local media session associated with |contents|.
  // 3. A supplemental notification populated using the PresentationRequest.
  base::WeakPtr<media_message_center::MediaNotificationItem> item;
  std::string item_id;

  // Find the cast notification item associated with |contents|.
  auto routes = media_router::WebContentsPresentationManager::Get(contents)
                    ->GetMediaRoutes();
  if (!routes.empty()) {
    // It is possible for a sender page to connect to two routes. For the
    // sake of the Zenith dialog, only one notification is needed.
    item_id = routes.begin()->media_route_id();
    item = cast_notification_producer_->GetNotificationItem(item_id);
  } else if (media_session_notification_producer_
                 ->HasActiveControllableSessionForWebContents(contents)) {
    item_id = media_session_notification_producer_
                  ->GetActiveControllableSessionForWebContents(contents);
    item = GetNotificationItem(item_id);
  } else {
    auto presentation_item =
        presentation_request_notification_producer_->GetNotificationItem();
    item_id = presentation_item->id();
    item = presentation_item;
    DCHECK(presentation_request_notification_producer_->GetWebContents() ==
           contents);
  }

  DCHECK(item);
  MediaNotificationContainerImpl* container =
      dialog_delegate_->ShowMediaSession(item_id, item);
  auto* notification_producer = GetNotificationProducer(item_id);
  if (notification_producer)
    notification_producer->OnItemShown(item_id, container);
}

std::set<std::string>
MediaNotificationService::GetActiveControllableNotificationIds() const {
  std::set<std::string> ids;
  for (auto* notification_provider : notification_producers_) {
    const std::set<std::string>& provider_ids =
        notification_provider->GetActiveControllableNotificationIds();
    ids.insert(provider_ids.begin(), provider_ids.end());
  }
  return ids;
}

bool MediaNotificationService::HasActiveNotifications() const {
  return !GetActiveControllableNotificationIds().empty();
}

bool MediaNotificationService::HasActiveNotificationsForWebContents(
    content::WebContents* web_contents) const {
  bool has_media_session =
      media_session_notification_producer_ &&
      media_session_notification_producer_
          ->HasActiveControllableSessionForWebContents(web_contents);
  return HasCastNotificationsForWebContents(web_contents) || has_media_session;
}

bool MediaNotificationService::HasFrozenNotifications() const {
  return media_session_notification_producer_->HasFrozenNotifications();
}

bool MediaNotificationService::HasOpenDialog() const {
  return !!dialog_delegate_;
}

bool MediaNotificationService::HasOpenDialogForPresentationRequest() const {
  return HasOpenDialog() && dialog_opened_from_presentation_;
}

void MediaNotificationService::HideMediaDialog() {
  if (dialog_delegate_) {
    dialog_delegate_->HideMediaDialog();
  }
}

std::unique_ptr<OverlayMediaNotification>
MediaNotificationService::PopOutNotification(const std::string& id,
                                             gfx::Rect bounds) {
  return dialog_delegate_ ? dialog_delegate_->PopOut(id, bounds) : nullptr;
}

base::CallbackListSubscription
MediaNotificationService::RegisterAudioOutputDeviceDescriptionsCallback(
    MediaNotificationDeviceProvider::GetOutputDevicesCallback callback) {
  return media_session_notification_producer_
      ->RegisterAudioOutputDeviceDescriptionsCallback(std::move(callback));
}

base::CallbackListSubscription
MediaNotificationService::RegisterIsAudioOutputDeviceSwitchingSupportedCallback(
    const std::string& id,
    base::RepeatingCallback<void(bool)> callback) {
  return media_session_notification_producer_
      ->RegisterIsAudioOutputDeviceSwitchingSupportedCallback(
          id, std::move(callback));
}

void MediaNotificationService::OnStartPresentationContextCreated(
    std::unique_ptr<media_router::StartPresentationContext> context) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(
      content::RenderFrameHost::FromID(
          context->presentation_request().render_frame_host_id));
  if (!web_contents) {
    CancelRequest(std::move(context), "The web page is closed.");
    return;
  }

  // If there exists a cast notification associated with |web_contents|,
  // delete |context| because users should not start a new presentation at
  // this time.
  if (HasCastNotificationsForWebContents(web_contents)) {
    CancelRequest(std::move(context), "A presentation has already started.");
  } else if (media_session_notification_producer_
                 ->HasActiveControllableSessionForWebContents(web_contents)) {
    // If there exists a media session notification associated with
    // |web_contents|, pass |context| to |media_session_notification_producer_|.
    media_session_notification_producer_->OnStartPresentationContextCreated(
        std::move(context));
  } else if (presentation_request_notification_producer_) {
    // If there do not exist active notifications, pass |context| to
    // |presentation_request_notification_producer_| to create a dummy
    // notification.
    presentation_request_notification_producer_
        ->OnStartPresentationContextCreated(std::move(context));
  } else {
    CancelRequest(std::move(context), "Unable to start presentation.");
  }
}

std::unique_ptr<media_router::CastDialogController>
MediaNotificationService::CreateCastDialogControllerForSession(
    const std::string& id) {
  return media_session_notification_producer_
      ->CreateCastDialogControllerForSession(id);
}

std::unique_ptr<media_router::CastDialogController>
MediaNotificationService::CreateCastDialogControllerForPresentationRequest() {
  auto* web_contents =
      presentation_request_notification_producer_->GetWebContents();
  if (!web_contents)
    return nullptr;

  auto ui = std::make_unique<media_router::MediaRouterUI>(web_contents);
  if (!presentation_request_notification_producer_->GetNotificationItem()
           ->is_default_presentation_request()) {
    ui->InitWithStartPresentationContext(
        presentation_request_notification_producer_->GetNotificationItem()
            ->PassContext());
  } else {
    ui->InitWithDefaultMediaSource();
  }
  return ui;
}

void MediaNotificationService::ShowAndObserveContainer(const std::string& id) {
  OnNotificationChanged();
  if (!dialog_delegate_) {
    return;
  }
  base::WeakPtr<media_message_center::MediaNotificationItem> item =
      GetNotificationItem(id);
  MediaNotificationContainerImpl* container =
      dialog_delegate_->ShowMediaSession(id, item);
  auto* notification_producer = GetNotificationProducer(id);
  if (notification_producer)
    notification_producer->OnItemShown(id, container);
}

void MediaNotificationService::FocusOnDialog() {
  dialog_delegate_->Focus();
}

base::WeakPtr<media_message_center::MediaNotificationItem>
MediaNotificationService::GetNotificationItem(const std::string& id) {
  for (auto* notification_provider : notification_producers_) {
    auto item = notification_provider->GetNotificationItem(id);
    if (item) {
      return item;
    }
  }
  return nullptr;
}

void MediaNotificationService::SetDialogDelegateCommon(
    MediaDialogDelegate* delegate) {
  DCHECK(!delegate || !dialog_delegate_);
  dialog_delegate_ = delegate;

  if (dialog_delegate_) {
    for (auto& observer : observers_)
      observer.OnMediaDialogOpened();
  } else {
    for (auto& observer : observers_)
      observer.OnMediaDialogClosed();
  }
}

MediaNotificationProducer* MediaNotificationService::GetNotificationProducer(
    const std::string& notification_id) {
  for (auto* notification_producer : notification_producers_) {
    if (notification_producer->GetNotificationItem(notification_id)) {
      return notification_producer;
    }
  }
  return nullptr;
}

bool MediaNotificationService::HasCastNotificationsForWebContents(
    content::WebContents* web_contents) const {
  return !media_router::WebContentsPresentationManager::Get(web_contents)
              ->GetMediaRoutes()
              .empty();
}
