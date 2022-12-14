// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_PROXIMITY_AUTH_SMARTLOCK_STATE_H_
#define CHROMEOS_COMPONENTS_PROXIMITY_AUTH_SMARTLOCK_STATE_H_

#include <ostream>

namespace proximity_auth {

// Full set of states for the Smart Lock feature. Used to control which UI
// elements are visible.
enum class SmartLockState {
  // Smart Lock Feature is disabled.
  kDisabled,
  // Screen is not locked.
  kInactive,
  // Smart Lock unavailable because Bluetooth is disabled.
  kBluetoothDisabled,
  // A phone eligible to unlock the local device is found, but it does not have
  // a lock screen enabled and therefore cannot unlock the local device.
  kPhoneNotLockable,
  // No phones eligible to unlock the local device can be found.
  kPhoneNotFound,
  // Currently establishing a connection to phone.
  kConnectingToPhone,
  // A phone eligible to unlock the local device is found, but it cannot be
  // authenticated.
  kPhoneNotAuthenticated,
  // An eligible phone is found, but is both locked and too distant (signal
  // strength is too low, indicating that the phone is ~30+ feet away and is
  // therefore not allowed to unlock the device).
  kPhoneFoundLockedAndDistant,
  // An eligible phone is found and is close enough, but is locked and cannot
  // unlock the local device.
  kPhoneFoundLockedAndProximate,
  // An eligible phone is found and is unlocked, but is too distant to unlock
  // the local device.
  kPhoneFoundUnlockedAndDistant,
  // Phone is authenticated, and the local device can be unlocked.
  kPhoneAuthenticated,
  // The user must reauthenticate using their password because a sufficient time
  // has elapsed since their last password entry.
  kPasswordReentryRequired,
  // The primary user profile is either in the background or this user is a
  // secondary user profile.
  kPrimaryUserAbsent
};

std::ostream& operator<<(std::ostream& stream, const SmartLockState& state);

}  // namespace proximity_auth

#endif  // CHROMEOS_COMPONENTS_PROXIMITY_AUTH_SMARTLOCK_STATE_H_
