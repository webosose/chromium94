// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// clang-format off
// #import 'chrome://os-settings/chromeos/os_settings.js';

// #import {ConsentStatus, DspHotwordState} from 'chrome://os-settings/chromeos/os_settings.js';
// #import {CrSettingsPrefs} from 'chrome://os-settings/chromeos/os_settings.js';
// #import {GoogleAssistantBrowserProxyImpl} from 'chrome://os-settings/chromeos/os_settings.js';
// #import {Router, routes} from 'chrome://os-settings/chromeos/os_settings.js';
// #import {TestBrowserProxy} from '../../test_browser_proxy.js';
// #import {assertEquals, assertFalse, assertTrue} from '../../chai_assert.js';
// #import {flush} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
// #import {getDeepActiveElement} from 'chrome://resources/js/util.m.js';
// #import {waitAfterNextRender} from 'chrome://test/test_util.m.js';
// clang-format on

/**
 * @implements {settings.GoogleAssistantBrowserProxy}
 */
class TestGoogleAssistantBrowserProxy extends TestBrowserProxy {
  constructor() {
    super([
      'showGoogleAssistantSettings',
      'retrainAssistantVoiceModel',
      'syncVoiceModelStatus',
    ]);
  }

  /** @override */
  showGoogleAssistantSettings() {
    this.methodCalled('showGoogleAssistantSettings');
  }

  /** @override */
  retrainAssistantVoiceModel() {
    this.methodCalled('retrainAssistantVoiceModel');
  }

  /** @override */
  syncVoiceModelStatus() {
    this.methodCalled('syncVoiceModelStatus');
  }
}

suite('GoogleAssistantHandler', function() {
  /** @type {SettingsGoogleAssistantPageElement} */
  let page = null;

  /** @type {?TestGoogleAssistantBrowserProxy} */
  let browserProxy = null;

  suiteSetup(function() {
    loadTimeData.overrideValues({
      isAssistantAllowed: true,
      hotwordDspAvailable: true,
    });
  });

  setup(function() {
    browserProxy = new TestGoogleAssistantBrowserProxy();
    settings.GoogleAssistantBrowserProxyImpl.instance_ = browserProxy;

    PolymerTest.clearBody();

    const prefElement = document.createElement('settings-prefs');
    document.body.appendChild(prefElement);

    return CrSettingsPrefs.initialized.then(function() {
      page = document.createElement('settings-google-assistant-page');
      page.prefs = prefElement.prefs;
      document.body.appendChild(page);
    });
  });

  teardown(function() {
    page.remove();
  });

  test('toggleAssistant', function() {
    Polymer.dom.flush();
    const button = page.$$('#google-assistant-enable');
    assertTrue(!!button);
    assertFalse(button.disabled);
    assertFalse(button.checked);

    // Tap the enable toggle button and ensure the state becomes enabled.
    button.click();
    Polymer.dom.flush();
    assertTrue(button.checked);
  });

  test('toggleAssistantContext', function() {
    let button = page.$$('#google-assistant-context-enable');
    assertFalse(!!button);
    page.setPrefValue('settings.voice_interaction.enabled', true);
    page.setPrefValue('settings.voice_interaction.context.enabled', false);
    Polymer.dom.flush();
    button = page.$$('#google-assistant-context-enable');
    assertTrue(!!button);
    assertFalse(button.disabled);
    assertFalse(button.checked);

    button.click();
    Polymer.dom.flush();
    assertTrue(button.checked);
    assertTrue(
        page.getPref('settings.voice_interaction.context.enabled.value'));
  });

  test('toggleAssistantHotword', function() {
    let button = page.$$('#google-assistant-hotword-enable');
    assertFalse(!!button);
    page.setPrefValue('settings.voice_interaction.enabled', true);
    page.setPrefValue('settings.voice_interaction.hotword.enabled', false);
    Polymer.dom.flush();
    button = page.$$('#google-assistant-hotword-enable');
    assertTrue(!!button);
    assertFalse(button.disabled);
    assertFalse(button.checked);

    button.click();
    Polymer.dom.flush();
    assertTrue(button.checked);
    assertTrue(
        page.getPref('settings.voice_interaction.hotword.enabled.value'));
    return browserProxy.whenCalled('syncVoiceModelStatus');
  });

  test('hotwordToggleVisibility', function() {
    let button = page.$$('#google-assistant-hotword-enable');
    assertFalse(!!button);

    page.setPrefValue('settings.voice_interaction.enabled', true);
    Polymer.dom.flush();

    button = page.$$('#google-assistant-hotword-enable');
    assertTrue(!!button);
  });

  test('hotwordToggleDisabledForChildUser', function() {
    page.setPrefValue('settings.voice_interaction.enabled', true);
    page.set('prefs.settings.voice_interaction.hotword.enabled', {
      enforcement: chrome.settingsPrivate.Enforcement.ENFORCED,
      controlledBy: chrome.settingsPrivate.ControlledBy.CHILD_RESTRICTION,
      value: false,
    });

    Polymer.dom.flush();
    const button = page.$$('#google-assistant-hotword-enable');
    const indicator = page.$$('#google-assistant-hotword-enable')
                          .$$('cr-policy-pref-indicator');
    assertTrue(!!button);
    assertTrue(!!indicator);
    assertTrue(button.disabled);
  });

  test('tapOnRetrainVoiceModel', function() {
    let button = page.$$('#retrain-voice-model');
    assertFalse(!!button);
    page.setPrefValue('settings.voice_interaction.enabled', true);
    page.setPrefValue('settings.voice_interaction.hotword.enabled', true);
    page.setPrefValue(
        'settings.voice_interaction.activity_control.consent_status',
        ConsentStatus.kActivityControlAccepted);
    Polymer.dom.flush();
    button = page.$$('#retrain-voice-model');
    assertTrue(!!button);

    button.click();
    Polymer.dom.flush();
    return browserProxy.whenCalled('retrainAssistantVoiceModel');
  });

  test('retrainButtonVisibility', function() {
    let button = page.$$('#retrain-voice-model');
    assertFalse(!!button);
    page.setPrefValue('settings.voice_interaction.enabled', true);
    Polymer.dom.flush();
    button = page.$$('#retrain-voice-model');
    assertFalse(!!button);

    // Hotword disabled.
    // Button should not be shown.
    page.setPrefValue('settings.voice_interaction.hotword.enabled', false);
    Polymer.dom.flush();
    button = page.$$('#retrain-voice-model');
    assertFalse(!!button);

    // Hotword enabled.
    // Button should be shown.
    page.setPrefValue('settings.voice_interaction.hotword.enabled', true);
    Polymer.dom.flush();
    button = page.$$('#retrain-voice-model');
    assertTrue(!!button);
  });

  test('Deep link to retrain voice model', async () => {
    loadTimeData.overrideValues({isDeepLinkingEnabled: true});
    assertTrue(loadTimeData.getBoolean('isDeepLinkingEnabled'));

    page.setPrefValue('settings.voice_interaction.enabled', true);
    page.setPrefValue('settings.voice_interaction.hotword.enabled', true);
    page.setPrefValue(
        'settings.voice_interaction.activity_control.consent_status',
        ConsentStatus.kActivityControlAccepted);
    Polymer.dom.flush();

    const params = new URLSearchParams;
    params.append('settingId', '607');
    settings.Router.getInstance().navigateTo(
        settings.routes.GOOGLE_ASSISTANT, params);

    const deepLinkElement = page.$$('#retrain-voice-model').$$('cr-button');
    await test_util.waitAfterNextRender(deepLinkElement);
    assertEquals(
        deepLinkElement, getDeepActiveElement(),
        'Retrain model button should be focused for settingId=607.');
  });

  test('toggleAssistantNotification', function() {
    let button = page.$$('#google-assistant-notification-enable');
    assertFalse(!!button);
    page.setPrefValue('settings.voice_interaction.enabled', true);
    page.setPrefValue('settings.voice_interaction.notification.enabled', false);
    Polymer.dom.flush();
    button = page.$$('#google-assistant-notification-enable');
    assertTrue(!!button);
    assertFalse(button.disabled);
    assertFalse(button.checked);

    button.click();
    Polymer.dom.flush();
    assertTrue(button.checked);
    assertTrue(
        page.getPref('settings.voice_interaction.notification.enabled.value'));
  });

  test('toggleAssistantLaunchWithMicOpen', function() {
    let button = page.$$('#google-assistant-launch-with-mic-open');
    assertFalse(!!button);
    page.setPrefValue('settings.voice_interaction.enabled', true);
    page.setPrefValue('settings.voice_interaction.launch_with_mic_open', false);
    Polymer.dom.flush();
    button = page.$$('#google-assistant-launch-with-mic-open');
    assertTrue(!!button);
    assertFalse(button.disabled);
    assertFalse(button.checked);

    button.click();
    Polymer.dom.flush();
    assertTrue(button.checked);
    assertTrue(
        page.getPref('settings.voice_interaction.launch_with_mic_open.value'));
  });

  test('tapOnAssistantSettings', function() {
    let button = page.$$('#google-assistant-settings');
    assertFalse(!!button);
    page.setPrefValue('settings.voice_interaction.enabled', true);
    Polymer.dom.flush();
    button = page.$$('#google-assistant-settings');
    assertTrue(!!button);

    button.click();
    Polymer.dom.flush();
    return browserProxy.whenCalled('showGoogleAssistantSettings');
  });

  test('assistantDisabledByPolicy', function() {
    let button = page.$$('#google-assistant-enable');
    assertTrue(!!button);
    assertFalse(button.disabled);
    assertFalse(button.checked);
    page.setPrefValue('settings.voice_interaction.enabled', true);
    Polymer.dom.flush();
    button = page.$$('#google-assistant-enable');
    assertTrue(!!button);
    assertFalse(button.disabled);
    assertTrue(button.checked);

    page.setPrefValue('settings.assistant.disabled_by_policy', true);
    Polymer.dom.flush();
    assertTrue(!!button);
    assertTrue(button.disabled);
    assertFalse(button.checked);
  });
});

suite('GoogleAssistantHandlerWithNoDspHotword', function() {
  /** @type {SettingsGoogleAssistantPageElement} */
  let page = null;

  /** @type {?TestGoogleAssistantBrowserProxy} */
  let browserProxy = null;

  suiteSetup(function() {
    loadTimeData.overrideValues({
      isAssistantAllowed: true,
      hotwordDspAvailable: false,
    });
  });

  setup(function() {
    browserProxy = new TestGoogleAssistantBrowserProxy();
    settings.GoogleAssistantBrowserProxyImpl.instance_ = browserProxy;

    PolymerTest.clearBody();

    const prefElement = document.createElement('settings-prefs');
    document.body.appendChild(prefElement);

    return CrSettingsPrefs.initialized.then(function() {
      page = document.createElement('settings-google-assistant-page');
      page.prefs = prefElement.prefs;
      document.body.appendChild(page);
      Polymer.dom.flush();
    });
  });

  teardown(function() {
    page.remove();
  });

  /**
   * @param {!HTMLElement} select
   * @param {!value} string
   */
  function selectValue(select, value) {
    select.value = value;
    select.dispatchEvent(new CustomEvent('change'));
    Polymer.dom.flush();
  }

  test('hotwordToggleVisibilityWithNoDspHotword', function() {
    let toggle = page.$$('#google-assistant-hotword-enable');
    assertFalse(!!toggle);

    page.setPrefValue('settings.voice_interaction.enabled', true);
    Polymer.dom.flush();

    toggle = page.$$('#google-assistant-hotword-enable');
    assertFalse(!!toggle);
  });

  test('dspHotwordDropdownVisibilityWithNoDspHotword', function() {
    let container = page.$$('#dsp-hotword-container');
    assertFalse(!!container);

    page.setPrefValue('settings.voice_interaction.enabled', true);
    Polymer.dom.flush();

    container = page.$$('#dsp-hotword-container');
    assertTrue(!!container);
  });

  test('dspHotwordDropdownIndicatorEnabled', function() {
    let indicator = page.$$('#hotword-policy-pref-indicator');
    assertFalse(!!indicator);

    page.setPrefValue('settings.voice_interaction.enabled', true);
    page.set('prefs.settings.voice_interaction.hotword.enabled', {
      enforcement: chrome.settingsPrivate.Enforcement.RECOMMENDED,
      value: true,
    });

    Polymer.dom.flush();
    const dropdown = page.$$('#dsp-hotword-state');
    indicator = page.$$('#hotword-policy-pref-indicator');
    assertTrue(!!dropdown);
    assertFalse(!!indicator);
    assertFalse(dropdown.hasAttribute('disabled'));
  });

  test('dspHotwordDropdownIndicatorDisabled', function() {
    let indicator = page.$$('#hotword-policy-pref-indicator');
    assertFalse(!!indicator);

    page.setPrefValue('settings.voice_interaction.enabled', true);
    page.set('prefs.settings.voice_interaction.hotword.enabled', {
      enforcement: chrome.settingsPrivate.Enforcement.ENFORCED,
      value: true,
    });

    Polymer.dom.flush();
    const dropdown = page.$$('#dsp-hotword-state');
    indicator = page.$$('#hotword-policy-pref-indicator');
    assertTrue(!!dropdown);
    assertTrue(!!indicator);
    assertTrue(dropdown.hasAttribute('disabled'));
  });

  test('dspHotwordDropdownDisabledForChildUser', function() {
    let indicator = page.$$('#hotword-policy-pref-indicator');
    assertFalse(!!indicator);

    page.setPrefValue('settings.voice_interaction.enabled', true);
    page.set('prefs.settings.voice_interaction.hotword.enabled', {
      enforcement: chrome.settingsPrivate.Enforcement.ENFORCED,
      controlledBy: chrome.settingsPrivate.ControlledBy.CHILD_RESTRICTION,
      value: false,
    });

    Polymer.dom.flush();
    const dropdown = page.$$('#dsp-hotword-state');
    indicator = page.$$('#hotword-policy-pref-indicator');
    assertTrue(!!dropdown);
    assertTrue(!!indicator);
    assertTrue(dropdown.disabled);
  });

  test('dspHotwordDropdownSelection', function() {
    let dropdown = page.$$('#dsp-hotword-state');
    assertFalse(!!dropdown);

    page.setPrefValue('settings.voice_interaction.enabled', true);
    Polymer.dom.flush();

    dropdown = page.$$('#dsp-hotword-state');
    assertTrue(!!dropdown);
    assertFalse(dropdown.disabled);

    selectValue(dropdown, DspHotwordState.DEFAULT_ON);
    Polymer.dom.flush();
    assertTrue(
        page.getPref('settings.voice_interaction.hotword.enabled.value'));
    assertFalse(
        page.getPref('settings.voice_interaction.hotword.always_on.value'));

    selectValue(dropdown, DspHotwordState.ALWAYS_ON);
    Polymer.dom.flush();
    assertTrue(
        page.getPref('settings.voice_interaction.hotword.enabled.value'));
    assertTrue(
        page.getPref('settings.voice_interaction.hotword.always_on.value'));

    selectValue(dropdown, DspHotwordState.OFF);
    Polymer.dom.flush();
    assertFalse(
        page.getPref('settings.voice_interaction.hotword.enabled.value'));
    assertFalse(
        page.getPref('settings.voice_interaction.hotword.always_on.value'));
  });

  test('dspHotwordDropdownStatus', function() {
    let dropdown = page.$$('#dsp-hotword-state');
    assertFalse(!!dropdown);

    page.setPrefValue('settings.voice_interaction.enabled', true);
    Polymer.dom.flush();

    dropdown = page.$$('#dsp-hotword-state');
    assertTrue(!!dropdown);
    assertFalse(dropdown.disabled);

    page.setPrefValue('settings.voice_interaction.hotword.enabled', true);
    page.setPrefValue('settings.voice_interaction.hotword.always_on', false);
    Polymer.dom.flush();
    assertEquals(Number(dropdown.value), DspHotwordState.DEFAULT_ON);

    page.setPrefValue('settings.voice_interaction.hotword.enabled', true);
    page.setPrefValue('settings.voice_interaction.hotword.always_on', true);
    Polymer.dom.flush();
    assertEquals(Number(dropdown.value), DspHotwordState.ALWAYS_ON);

    page.setPrefValue('settings.voice_interaction.hotword.enabled', false);
    page.setPrefValue('settings.voice_interaction.hotword.always_on', false);
    Polymer.dom.flush();
    assertEquals(Number(dropdown.value), DspHotwordState.OFF);
  });

  test('dspHotwordDropdownDefaultOnSync', function() {
    let dropdown = page.$$('#dsp-hotword-state');
    assertFalse(!!dropdown);

    page.setPrefValue('settings.voice_interaction.enabled', true);
    Polymer.dom.flush();

    dropdown = page.$$('#dsp-hotword-state');
    assertTrue(!!dropdown);
    assertFalse(dropdown.disabled);
    selectValue(dropdown, DspHotwordState.OFF);
    Polymer.dom.flush();

    selectValue(dropdown, DspHotwordState.DEFAULT_ON);
    Polymer.dom.flush();
    return browserProxy.whenCalled('syncVoiceModelStatus');
  });

  test('dspHotwordDropdownAlwaysOnSync', function() {
    let dropdown = page.$$('#dsp-hotword-state');
    assertFalse(!!dropdown);

    page.setPrefValue('settings.voice_interaction.enabled', true);
    Polymer.dom.flush();

    dropdown = page.$$('#dsp-hotword-state');
    assertTrue(!!dropdown);
    assertFalse(dropdown.disabled);
    selectValue(dropdown, DspHotwordState.OFF);
    Polymer.dom.flush();

    selectValue(dropdown, DspHotwordState.ALWAYS_ON);
    Polymer.dom.flush();
    return browserProxy.whenCalled('syncVoiceModelStatus');
  });
});
