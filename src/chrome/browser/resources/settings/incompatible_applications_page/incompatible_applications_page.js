// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-incompatible-applications-page' is the settings subpage containing
 * the list of incompatible applications.
 *
 * Example:
 *
 *    <iron-animated-pages>
 *      <settings-incompatible-applications-page">
 *      </settings-incompatible-applications-page>
 *      ... other pages ...
 *    </iron-animated-pages>
 */

import 'chrome://resources/cr_elements/icons.m.js';
import 'chrome://resources/cr_elements/shared_style_css.m.js';
import 'chrome://resources/polymer/v3_0/iron-flex-layout/iron-flex-layout-classes.js';
import 'chrome://resources/polymer/v3_0/iron-icon/iron-icon.js';
import '../settings_shared_css.js';
import './incompatible_application_item.js';

import {assert} from 'chrome://resources/js/assert.m.js';
import {WebUIListenerBehavior, WebUIListenerBehaviorInterface} from 'chrome://resources/js/web_ui_listener_behavior.m.js';
import {html, mixinBehaviors, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {loadTimeData} from '../i18n_setup.js';

import {IncompatibleApplication, IncompatibleApplicationsBrowserProxyImpl} from './incompatible_applications_browser_proxy.js';


/**
 * @constructor
 * @extends {PolymerElement}
 * @implements {WebUIListenerBehaviorInterface}
 */
const SettingsIncompatibleApplicationsPageElementBase =
    mixinBehaviors([WebUIListenerBehavior], PolymerElement);

/** @polymer */
class SettingsIncompatibleApplicationsPageElement extends
    SettingsIncompatibleApplicationsPageElementBase {
  static get is() {
    return 'settings-incompatible-applications-page';
  }

  static get template() {
    return html`{__html_template__}`;
  }

  static get properties() {
    return {
      /**
       * Indicates if the current user has administrator rights.
       * @private
       */
      hasAdminRights_: {
        type: Boolean,
        value() {
          return loadTimeData.getBoolean('hasAdminRights');
        },
      },

      /**
       * The list of all the incompatible applications.
       * @private {Array<IncompatibleApplication>}
       */
      applications_: Array,

      /**
       * Determines if the user has finished with this page.
       * @private
       */
      isDone_: {
        type: Boolean,
        computed: 'computeIsDone_(applications_.*)',
      },

      /**
       * The text for the subtitle of the subpage.
       * @private
       */
      subtitleText_: {
        type: String,
        value: '',
      },

      /**
       * The text for the subtitle of the subpage, when the user does not have
       * administrator rights.
       * @private
       */
      subtitleNoAdminRightsText_: {
        type: String,
        value: '',
      },

      /**
       * The text for the title of the list of incompatible applications.
       * @private
       */
      listTitleText_: {
        type: String,
        value: '',
      },
    };
  }

  /** @override */
  ready() {
    super.ready();

    this.addWebUIListener(
        'incompatible-application-removed',
        this.onIncompatibleApplicationRemoved_.bind(this));

    IncompatibleApplicationsBrowserProxyImpl.getInstance()
        .requestIncompatibleApplicationsList()
        .then(list => {
          this.applications_ = list;
          this.updatePluralStrings_();
        });
  }

  /**
   * @return {boolean}
   * @private
   */
  computeIsDone_() {
    return this.applications_.length === 0;
  }

  /**
   * Removes a single incompatible application from the |applications_| list.
   * @private
   */
  onIncompatibleApplicationRemoved_(applicationName) {
    // Find the index of the element.
    const index = this.applications_.findIndex(function(application) {
      return application.name === applicationName;
    });

    assert(index !== -1);

    this.splice('applications_', index, 1);
  }

  /**
   * Updates the texts of the Incompatible Applications subpage that depends on
   * the length of |applications_|.
   * @private
   */
  updatePluralStrings_() {
    const browserProxy = IncompatibleApplicationsBrowserProxyImpl.getInstance();
    const numApplications = this.applications_.length;

    // The plural strings are not displayed when there is no applications.
    if (this.applications_.length === 0) {
      return;
    }

    Promise
        .all([
          browserProxy.getSubtitlePluralString(numApplications),
          browserProxy.getSubtitleNoAdminRightsPluralString(numApplications),
          browserProxy.getListTitlePluralString(numApplications),
        ])
        .then(strings => {
          this.subtitleText_ = strings[0];
          this.subtitleNoAdminRightsText_ = strings[1];
          this.listTitleText_ = strings[2];
        });
  }
}

customElements.define(
    SettingsIncompatibleApplicationsPageElement.is,
    SettingsIncompatibleApplicationsPageElement);
