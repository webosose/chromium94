// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import '//resources/cr_elements/cr_radio_button/cr_radio_button_style_css.m.js';
import '//resources/cr_elements/policy/cr_policy_pref_indicator.m.js';
import '//resources/polymer/v3_0/iron-a11y-keys-behavior/iron-a11y-keys-behavior.js';
import '../settings_shared_css.js';

import {CrRadioButtonBehavior, CrRadioButtonBehaviorInterface} from '//resources/cr_elements/cr_radio_button/cr_radio_button_behavior.m.js';
import {assert} from '//resources/js/assert.m.js';
import {html, mixinBehaviors, PolymerElement} from '//resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {prefToString} from '../prefs/pref_util.js';

import {PrefControlBehavior, PrefControlBehaviorInterface} from './pref_control_behavior.js';

/**
 * @constructor
 * @extends {PolymerElement}
 * @implements {CrRadioButtonBehaviorInterface}
 * @implements {PrefControlBehaviorInterface}
 */
const ControlledRadioButtonElementBase = mixinBehaviors(
    [PrefControlBehavior, CrRadioButtonBehavior], PolymerElement);

/** @polymer */
class ControlledRadioButtonElement extends ControlledRadioButtonElementBase {
  static get is() {
    return 'controlled-radio-button';
  }

  static get template() {
    return html`{__html_template__}`;
  }

  static get observers() {
    return [
      'updateDisabled_(pref.enforcement)',
    ];
  }

  /** @private */
  updateDisabled_() {
    this.disabled =
        this.pref.enforcement === chrome.settingsPrivate.Enforcement.ENFORCED;
  }

  /**
   * @return {boolean}
   * @private
   */
  showIndicator_() {
    return this.disabled && this.name === prefToString(assert(this.pref));
  }

  /**
   * @param {!Event} e
   * @private
   */
  onIndicatorTap_(e) {
    // Disallow <controlled-radio-button on-click="..."> when disabled.
    e.preventDefault();
    e.stopPropagation();
  }
}

customElements.define(
    ControlledRadioButtonElement.is, ControlledRadioButtonElement);
