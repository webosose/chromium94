// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SEND_TAB_TO_SELF_METRICS_UTIL_H_
#define COMPONENTS_SEND_TAB_TO_SELF_METRICS_UTIL_H_

namespace send_tab_to_self {

enum class ShareEntryPoint {
  kContentMenu,
  kLinkMenu,
  kOmniboxIcon,
  kOmniboxMenu,
  kShareMenu,
  kShareSheet,
  kTabMenu,
};

// Records whether the user clicked to send a tab to a device.
void RecordDeviceClicked(ShareEntryPoint entry_point);

// Records when a tab is sent to a device.
void RecordNotificationSent();

// Records when a recieved STTS notification is shown.
void RecordNotificationShown();

// Records when a recieved STTS notification is dismissed.
void RecordNotificationDismissed();

// Records when a recieved STTS notification is opened.
void RecordNotificationOpened();

// Records when a recieved STTS notification is shown and times out.
void RecordNotificationTimedOut();

}  // namespace send_tab_to_self

#endif  // COMPONENTS_SEND_TAB_TO_SELF_METRICS_UTIL_H_
