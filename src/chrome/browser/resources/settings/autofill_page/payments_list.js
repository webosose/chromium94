// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'payments-list' is a list of saved payment methods (credit
 * cards etc.) to be shown in the settings page.
 */

import 'chrome://resources/cr_elements/shared_vars_css.m.js';
import '../settings_shared_css.js';
import './credit_card_list_entry.js';
import './passwords_shared_css.js';
import './upi_id_list_entry.js';

import {html, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {loadTimeData} from '../i18n_setup.js';



/** @polymer */
class SettingsPaymentsListElement extends PolymerElement {
  static get is() {
    return 'settings-payments-list';
  }

  static get template() {
    return html`{__html_template__}`;
  }

  static get properties() {
    return {
      /**
       * An array of all saved credit cards.
       * @type {!Array<!chrome.autofillPrivate.CreditCardEntry>}
       */
      creditCards: Array,

      /**
       * An array of all saved UPI Virtual Payment Addresses.
       * @type {!Array<!string>}
       */
      upiIds: Array,

      /**
       * True if displaying UPI IDs in settings is enabled.
       */
      enableUpiIds_: {
        type: Boolean,
        value() {
          return loadTimeData.getBoolean('showUpiIdSettings');
        },
      },

      /**
       * True iff both credit cards and UPI IDs will be shown.
       */
      showCreditCardUpiSeparator_: {
        type: Boolean,
        value: false,
        computed: 'computeShowCreditCardUpiSeparator_(' +
            'creditCards, upiIds, enableUpiIds_)',
      },

      /**
       * True iff any payment methods will be shown.
       */
      showAnyPaymentMethods_: {
        type: Boolean,
        value: false,
        computed:
            'computeShowAnyPaymentMethods_(creditCards, upiIds, enableUpiIds_)',
      },
    };
  }

  /**
   * Returns true if the list exists and has items.
   * @param {Array<?>} list
   * @return {boolean}
   * @private
   */
  hasSome_(list) {
    return !!(list && list.length);
  }

  /**
   * Returns true iff there are credit cards to be shown.
   * @return {boolean}
   * @private
   */
  showCreditCards_() {
    return this.hasSome_(this.creditCards);
  }

  /**
   * Returns true iff both credit cards and UPI IDs will be shown.
   * @return {boolean}
   * @private
   */
  computeShowCreditCardUpiSeparator_() {
    return this.showCreditCards_() && this.showUpiIds_();
  }

  /**
   * Returns true iff there UPI IDs and they should be shown.
   * @return {boolean}
   * @private
   */
  showUpiIds_() {
    return this.enableUpiIds_ && this.hasSome_(this.upiIds);
  }

  /**
   * Returns true iff any payment methods will be shown.
   * @return {boolean}
   * @private
   */
  computeShowAnyPaymentMethods_() {
    return this.showCreditCards_() || this.showUpiIds_();
  }
}

customElements.define(
    SettingsPaymentsListElement.is, SettingsPaymentsListElement);
