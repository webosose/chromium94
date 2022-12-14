// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_U2F_NOTIFICATION_H_
#define CHROME_BROWSER_CHROMEOS_U2F_NOTIFICATION_H_

#include <set>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace chromeos {

// Detects whether the legacy, never-officially-launched built-in U2F feature is
// enabled. If so, shows a notification to tell the user about a security issue.
class U2FNotification {
 public:
  U2FNotification();
  ~U2FNotification();

  // Asynchronously checks whether the legacy implementation is enabled and if
  // so, displays a notification.
  void Check();

 private:
  // Checks status given the current U2F flags.
  void CheckStatus(absl::optional<std::set<std::string>> flags);

  // Shows the notification.
  void ShowNotification();

  // Handles clicks on the notification.
  void OnNotificationClick(const absl::optional<int> button_index);

  base::WeakPtrFactory<U2FNotification> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(U2FNotification);
};

}  // namespace chromeos

// TODO(https://crbug.com/1164001): remove after the //chrome/browser/chromeos
// source migration is finished.
namespace ash {
using ::chromeos::U2FNotification;
}

#endif  // CHROME_BROWSER_CHROMEOS_U2F_NOTIFICATION_H_
