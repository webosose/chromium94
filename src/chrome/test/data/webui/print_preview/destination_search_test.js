// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {Destination, DestinationConnectionStatus, DestinationOrigin, DestinationStore, DestinationType, NativeLayer, NativeLayerImpl, PrintPreviewDestinationDialogElement} from 'chrome://print/print_preview.js';
import {assert} from 'chrome://resources/js/assert.m.js';
import {NativeEventTarget as EventTarget} from 'chrome://resources/js/cr/event_target.m.js';
import {flush} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {assertEquals, assertNotEquals} from '../chai_assert.js';
import {eventToPromise} from '../test_util.m.js';

import {NativeLayerStub} from './native_layer_stub.js';
import {createDestinationStore, getCddTemplate, setupTestListenerElement} from './print_preview_test_utils.js';

window.destination_search_test = {};
const destination_search_test = window.destination_search_test;
destination_search_test.suiteName = 'DestinationSearchTest';
/** @enum {string} */
destination_search_test.TestNames = {
  GetCapabilitiesSucceeds: 'get capabilities succeeds',
  GetCapabilitiesFails: 'get capabilities fails',
};

suite(destination_search_test.suiteName, function() {
  /** @type {PrintPreviewDestinationDialogElement} */
  let dialog;

  /** @type {DestinationStore} */
  let destinationStore;

  /** @type {NativeLayerStub} */
  let nativeLayer;

  /** @override */
  suiteSetup(function() {
    setupTestListenerElement();
  });

  /** @override */
  setup(function() {
    // Create data classes
    nativeLayer = new NativeLayerStub();
    NativeLayerImpl.instance_ = nativeLayer;
    destinationStore = createDestinationStore();
    nativeLayer.setLocalDestinationCapabilities(
        getCddTemplate('FooDevice', 'FooName'));
    destinationStore.init(
        false /* pdfPrinterDisabled */, false /* isDriveMounted */,
        'FooDevice' /* printerName */,
        '' /* serializedDefaultDestinationSelectionRulesStr */,
        [] /* recentDestinations */);

    // Set up dialog
    dialog = /** @type {!PrintPreviewDestinationDialogElement} */ (
        document.createElement('print-preview-destination-dialog'));
    dialog.users = [];
    dialog.activeUser = '';
    dialog.destinationStore = destinationStore;
    document.body.innerHTML = '';
    document.body.appendChild(dialog);
    return nativeLayer.whenCalled('getPrinterCapabilities').then(function() {
      dialog.show();
      flush();
      nativeLayer.reset();
    });
  });

  /**
   * @param {!Destination} destination The destination to
   *     simulate selection of.
   */
  function simulateDestinationSelect(destination) {
    // Fake destinationListItem.
    const item = document.createElement('print-preview-destination-list-item');
    item.destination = destination;

    // Get print list and fire event.
    const list =
        dialog.shadowRoot.querySelector('print-preview-destination-list');
    list.fire('destination-selected', item);
  }

  /**
   * Adds a destination to the dialog and simulates selection of the
   * destination.
   * @param {string} destId The ID for the destination.
   */
  function requestSetup(destId) {
    const dest = new Destination(
        destId, DestinationType.LOCAL, DestinationOrigin.LOCAL, 'displayName',
        DestinationConnectionStatus.ONLINE);

    // Add the destination to the list.
    simulateDestinationSelect(dest);
  }

  // Tests that a destination is selected if the user clicks on it and
  // capabilities fetch succeeds.
  test(
      assert(destination_search_test.TestNames.GetCapabilitiesSucceeds),
      function() {
        const destId = '00112233DEADBEEF';
        nativeLayer.setLocalDestinationCapabilities(getCddTemplate(destId));

        const waiter = eventToPromise(
            DestinationStore.EventType.DESTINATION_SELECT,
            /** @type {!EventTarget} */ (destinationStore));
        requestSetup(destId);
        return Promise
            .all([nativeLayer.whenCalled('getPrinterCapabilities'), waiter])
            .then(function(results) {
              const actualId = results[0].destinationId;
              assertEquals(destId, actualId);
              // After setup or capabilities fetch succeeds, the destination
              // should be selected.
              assertNotEquals(null, destinationStore.selectedDestination);
              assertEquals(destId, destinationStore.selectedDestination.id);
            });
      });

  // Tests what happens when capabilities cannot be retrieved for the chosen
  // destination. The destination will still be selected in this case.
  test(
      assert(destination_search_test.TestNames.GetCapabilitiesFails),
      function() {
        const destId = '001122DEADBEEF';
        nativeLayer.setLocalDestinationCapabilities(
            getCddTemplate(destId), true);
        requestSetup(destId);
        return nativeLayer.whenCalled('getPrinterCapabilities')
            .then(function(args) {
              assertEquals(destId, args.destinationId);
              // The destination is selected even though capabilities cannot be
              // retrieved.
              assertEquals(destId, destinationStore.selectedDestination.id);
            });
      });
});
