// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'chooser-exception-list-entry' shows a single chooser exception for a given
 * chooser type.
 */
import 'chrome://resources/cr_elements/shared_vars_css.m.js';
import 'chrome://resources/cr_elements/shared_style_css.m.js';
import 'chrome://resources/polymer/v3_0/iron-flex-layout/iron-flex-layout-classes.js';
import 'chrome://resources/polymer/v3_0/iron-list/iron-list.js';
import '../settings_shared_css.js';
import './site_list_entry.js';

import {focusWithoutInk} from 'chrome://resources/js/cr/ui/focus_without_ink.m.js';
import {html, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {ChooserException} from './site_settings_prefs_browser_proxy.js';

/** @polymer */
class ChooserExceptionListEntryElement extends PolymerElement {
  static get is() {
    return 'chooser-exception-list-entry';
  }

  static get template() {
    return html`{__html_template__}`;
  }

  static get properties() {
    return {
      /**
       * Chooser exception object to display in the widget.
       * @type {!ChooserException}
       */
      exception: Object,

      /** @private */
      lastFocused_: Object,
    };
  }
}

customElements.define(
    ChooserExceptionListEntryElement.is, ChooserExceptionListEntryElement);
