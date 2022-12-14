// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'zoom-levels' is the polymer element for showing the sites that are zoomed in
 * or out.
 */

import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.m.js';
import 'chrome://resources/cr_elements/icons.m.js';
import 'chrome://resources/cr_elements/shared_vars_css.m.js';
import '../settings_shared_css.js';
import '../site_favicon.js';

import {ListPropertyUpdateBehavior, ListPropertyUpdateBehaviorInterface} from 'chrome://resources/js/list_property_update_behavior.m.js';
import {WebUIListenerBehavior, WebUIListenerBehaviorInterface} from 'chrome://resources/js/web_ui_listener_behavior.m.js';
import {html, mixinBehaviors, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {loadTimeData} from '../i18n_setup.js';

import {SiteSettingsMixin, SiteSettingsMixinInterface} from './site_settings_mixin.js';
import {ZoomLevelEntry} from './site_settings_prefs_browser_proxy.js';

/**
 * @constructor
 * @extends {PolymerElement}
 * @implements {SiteSettingsMixinInterface}
 * @implements {ListPropertyUpdateBehaviorInterface}
 * @implements {WebUIListenerBehaviorInterface}
 */
const ZoomLevelsElementBase = mixinBehaviors(
    [ListPropertyUpdateBehavior, WebUIListenerBehavior],
    SiteSettingsMixin(PolymerElement));

/** @polymer */
class ZoomLevelsElement extends ZoomLevelsElementBase {
  static get is() {
    return 'zoom-levels';
  }

  static get template() {
    return html`{__html_template__}`;
  }

  static get properties() {
    return {
      /**
       * Array of sites that are zoomed in or out.
       * @type {!Array<ZoomLevelEntry>}
       */
      sites_: {
        type: Array,
        value: () => [],
      },

      /** @private */
      showNoSites_: {
        type: Boolean,
        value: false,
      },
    };
  }

  /** @override */
  ready() {
    super.ready();

    this.addWebUIListener(
        'onZoomLevelsChanged', this.onZoomLevelsChanged_.bind(this));
    this.browserProxy.fetchZoomLevels();
  }

  /**
   * A handler for when zoom levels change.
   * @param {!Array<ZoomLevelEntry>} sites The up to date list of sites and
   *     their zoom levels.
   */
  onZoomLevelsChanged_(sites) {
    this.updateList('sites_', item => item.origin, sites);
    this.showNoSites_ = this.sites_.length === 0;
  }

  /**
   * A handler for when a zoom level for a site is deleted.
   * @param {!{model: !{index: number}}} event
   * @private
   */
  removeZoomLevel_(event) {
    const site = this.sites_[event.model.index];
    this.browserProxy.removeZoomLevel(site.origin);
  }
}

customElements.define(ZoomLevelsElement.is, ZoomLevelsElement);
