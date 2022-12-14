// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'site-data-entry' handles showing the local storage summary for a site.
 */

import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.m.js';
import 'chrome://resources/cr_elements/icons.m.js';
import 'chrome://resources/cr_elements/shared_style_css.m.js';
import 'chrome://resources/polymer/v3_0/iron-flex-layout/iron-flex-layout-classes.js';
import 'chrome://resources/polymer/v3_0/iron-icon/iron-icon.js';
import '../settings_shared_css.js';
import '../site_favicon.js';

import {FocusRowBehavior} from 'chrome://resources/js/cr/ui/focus_row_behavior.m.js';
import {I18nBehavior} from 'chrome://resources/js/i18n_behavior.m.js';
import {html, mixinBehaviors, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {MetricsBrowserProxyImpl, PrivacyElementInteractions} from '../metrics_browser_proxy.js';

import {LocalDataBrowserProxy, LocalDataBrowserProxyImpl, LocalDataItem} from './local_data_browser_proxy.js';

/**
 * @constructor
 * @extends {PolymerElement}
 */
const SiteDataEntryElementBase =
    mixinBehaviors([FocusRowBehavior, I18nBehavior], PolymerElement);

/** @polymer */
class SiteDataEntryElement extends SiteDataEntryElementBase {
  static get is() {
    return 'site-data-entry';
  }

  static get template() {
    return html`{__html_template__}`;
  }

  static get properties() {
    return {
      /** @type {!LocalDataItem} */
      model: Object,
    };
  }

  /**
   * Deletes all site data for this site.
   * @param {!Event} e
   * @private
   */
  onRemove_(e) {
    e.stopPropagation();
    MetricsBrowserProxyImpl.getInstance().recordSettingsPageHistogram(
        PrivacyElementInteractions.SITE_DATA_REMOVE_SITE);
    LocalDataBrowserProxyImpl.getInstance().removeSite(this.model.site);
  }
}

customElements.define(SiteDataEntryElement.is, SiteDataEntryElement);
