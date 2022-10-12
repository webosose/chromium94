// Copyright 2022 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "media/capture/video/webos/video_capture_device_webos.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "media/capture/video/webos/webos_camera_service.h"
#include "media/capture/video/webos/webos_capture_delegate.h"

namespace media {

namespace {

int TranslatePowerLineFrequencyToWebOS(PowerLineFrequency frequency) {
  switch (frequency) {
    case PowerLineFrequency::FREQUENCY_50HZ:
      return 1;
    case PowerLineFrequency::FREQUENCY_60HZ:
      return 2;
    default:
      return 3;
  }
}

}  // namespace

VideoCaptureDeviceWebOS::VideoCaptureDeviceWebOS(
    scoped_refptr<WebOSCameraService> camera_service,
    const VideoCaptureDeviceDescriptor& device_descriptor)
    : camera_service_(camera_service),
      device_descriptor_(device_descriptor),
      camera_capture_thread_("CameraCaptureThread") {
  VLOG(1) << __func__ << " this[" << this << "]";
}

VideoCaptureDeviceWebOS::~VideoCaptureDeviceWebOS() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (camera_capture_thread_.IsRunning())
    camera_capture_thread_.Stop();

  VLOG(1) << __func__ << " this[" << this << "]";
}

void VideoCaptureDeviceWebOS::AllocateAndStart(
    const VideoCaptureParams& params,
    std::unique_ptr<VideoCaptureDevice::Client> client) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(1) << __func__;

  DCHECK(!capture_impl_);
  if (camera_capture_thread_.IsRunning())
    return;  // Wrong state.

  camera_capture_thread_.Start();

  const int line_frequency =
      TranslatePowerLineFrequencyToWebOS(GetPowerLineFrequency(params));
  capture_impl_ = std::make_unique<WebOSCaptureDelegate>(
      camera_service_, device_descriptor_, camera_capture_thread_.task_runner(),
      line_frequency, rotation_);
  if (!capture_impl_) {
    client->OnError(VideoCaptureError::
                        kDeviceCaptureLinuxFailedToCreateVideoCaptureDelegate,
                    FROM_HERE, "Failed to create VideoCaptureDelegate");
    return;
  }

  camera_capture_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&WebOSCaptureDelegate::AllocateAndStart,
                     capture_impl_->GetWeakPtr(),
                     camera_capture_thread_.GetThreadId(),
                     params.requested_format.frame_size.width(),
                     params.requested_format.frame_size.height(),
                     params.requested_format.frame_rate, std::move(client)));

  for (auto& request : photo_requests_queue_) {
    camera_capture_thread_.task_runner()->PostTask(FROM_HERE,
                                                   std::move(request));
  }
  photo_requests_queue_.clear();
}

void VideoCaptureDeviceWebOS::StopAndDeAllocate() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(1) << __func__;

  if (!camera_capture_thread_.IsRunning())
    return;  // Wrong state.

  camera_capture_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&WebOSCaptureDelegate::StopAndDeAllocate,
                                capture_impl_->GetWeakPtr(),
                                camera_capture_thread_.GetThreadId()));
  camera_capture_thread_.task_runner()->DeleteSoon(FROM_HERE,
                                                   capture_impl_.release());
  camera_capture_thread_.Stop();
  capture_impl_ = nullptr;
}

void VideoCaptureDeviceWebOS::TakePhoto(TakePhotoCallback callback) {
  VLOG(1) << __func__;

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(capture_impl_);
  auto functor =
      base::BindOnce(&WebOSCaptureDelegate::TakePhoto,
                     capture_impl_->GetWeakPtr(), std::move(callback));
  if (!camera_capture_thread_.IsRunning()) {
    // We have to wait until we get the device AllocateAndStart()ed.
    photo_requests_queue_.push_back(std::move(functor));
    return;
  }
  camera_capture_thread_.task_runner()->PostTask(FROM_HERE, std::move(functor));
}

void VideoCaptureDeviceWebOS::GetPhotoState(GetPhotoStateCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(1) << __func__;

  auto functor =
      base::BindOnce(&WebOSCaptureDelegate::GetPhotoState,
                     capture_impl_->GetWeakPtr(), std::move(callback));
  if (!camera_capture_thread_.IsRunning()) {
    // We have to wait until we get the device AllocateAndStart()ed.
    photo_requests_queue_.push_back(std::move(functor));
    return;
  }
  camera_capture_thread_.task_runner()->PostTask(FROM_HERE, std::move(functor));
}

void VideoCaptureDeviceWebOS::SetPhotoOptions(
    mojom::PhotoSettingsPtr settings,
    SetPhotoOptionsCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(1) << __func__;

  auto functor = base::BindOnce(&WebOSCaptureDelegate::SetPhotoOptions,
                                capture_impl_->GetWeakPtr(),
                                std::move(settings), std::move(callback));
  if (!camera_capture_thread_.IsRunning()) {
    // We have to wait until we get the device AllocateAndStart()ed.
    photo_requests_queue_.push_back(std::move(functor));
    return;
  }
  camera_capture_thread_.task_runner()->PostTask(FROM_HERE, std::move(functor));
}

void VideoCaptureDeviceWebOS::SetRotation(int rotation) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  rotation_ = rotation;
  if (camera_capture_thread_.IsRunning()) {
    camera_capture_thread_.task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&WebOSCaptureDelegate::SetRotation,
                                  capture_impl_->GetWeakPtr(), rotation));
  }
}

}  // namespace media
