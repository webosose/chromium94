// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_CHROMEOS_FAKE_ASH_TEST_CHROME_BROWSER_MAIN_EXTRA_PARTS_H_
#define CHROME_TEST_BASE_CHROMEOS_FAKE_ASH_TEST_CHROME_BROWSER_MAIN_EXTRA_PARTS_H_

#include "chrome/browser/chrome_browser_main_extra_parts.h"

namespace test {

class FakeAshTestChromeBrowserMainExtraParts
    : public ChromeBrowserMainExtraParts {
 public:
  FakeAshTestChromeBrowserMainExtraParts();
  FakeAshTestChromeBrowserMainExtraParts(
      const FakeAshTestChromeBrowserMainExtraParts&) = delete;
  FakeAshTestChromeBrowserMainExtraParts& operator=(
      const FakeAshTestChromeBrowserMainExtraParts&) = delete;
  ~FakeAshTestChromeBrowserMainExtraParts() override;

  void PostBrowserStart() override;
};

}  // namespace test

#endif  // CHROME_TEST_BASE_CHROMEOS_FAKE_ASH_TEST_CHROME_BROWSER_MAIN_EXTRA_PARTS_H_
