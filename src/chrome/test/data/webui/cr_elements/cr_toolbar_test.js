// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for cr-toolbar. */

// clang-format off
import {CrToolbarElement} from 'chrome://resources/cr_elements/cr_toolbar/cr_toolbar.js';
import {assertFalse, assertTrue} from '../chai_assert.js';
import {isVisible} from '../test_util.m.js';
// clang-format on

suite('cr-toolbar', function() {
  /** @type {?CrToolbarElement} */
  let toolbar = null;

  setup(function() {
    document.documentElement.toggleAttribute('enable-branding-update', true);
    document.body.innerHTML = '';
    toolbar =
        /** @type {!CrToolbarElement} */ (document.createElement('cr-toolbar'));
    document.body.appendChild(toolbar);
  });

  test('AlwaysShowLogo', function() {
    toolbar.narrow = true;
    assertFalse(isVisible(/** @type {!HTMLElement} */ (
        toolbar.shadowRoot.querySelector('picture'))));

    toolbar.alwaysShowLogo = true;
    assertTrue(isVisible(/** @type {!HTMLElement} */ (
        toolbar.shadowRoot.querySelector('picture'))));
  });
});
