// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * `settings-checkbox` is a checkbox that controls a supplied preference.
 */
import 'chrome://resources/cr_elements/cr_checkbox/cr_checkbox.m.js';
import 'chrome://resources/cr_elements/policy/cr_policy_pref_indicator.m.js';
import '../settings_shared_css.js';

import {html, mixinBehaviors, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {SettingsBooleanControlBehavior, SettingsBooleanControlBehaviorInterface} from './settings_boolean_control_behavior.js';

/**
 * @constructor
 * @extends {PolymerElement}
 * @implements {SettingsBooleanControlBehaviorInterface}
 */
const SettingsCheckboxElementBase =
    mixinBehaviors([SettingsBooleanControlBehavior], PolymerElement);

/** @polymer */
export class SettingsCheckboxElement extends SettingsCheckboxElementBase {
  static get is() {
    return 'settings-checkbox';
  }

  static get template() {
    return html`{__html_template__}`;
  }

  static get properties() {
    return {
      /**
       * Alternative source for the sub-label that can contain html markup.
       * Only use with trusted input.
       */
      subLabelHtml: {
        type: String,
        value: '',
        observer: 'onSubLabelHtmlChanged_',
      },
    };
  }

  static get observers() {
    return [
      'onSubLabelChanged_(subLabel, subLabelHtml)',
    ];
  }

  /** @private */
  onSubLabelChanged_() {
    this.$.checkbox.ariaDescription = this.$.subLabel.textContent;
  }

  /**
   * Don't let clicks on a link inside the secondary label reach the checkbox.
   * @private
   */
  onSubLabelHtmlChanged_() {
    const links = this.root.querySelectorAll('.secondary.label a');
    links.forEach((link) => {
      link.addEventListener('click', this.stopPropagation);
    });
  }

  /**
   * @param {!Event} event
   * @private
   */
  stopPropagation(event) {
    event.stopPropagation();
  }

  /**
   * @param {string} subLabel
   * @param {string} subLabelHtml
   * @return {boolean} Whether there is a subLabel
   * @private
   */
  hasSubLabel_(subLabel, subLabelHtml) {
    return !!subLabel || !!subLabelHtml;
  }
}

customElements.define(SettingsCheckboxElement.is, SettingsCheckboxElement);
