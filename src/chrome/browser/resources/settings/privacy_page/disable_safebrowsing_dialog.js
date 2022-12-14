// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'disable-safebrowsing-dialog' makes sure users want to disable safebrowsing.
 */
import 'chrome://resources/cr_elements/cr_button/cr_button.m.js';
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.m.js';

import {html, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';


/** @polymer */
export class SettingsDisableSafebrowsingDialogElement extends PolymerElement {
  static get is() {
    return 'settings-disable-safebrowsing-dialog';
  }

  static get template() {
    return html`{__html_template__}`;
  }



  /** @override */
  connectedCallback() {
    super.connectedCallback();

    this.$.dialog.showModal();
  }

  /** @return {boolean} Whether the user confirmed the dialog. */
  wasConfirmed() {
    return /** @type {!CrDialogElement} */ (this.$.dialog)
               .getNative()
               .returnValue === 'success';
  }

  /** @private */
  onDialogCancel_() {
    this.$.dialog.cancel();
  }

  /** @private */
  onDialogConfirm_() {
    this.$.dialog.close();
  }
}

customElements.define(
    SettingsDisableSafebrowsingDialogElement.is,
    SettingsDisableSafebrowsingDialogElement);
