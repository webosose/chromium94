// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

GEN('#include "ash/constants/ash_features.h"');
GEN('#include "content/public/test/browser_test.h"')

const HOST_ORIGIN = 'chrome://demo-mode-app';

var DemoModeAppUIBrowserTest = class extends testing.Test {
  /** @override */
  get browsePreload() {
    return HOST_ORIGIN;
  }

  /** @override */
  get runAccessibilityChecks() {
    return false;
  }

  /** @override */
  get featureList() {
    return {enabled: ['chromeos::features::kDemoModeSWA']};
  }
};

// Tests that chrome://demo-mode-app runs js file and that it goes
// somewhere instead of 404ing or crashing.
TEST_F('DemoModeAppUIBrowserTest', 'HasChromeSchemeURL', () => {
  const header = document.querySelector('h1');

  assertEquals(header.innerText, 'Demo Mode App');
  assertEquals(document.location.origin, HOST_ORIGIN);
});
