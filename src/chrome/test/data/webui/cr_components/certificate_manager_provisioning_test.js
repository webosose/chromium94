// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'chrome://settings/strings.m.js';

import {CertificateProvisioningViewDetailsActionEvent} from 'chrome://resources/cr_components/certificate_manager/certificate_manager_types.js';
import {CertificateProvisioningBrowserProxyImpl} from 'chrome://resources/cr_components/certificate_manager/certificate_provisioning_browser_proxy.js';
import {CertificateProvisioningDetailsDialogElement} from 'chrome://resources/cr_components/certificate_manager/certificate_provisioning_details_dialog.js';
import {CertificateProvisioningEntryElement} from 'chrome://resources/cr_components/certificate_manager/certificate_provisioning_entry.js';
import {CertificateProvisioningListElement} from 'chrome://resources/cr_components/certificate_manager/certificate_provisioning_list.js';
import {webUIListenerCallback} from 'chrome://resources/js/cr.m.js';
import {flush} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {assertEquals, assertFalse, assertTrue} from '../chai_assert.js';
import {TestBrowserProxy} from '../test_browser_proxy.js';
import {eventToPromise} from '../test_util.m.js';


/**
 * A test version of CertificateProvisioningBrowserProxy.
 * Provides helper methods for allowing tests to know when a method was called,
 * as well as specifying mock responses.
 *
 * @implements {CertificateProvisioningBrowserProxy}
 */
class TestCertificateProvisioningBrowserProxy extends TestBrowserProxy {
  constructor() {
    super([
      'refreshCertificateProvisioningProcesses',
      'triggerCertificateProvisioningProcessUpdate',
    ]);
  }

  /** override */
  refreshCertificateProvisioningProcesses() {
    this.methodCalled('refreshCertificateProvisioningProcesses');
  }

  /** override */
  triggerCertificateProvisioningProcessUpdate(certProfileId, isDeviceWide) {
    this.methodCalled(
        'triggerCertificateProvisioningProcessUpdate',
        {certProfileId, isDeviceWide});
  }
}

/**
 * @param {boolean} isUpdated
 * @return {!CertificateProvisioningProcess}
 */
function createSampleCertificateProvisioningProcess(isUpdated) {
  return {
    certProfileId: 'dummyProfileId',
    certProfileName: 'Dummy Profile Name',
    isDeviceWide: true,
    publicKey: 'dummyPublicKey',
    stateId: 8,
    status: isUpdated ? 'dummyStateName2' : 'dummyStateName',
    timeSinceLastUpdate: 'dummyTimeSinceLastUpdate',
  };
}

/**
 * @param {?CertificateProvisioningListElement} certProvisioningList
 * @return {!NodeList<!Element>}
 */
function getEntries(certProvisioningList) {
  assertTrue(!!certProvisioningList);
  return certProvisioningList.shadowRoot.querySelectorAll(
      'certificate-provisioning-entry');
}

suite('CertificateProvisioningEntryTests', function() {
  /** @type {!CertificateProvisioningEntryElement} */
  let entry;

  /** @type {?TestCertificateProvisioningBrowserProxy} */
  let browserProxy = null;

  /**
   * @return {!Promise} A promise firing once
   *     |CertificateProvisioningViewDetailsActionEvent| fires.
   */
  function actionEventToPromise() {
    return eventToPromise(CertificateProvisioningViewDetailsActionEvent, entry);
  }

  setup(function() {
    browserProxy = new TestCertificateProvisioningBrowserProxy();
    CertificateProvisioningBrowserProxyImpl.setInstance(browserProxy);
    entry = /** @type {!CertificateProvisioningEntryElement} */ (
        document.createElement('certificate-provisioning-entry'));
    entry.model = createSampleCertificateProvisioningProcess(false);
    document.body.appendChild(entry);

    // Bring up the popup menu for the following tests to use.
    entry.shadowRoot.querySelector('#dots').click();
    flush();
  });

  teardown(function() {
    entry.remove();
  });

  // Test case where 'Details' option is tapped.
  test('MenuOptions_Details', function() {
    const detailsButton = entry.shadowRoot.querySelector('#details');
    const waitForActionEvent = actionEventToPromise();
    detailsButton.click();
    return waitForActionEvent.then(function(event) {
      const detail =
          /** @type {!CertificateProvisioningActionEventDetail} */ (
              event.detail);
      assertEquals(entry.model, detail.model);
    });
  });
});

suite('CertificateManagerProvisioningTests', function() {
  /** @type {?CertificateProvisioningListElement} */
  let certProvisioningList = null;

  /** @type {?TestCertificateProvisioningBrowserProxy} */
  let browserProxy = null;

  setup(function() {
    browserProxy = new TestCertificateProvisioningBrowserProxy();
    CertificateProvisioningBrowserProxyImpl.setInstance(browserProxy);
    certProvisioningList =
        /** @type {!CertificateProvisioningListElement} */ (
            document.createElement('certificate-provisioning-list'));
    document.body.appendChild(certProvisioningList);
  });

  teardown(function() {
    certProvisioningList.remove();
  });

  /**
   * Test that the certProvisioningList requests information about certificate
   * provisioning processesfrom the browser on startup and that it gets
   * populated accordingly.
   */
  test('Initialization', function() {
    assertEquals(0, getEntries(certProvisioningList).length);

    return browserProxy.whenCalled('refreshCertificateProvisioningProcesses')
        .then(function() {
          webUIListenerCallback(
              'certificate-provisioning-processes-changed',
              [createSampleCertificateProvisioningProcess(false)]);

          flush();

          assertEquals(1, getEntries(certProvisioningList).length);
        });
  });

  test('OpensDialog_ViewDetails', function() {
    const dialogId = 'certificate-provisioning-details-dialog';
    const anchorForTest = document.createElement('a');
    document.body.appendChild(anchorForTest);

    assertFalse(!!certProvisioningList.shadowRoot.querySelector(dialogId));
    const whenDialogOpen =
        eventToPromise('cr-dialog-open', certProvisioningList);
    certProvisioningList.dispatchEvent(
        new CustomEvent(CertificateProvisioningViewDetailsActionEvent, {
          bubbles: true,
          composed: true,
          detail:
              /** @type {!CertificateProvisioningActionEventDetail} */ ({
                model: createSampleCertificateProvisioningProcess(false),
                anchor: anchorForTest
              })
        }));

    return whenDialogOpen
        .then(() => {
          const dialog =
              /** @type {!CertificateProvisioningDetailsDialogElement} */ (
                  certProvisioningList.shadowRoot.querySelector(dialogId));
          assertTrue(!!dialog);
          const whenDialogClosed = eventToPromise('close', dialog);
          dialog.shadowRoot.querySelector('#dialog')
              .shadowRoot.querySelector('#close')
              .click();
          return whenDialogClosed;
        })
        .then(() => {
          const dialog =
              certProvisioningList.shadowRoot.querySelector(dialogId);
          assertFalse(!!dialog);
        });
  });
});

suite('DetailsDialogTests', function() {
  /** @type {?TestCertificateProvisioningBrowserProxy} */
  let browserProxy = null;

  /** @type {?CertificateProvisioningListElement} */
  let certProvisioningList = null;

  let dialog = null;

  setup(async function() {
    document.body.innerHTML = '';

    browserProxy = new TestCertificateProvisioningBrowserProxy();
    CertificateProvisioningBrowserProxyImpl.setInstance(browserProxy);

    certProvisioningList =
        /** @type {!CertificateProvisioningListElement} */ (
            document.createElement('certificate-provisioning-list'));
    document.body.appendChild(certProvisioningList);

    const anchorForTest = document.createElement('a');
    document.body.appendChild(anchorForTest);

    // Open the details dialog for testing.
    const dialogId = 'certificate-provisioning-details-dialog';
    assertFalse(!!certProvisioningList.shadowRoot.querySelector(dialogId));
    const whenDialogOpen =
        eventToPromise('cr-dialog-open', certProvisioningList);
    certProvisioningList.dispatchEvent(
        new CustomEvent(CertificateProvisioningViewDetailsActionEvent, {
          bubbles: true,
          composed: true,
          detail:
              /** @type {!CertificateProvisioningActionEventDetail} */ ({
                model: createSampleCertificateProvisioningProcess(false),
                anchor: anchorForTest
              })
        }));
    await whenDialogOpen;
    dialog = /** @type {!CertificateProvisioningDetailsDialogElement} */ (
        certProvisioningList.shadowRoot.querySelector(dialogId));
    // Check if the dialog is initialized and opened.
    assertTrue(!!dialog);
    assertTrue(dialog.shadowRoot.querySelector('#dialog').open);
  });

  test('RefreshProcess', async function() {
    // Simulate clicking 'Refresh'.
    dialog.$.refresh.click();

    const {certProfileId, isDeviceWide} = await browserProxy.whenCalled(
        'triggerCertificateProvisioningProcessUpdate');
    // Check if the parameters received by function are correct.
    assertEquals(dialog.model.certProfileId, certProfileId);
    assertEquals(dialog.model.isDeviceWide, isDeviceWide);
    // Check that the dialog is still open.
    assertTrue(dialog.shadowRoot.querySelector('#dialog').open);
  });

  /**
   * Test that the details dialog gets updated correctly if the process is
   * changed after refreshCertificateProvisioningProcesses.
   */
  test('UpdateDialogDetails', async function() {
    // Start testing when dialog is open.
    await browserProxy.whenCalled('refreshCertificateProvisioningProcesses');

    // Check the status of dialog.model.
    assertEquals(dialog.model.status, 'dummyStateName');
    webUIListenerCallback(
        'certificate-provisioning-processes-changed',
        [createSampleCertificateProvisioningProcess(true)]);
    flush();
    // Check if the status of dialog.model is updated accordingly.
    assertEquals(dialog.model.status, 'dummyStateName2');
  });

  /**
   * Test that the details dialog gets closed if the process doesn't exist in
   * the list after refreshCertificateProvisioningProcesses.
   */
  test('CloseDialog', async function() {
    await browserProxy.whenCalled('refreshCertificateProvisioningProcesses');

    webUIListenerCallback('certificate-provisioning-processes-changed', []);
    flush();
    // Check that the dialog closes if the process no longer exists.
    assertFalse(dialog.shadowRoot.querySelector('#dialog').open);
  });
});
