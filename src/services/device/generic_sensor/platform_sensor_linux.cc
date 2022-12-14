// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/generic_sensor/platform_sensor_linux.h"

#include "base/bind.h"
#include "services/device/generic_sensor/linux/sensor_data_linux.h"
#include "services/device/generic_sensor/platform_sensor_reader_linux.h"

namespace device {

namespace {

// Checks if at least one value has been changed.
bool HaveValuesChanged(const SensorReading& lhs, const SensorReading& rhs) {
  for (size_t i = 0; i < SensorReadingRaw::kValuesCount; ++i) {
    if (lhs.raw.values[i] != rhs.raw.values[i])
      return true;
  }
  return false;
}

}  // namespace

PlatformSensorLinux::PlatformSensorLinux(
    mojom::SensorType type,
    SensorReadingSharedBuffer* reading_buffer,
    PlatformSensorProvider* provider,
    const SensorInfoLinux* sensor_device)
    : PlatformSensor(type, reading_buffer, provider),
      default_configuration_(
          PlatformSensorConfiguration(sensor_device->device_frequency)),
      reporting_mode_(sensor_device->reporting_mode) {
  sensor_reader_ =
      SensorReader::Create(*sensor_device, weak_factory_.GetWeakPtr());
}

PlatformSensorLinux::~PlatformSensorLinux() {
  DCHECK(main_task_runner()->RunsTasksInCurrentSequence());
}

mojom::ReportingMode PlatformSensorLinux::GetReportingMode() {
  DCHECK(main_task_runner()->RunsTasksInCurrentSequence());
  return reporting_mode_;
}

void PlatformSensorLinux::UpdatePlatformSensorReading(SensorReading reading) {
  DCHECK(main_task_runner()->RunsTasksInCurrentSequence());
  if (GetReportingMode() == mojom::ReportingMode::ON_CHANGE &&
      !HaveValuesChanged(reading, old_values_)) {
    return;
  }
  old_values_ = reading;
  reading.raw.timestamp =
      (base::TimeTicks::Now() - base::TimeTicks()).InSecondsF();
  UpdateSharedBufferAndNotifyClients(reading);
}

void PlatformSensorLinux::NotifyPlatformSensorError() {
  DCHECK(main_task_runner()->RunsTasksInCurrentSequence());
  NotifySensorError();
}

bool PlatformSensorLinux::StartSensor(
    const PlatformSensorConfiguration& configuration) {
  DCHECK(main_task_runner()->RunsTasksInCurrentSequence());
  sensor_reader_->StartFetchingData(configuration);
  return true;
}

void PlatformSensorLinux::StopSensor() {
  DCHECK(main_task_runner()->RunsTasksInCurrentSequence());
  sensor_reader_->StopFetchingData();
}

bool PlatformSensorLinux::CheckSensorConfiguration(
    const PlatformSensorConfiguration& configuration) {
  DCHECK(main_task_runner()->RunsTasksInCurrentSequence());
  return configuration.frequency() > 0 &&
         configuration.frequency() <= default_configuration_.frequency();
}

PlatformSensorConfiguration PlatformSensorLinux::GetDefaultConfiguration() {
  DCHECK(main_task_runner()->RunsTasksInCurrentSequence());
  return default_configuration_;
}

}  // namespace device
