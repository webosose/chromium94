// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import './accelerator_view.js';
import './accelerator_edit_view.js';
import './shortcut_customization_shared_css.js';

import 'chrome://resources/cr_elements/cr_button/cr_button.m.js';
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.m.js';
import 'chrome://resources/cr_elements/cr_input/cr_input.m.js';

import {html, PolymerElement, flush} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {AcceleratorInfo} from './shortcut_types.js';

/**
 * @fileoverview
 * 'accelerator-edit-dialog' is a dialog that displays the accelerators for
 * a given shortcut. Allows users to edit the accelerators.
 * TODO(jimmyxgong): Implement editing accelerators.
 */
export class AcceleratorEditDialogElement extends PolymerElement {
  static get is() {
    return 'accelerator-edit-dialog';
  }

  static get template() {
    return html`{__html_template__}`;
  }

  static get properties() {
    return {
      description: {
        type: String,
        value: '',
      },

      /** @type {!Array<!AcceleratorInfo>} */
      acceleratorInfos: {
        type: Array,
        value: () => {},
      },

      /** @private */
      isAddingNewAccelerator_: {
        type: Boolean,
        value: false,
      },
    }
  }

  /** @override */
  connectedCallback() {
    super.connectedCallback();
    this.$.editDialog.showModal();
  }

  /** @protected */
  onDoneButtonClicked_() {
    this.$.editDialog.close();
  }

  /** @protected */
  onDialogClose_() {
    this.dispatchEvent(new CustomEvent('edit-dialog-closed',
        {bubbles: true, composed: true}));
  }

  /** @protected */
  onAddAcceleratorClicked_() {
    this.isAddingNewAccelerator_ = true;

    // Flush the dom so that the AcceleratorEditView is ready to be focused.
    flush();
    const editView = this.$.editDialog.querySelector('#pendingAccelerator');
    const accelItem = editView.shadowRoot.querySelector("#acceleratorItem");
    accelItem.shadowRoot.querySelector('#container').focus();
  }

}

customElements.define(AcceleratorEditDialogElement.is,
                      AcceleratorEditDialogElement);