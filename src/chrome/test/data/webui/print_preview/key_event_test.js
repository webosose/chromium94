// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {NativeLayer, NativeLayerImpl, PluginProxyImpl, PrintPreviewAppElement} from 'chrome://print/print_preview.js';
import {assert} from 'chrome://resources/js/assert.m.js';
import {isChromeOS, isLacros, isMac, isWindows} from 'chrome://resources/js/cr.m.js';
import {keyEventOn} from 'chrome://resources/polymer/v3_0/iron-test-helpers/mock-interactions.js';
import {flush} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {assertEquals, assertTrue} from '../chai_assert.js';
import {eventToPromise, flushTasks} from '../test_util.m.js';

// <if expr="chromeos or lacros">
import {setNativeLayerCrosInstance} from './native_layer_cros_stub.js';
// </if>
import {NativeLayerStub} from './native_layer_stub.js';
import {getCddTemplateWithAdvancedSettings, getDefaultInitialSettings} from './print_preview_test_utils.js';
import {TestPluginProxy} from './test_plugin_proxy.js';

window.key_event_test = {};
const key_event_test = window.key_event_test;
key_event_test.suiteName = 'KeyEventTest';
/** @enum {string} */
key_event_test.TestNames = {
  EnterTriggersPrint: 'enter triggers print',
  NumpadEnterTriggersPrint: 'numpad enter triggers print',
  EnterOnInputTriggersPrint: 'enter on input triggers print',
  EnterOnDropdownDoesNotPrint: 'enter on dropdown does not print',
  EnterOnButtonDoesNotPrint: 'enter on button does not print',
  EnterOnCheckboxDoesNotPrint: 'enter on checkbox does not print',
  EscapeClosesDialogOnMacOnly: 'escape closes dialog on mac only',
  CmdPeriodClosesDialogOnMacOnly: 'cmd period closes dialog on mac only',
  CtrlShiftPOpensSystemDialog: 'ctrl shift p opens system dialog',
};

suite(key_event_test.suiteName, function() {
  /** @type {!PrintPreviewAppElement} */
  let page;

  /** @type {!NativeLayerStub} */
  let nativeLayer;

  /** @override */
  setup(function() {
    const initialSettings = getDefaultInitialSettings();
    nativeLayer = new NativeLayerStub();
    nativeLayer.setInitialSettings(initialSettings);
    nativeLayer.setLocalDestinations(
        [{deviceName: initialSettings.printerName, printerName: 'FooName'}]);
    // Use advanced settings so that we can test with the cr-button.
    nativeLayer.setLocalDestinationCapabilities(
        getCddTemplateWithAdvancedSettings(1, initialSettings.printerName));
    nativeLayer.setPageCount(3);
    NativeLayerImpl.instance_ = nativeLayer;
    // <if expr="chromeos or lacros">
    setNativeLayerCrosInstance();
    // </if>
    const pluginProxy = new TestPluginProxy();
    PluginProxyImpl.instance_ = pluginProxy;

    document.body.innerHTML = '';
    page = /** @type {!PrintPreviewAppElement} */ (
        document.createElement('print-preview-app'));
    document.body.appendChild(page);
    const previewArea = /** @type {!PrintPreviewPreviewAreaElement} */ (
        page.shadowRoot.querySelector('#previewArea'));

    // Wait for initialization to complete.
    return Promise
        .all([
          nativeLayer.whenCalled('getInitialSettings'),
          nativeLayer.whenCalled('getPrinterCapabilities')
        ])
        .then(function() {
          flush();
        });
  });

  // Tests that the enter key triggers a call to print.
  test(assert(key_event_test.TestNames.EnterTriggersPrint), function() {
    const whenPrintCalled = nativeLayer.whenCalled('print');
    keyEventOn(page, 'keydown', 'Enter', [], 'Enter');
    return whenPrintCalled;
  });

  // Tests that the numpad enter key triggers a call to print.
  test(assert(key_event_test.TestNames.NumpadEnterTriggersPrint), function() {
    const whenPrintCalled = nativeLayer.whenCalled('print');
    keyEventOn(page, 'keydown', 'NumpadEnter', [], 'Enter');
    return whenPrintCalled;
  });

  // Tests that the enter key triggers a call to print if an input is the
  // source of the event.
  test(assert(key_event_test.TestNames.EnterOnInputTriggersPrint), function() {
    const whenPrintCalled = nativeLayer.whenCalled('print');
    keyEventOn(
        page.shadowRoot.querySelector('print-preview-sidebar')
            .$$('print-preview-copies-settings')
            .shadowRoot.querySelector('print-preview-number-settings-section')
            .$$('cr-input')
            .inputElement,
        'keydown', 'Enter', [], 'Enter');
    return whenPrintCalled;
  });

  // Tests that the enter key does not trigger a call to print if the event
  // comes from a dropdown.
  test(
      assert(key_event_test.TestNames.EnterOnDropdownDoesNotPrint), function() {
        const whenKeyEventFired = eventToPromise('keydown', page);
        keyEventOn(
            page.shadowRoot.querySelector('print-preview-sidebar')
                .$$('print-preview-layout-settings')
                .$$('.md-select'),
            'keydown', 'Enter', [], 'Enter');
        return whenKeyEventFired.then(
            () => assertEquals(0, nativeLayer.getCallCount('print')));
      });

  // Tests that the enter key does not trigger a call to print if the event
  // comes from a button.
  test(assert(key_event_test.TestNames.EnterOnButtonDoesNotPrint), async () => {
    const moreSettingsElement =
        page.shadowRoot.querySelector('print-preview-sidebar')
            .$$('print-preview-more-settings');
    moreSettingsElement.$.label.click();
    const button = page.shadowRoot.querySelector('print-preview-sidebar')
                       .$$('print-preview-advanced-options-settings')
                       .shadowRoot.querySelector('cr-button');
    const whenKeyEventFired = eventToPromise('keydown', button);
    keyEventOn(button, 'keydown', 'Enter', [], 'Enter');
    await whenKeyEventFired;
    await flushTasks();
    assertEquals(0, nativeLayer.getCallCount('print'));
  });

  // Tests that the enter key does not trigger a call to print if the event
  // comes from a checkbox.
  test(
      assert(key_event_test.TestNames.EnterOnCheckboxDoesNotPrint), function() {
        const moreSettingsElement =
            page.shadowRoot.querySelector('print-preview-sidebar')
                .$$('print-preview-more-settings');
        moreSettingsElement.$.label.click();
        const whenKeyEventFired = eventToPromise('keydown', page);
        keyEventOn(
            page.shadowRoot.querySelector('print-preview-sidebar')
                .$$('print-preview-other-options-settings')
                .$$('cr-checkbox'),
            'keydown', 'Enter', [], 'Enter');
        return whenKeyEventFired.then(
            () => assertEquals(0, nativeLayer.getCallCount('print')));
      });

  // Tests that escape closes the dialog only on Mac.
  test(
      assert(key_event_test.TestNames.EscapeClosesDialogOnMacOnly), function() {
        const promise = isMac ?
            nativeLayer.whenCalled('dialogClose') :
            eventToPromise('keydown', page).then(() => {
              assertEquals(0, nativeLayer.getCallCount('dialogClose'));
            });
        keyEventOn(page, 'keydown', 'Escape', [], 'Escape');
        return promise;
      });

  // Tests that Cmd + Period closes the dialog only on Mac
  test(
      assert(key_event_test.TestNames.CmdPeriodClosesDialogOnMacOnly),
      function() {
        const promise = isMac ?
            nativeLayer.whenCalled('dialogClose') :
            eventToPromise('keydown', page).then(() => {
              assertEquals(0, nativeLayer.getCallCount('dialogClose'));
            });
        keyEventOn(page, 'keydown', 'Period', ['meta'], 'Period');
        return promise;
      });

  // Tests that Ctrl+Shift+P opens the system dialog.
  test(
      assert(key_event_test.TestNames.CtrlShiftPOpensSystemDialog), function() {
        let promise = null;
        if (isChromeOS || isLacros) {
          // Chrome OS doesn't have a system dialog. Just make sure the key
          // event does not trigger a crash.
          promise = Promise.resolve();
        } else if (isWindows) {
          promise = nativeLayer.whenCalled('print').then((printTicket) => {
            assertTrue(JSON.parse(printTicket).showSystemDialog);
          });
        } else {
          promise = nativeLayer.whenCalled('showSystemDialog');
        }
        const modifiers = isMac ? ['meta', 'alt'] : ['ctrl', 'shift'];
        keyEventOn(page, 'keydown', 'KeyP', modifiers, 'KeyP');
        return promise;
      });
});
