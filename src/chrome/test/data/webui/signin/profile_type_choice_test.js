// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {ProfileTypeChoiceElement} from 'chrome://profile-picker/lazy_load.js';

import {ManageProfilesBrowserProxyImpl} from 'chrome://profile-picker/profile_picker.js';
import {isLacros} from 'chrome://resources/js/cr.m.js';

import {assertEquals, assertTrue} from '../chai_assert.js';
import {isChildVisible} from '../test_util.m.js';

import {TestManageProfilesBrowserProxy} from './test_manage_profiles_browser_proxy.js';

suite('ProfileTypeChoiceTest', function() {
  /** @type {!ProfileTypeChoiceElement} */
  let choice;

  /** @type {!TestManageProfilesBrowserProxy} */
  let browserProxy;

  setup(function() {
    browserProxy = new TestManageProfilesBrowserProxy();
    ManageProfilesBrowserProxyImpl.setInstance(browserProxy);
    document.body.innerHTML = '';
    choice = /** @type {!ProfileTypeChoiceElement} */ (
        document.createElement('profile-type-choice'));
    document.body.append(choice);
  });

  test('BackButton', function() {
    assertTrue(isChildVisible(choice, '#backButton'));
  });

  test('SignInButton', function() {
    assertTrue(isChildVisible(choice, '#signInButton'));
  });

  test('NotNowButton', function() {
    // Local profile creation is not enabled on Lacros.
    assertEquals(isChildVisible(choice, '#notNowButton'), !isLacros);
  });

  test('VerifySignInPromoImpressionRecorded', function() {
    return browserProxy.whenCalled('recordSignInPromoImpression');
  });
});
