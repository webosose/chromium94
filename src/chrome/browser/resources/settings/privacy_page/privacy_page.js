// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-privacy-page' is the settings page containing privacy and
 * security settings.
 */
import 'chrome://resources/cr_elements/cr_button/cr_button.m.js';
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.m.js';
import 'chrome://resources/cr_elements/cr_link_row/cr_link_row.js';
import 'chrome://resources/cr_elements/shared_style_css.m.js';
import 'chrome://resources/polymer/v3_0/iron-flex-layout/iron-flex-layout-classes.js';
import '../controls/settings_toggle_button.js';
import '../prefs/prefs.js';
import '../site_settings/settings_category_default_radio_group.js';
import '../settings_page/settings_animated_pages.js';
import '../settings_page/settings_subpage.js';
import '../settings_shared_css.js';

import {assert} from 'chrome://resources/js/assert.m.js';
import {focusWithoutInk} from 'chrome://resources/js/cr/ui/focus_without_ink.m.js';
import {I18nBehavior, I18nBehaviorInterface} from 'chrome://resources/js/i18n_behavior.m.js';
import {WebUIListenerBehavior, WebUIListenerBehaviorInterface} from 'chrome://resources/js/web_ui_listener_behavior.m.js';
import {html, mixinBehaviors, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {SettingsToggleButtonElement} from '../controls/settings_toggle_button.js';
import {HatsBrowserProxyImpl, TrustSafetyInteraction} from '../hats_browser_proxy.js';
import {loadTimeData} from '../i18n_setup.js';
import {MetricsBrowserProxy, MetricsBrowserProxyImpl, PrivacyElementInteractions} from '../metrics_browser_proxy.js';
import {PrefsBehavior, PrefsBehaviorInterface} from '../prefs/prefs_behavior.js';
import {routes} from '../route.js';
import {RouteObserverMixin, RouteObserverMixinInterface, Router} from '../router.js';
import {ChooserType, ContentSettingsTypes, CookieControlsMode, NotificationSetting} from '../site_settings/constants.js';
import {SiteSettingsPrefsBrowserProxyImpl} from '../site_settings/site_settings_prefs_browser_proxy.js';

import {PrivacyPageBrowserProxy, PrivacyPageBrowserProxyImpl} from './privacy_page_browser_proxy.js';

/**
 * @typedef {{
 *   enabled: boolean,
 *   pref: !chrome.settingsPrivate.PrefObject
 * }}
 */
let BlockAutoplayStatus;

/**
 * @constructor
 * @extends {PolymerElement}
 * @implements {I18nBehaviorInterface}
 * @implements {PrefsBehaviorInterface}
 * @implements {RouteObserverMixinInterface}
 * @implements {WebUIListenerBehaviorInterface}
 */
const SettingsPrivacyPageElementBase = mixinBehaviors(
    [PrefsBehavior, I18nBehavior, WebUIListenerBehavior],
    RouteObserverMixin(PolymerElement));

/** @polymer */
export class SettingsPrivacyPageElement extends SettingsPrivacyPageElementBase {
  static get is() {
    return 'settings-privacy-page';
  }

  static get template() {
    return html`{__html_template__}`;
  }

  static get properties() {
    return {
      /**
       * Preferences state.
       */
      prefs: {
        type: Object,
        notify: true,
      },

      /** @private */
      isGuest_: {
        type: Boolean,
        value() {
          return loadTimeData.getBoolean('isGuest');
        }
      },

      /** @private */
      showClearBrowsingDataDialog_: Boolean,

      /** @private */
      enableSafeBrowsingSubresourceFilter_: {
        type: Boolean,
        value() {
          return loadTimeData.getBoolean('enableSafeBrowsingSubresourceFilter');
        }
      },

      /** @private */
      cookieSettingDescription_: String,

      /** @private */
      enableBlockAutoplayContentSetting_: {
        type: Boolean,
        value() {
          return loadTimeData.getBoolean('enableBlockAutoplayContentSetting');
        }
      },

      /** @private {BlockAutoplayStatus} */
      blockAutoplayStatus_: {
        type: Object,
        value() {
          return /** @type {BlockAutoplayStatus} */ ({});
        }
      },

      /** @private */
      enableContentSettingsRedesign_: {
        type: Boolean,
        value() {
          return loadTimeData.getBoolean('enableContentSettingsRedesign');
        }
      },

      /** @private */
      enablePaymentHandlerContentSetting_: {
        type: Boolean,
        value() {
          return loadTimeData.getBoolean('enablePaymentHandlerContentSetting');
        }
      },

      /** @private */
      enableExperimentalWebPlatformFeatures_: {
        type: Boolean,
        value() {
          return loadTimeData.getBoolean(
              'enableExperimentalWebPlatformFeatures');
        },
      },

      /** @private */
      enableSecurityKeysSubpage_: {
        type: Boolean,
        readOnly: true,
        value() {
          return loadTimeData.getBoolean('enableSecurityKeysSubpage');
        }
      },

      /** @private */
      enableQuietNotificationPromptsSetting_: {
        type: Boolean,
        value: () =>
            loadTimeData.getBoolean('enableQuietNotificationPromptsSetting'),
      },

      /** @private */
      enableWebBluetoothNewPermissionsBackend_: {
        type: Boolean,
        value: () =>
            loadTimeData.getBoolean('enableWebBluetoothNewPermissionsBackend'),
      },

      /** @private */
      enablePrivacySandboxSettings_: {
        type: Boolean,
        value: () => loadTimeData.getBoolean('privacySandboxSettingsEnabled'),
      },

      /** @private */
      enablePrivacyReview_: {
        type: Boolean,
        value: () => loadTimeData.getBoolean('privacyReviewEnabled'),
      },

      /** @private {!Map<string, string>} */
      focusConfig_: {
        type: Object,
        value() {
          const map = new Map();

          if (routes.SECURITY) {
            map.set(routes.SECURITY.path, '#securityLinkRow');
          }

          if (routes.COOKIES) {
            map.set(
                `${routes.COOKIES.path}_${routes.PRIVACY.path}`,
                '#cookiesLinkRow');
            map.set(
                `${routes.COOKIES.path}_${routes.BASIC.path}`,
                '#cookiesLinkRow');
          }

          if (routes.SITE_SETTINGS) {
            map.set(routes.SITE_SETTINGS.path, '#permissionsLinkRow');
          }

          if (routes.PRIVACY_REVIEW) {
            map.set(routes.PRIVACY_REVIEW.path, '#privacyReviewLinkRow');
          }

          return map;
        },
      },

      /**
       * Expose NotificationSetting enum to HTML bindings.
       * @private
       */
      notificationSettingEnum_: {
        type: Object,
        value: NotificationSetting,
      },

      /** @private */
      searchFilter_: String,

      /** @private */
      siteDataFilter_: String,

      /**
       * Expose ContentSettingsTypes enum to HTML bindings.
       * @private
       */
      contentSettingsTypesEnum_: {
        type: Object,
        value: ContentSettingsTypes,
      },

      /**
       * Expose ChooserType enum to HTML bindings.
       * @private
       */
      chooserTypeEnum_: {
        type: Object,
        value: ChooserType,
      },
    };
  }

  constructor() {
    super();

    /** @private {!PrivacyPageBrowserProxy} */
    this.browserProxy_ = PrivacyPageBrowserProxyImpl.getInstance();

    /** @private {!MetricsBrowserProxy} */
    this.metricsBrowserProxy_ = MetricsBrowserProxyImpl.getInstance();
  }

  /** @override */
  ready() {
    super.ready();

    this.onBlockAutoplayStatusChanged_({
      pref: /** @type {chrome.settingsPrivate.PrefObject} */ ({value: false}),
      enabled: false
    });

    this.addWebUIListener(
        'onBlockAutoplayStatusChanged',
        this.onBlockAutoplayStatusChanged_.bind(this));

    SiteSettingsPrefsBrowserProxyImpl.getInstance()
        .getCookieSettingDescription()
        .then(description => this.cookieSettingDescription_ = description);
    this.addWebUIListener(
        'cookieSettingDescriptionChanged',
        description => this.cookieSettingDescription_ = description);
  }

  /** @protected */
  currentRouteChanged() {
    this.showClearBrowsingDataDialog_ =
        Router.getInstance().getCurrentRoute() === routes.CLEAR_BROWSER_DATA;
  }

  /**
   * Called when the block autoplay status changes.
   * @param {BlockAutoplayStatus} autoplayStatus
   * @private
   */
  onBlockAutoplayStatusChanged_(autoplayStatus) {
    this.blockAutoplayStatus_ = autoplayStatus;
  }

  /**
   * Updates the block autoplay pref when the toggle is changed.
   * @param {!Event} event
   * @private
   */
  onBlockAutoplayToggleChange_(event) {
    const target = /** @type {!SettingsToggleButtonElement} */ (event.target);
    this.browserProxy_.setBlockAutoplayEnabled(target.checked);
  }

  /**
   * This is a workaround to connect the remove all button to the subpage.
   * @private
   */
  onRemoveAllCookiesFromSite_() {
    // Intentionally not casting to SiteDataDetailsSubpageElement, as this would
    // require importing site_data_details_subpage.js and would endup in the
    // main JS bundle.
    const node = this.shadowRoot.querySelector('site-data-details-subpage');
    if (node) {
      node.removeAll();
    }
  }

  /** @private */
  onClearBrowsingDataTap_() {
    this.interactedWithPage_();

    Router.getInstance().navigateTo(routes.CLEAR_BROWSER_DATA);
  }

  /** @private */
  onCookiesClick_() {
    this.interactedWithPage_();

    Router.getInstance().navigateTo(routes.COOKIES);
  }

  /** @private */
  onDialogClosed_() {
    Router.getInstance().navigateTo(assert(routes.CLEAR_BROWSER_DATA.parent));
    setTimeout(() => {
      // Focus after a timeout to ensure any a11y messages get read before
      // screen readers read out the newly focused element.
      focusWithoutInk(
          assert(this.shadowRoot.querySelector('#clearBrowsingData')));
    });
  }

  /** @private */
  onPermissionsPageClick_() {
    this.interactedWithPage_();

    Router.getInstance().navigateTo(routes.SITE_SETTINGS);
  }

  /** @private */
  onSecurityPageClick_() {
    this.interactedWithPage_();
    this.metricsBrowserProxy_.recordAction(
        'SafeBrowsing.Settings.ShowedFromParentSettings');
    Router.getInstance().navigateTo(routes.SECURITY);
  }

  /** @private */
  onPrivacySandboxClick_() {
    this.metricsBrowserProxy_.recordAction(
        'Settings.PrivacySandbox.OpenedFromSettingsParent');
    // TODO(crbug/1159942): Replace this with an ordinary OpenWindowProxy call.
    this.shadowRoot.getElementById('privacySandboxLink').click();
  }

  /** @private */
  onPrivacyReviewClick_() {
    // TODO(crbug/1215630): Implement metrics.
    Router.getInstance().navigateTo(routes.PRIVACY_REVIEW);
  }

  /** @private */
  getProtectedContentLabel_(value) {
    return value ? this.i18n('siteSettingsProtectedContentAllowed') :
                   this.i18n('siteSettingsProtectedContentBlocked');
  }

  /** @private */
  interactedWithPage_() {
    HatsBrowserProxyImpl.getInstance().trustSafetyInteractionOccurred(
        TrustSafetyInteraction.USED_PRIVACY_CARD);
  }

  /**
   * @return {string}
   * @private
   */
  computePrivacySandboxSublabel_() {
    return this.getPref('privacy_sandbox.apis_enabled').value ?
        this.i18n('privacySandboxTrialsEnabled') :
        this.i18n('privacySandboxTrialsDisabled');
  }

  /**
   * @return {string}
   * @private
   */
  computeClearBrowsingDataClass_() {
    return this.enablePrivacyReview_ ? 'hr' : '';
  }
}

customElements.define(
    SettingsPrivacyPageElement.is, SettingsPrivacyPageElement);
