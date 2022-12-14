// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'switch-access-subpage' is the collapsible section containing
 * Switch Access settings.
 */

import {getLabelForAssignment} from './switch_access_action_assignment_pane.m.js';

/**
 * The portion of the setting name common to all Switch Access preferences.
 * @const
 */
const PREFIX = 'settings.a11y.switch_access.';

/** @type {!Array<number>} */
const POINT_SCAN_SPEED_RANGE_DIPS_PER_SECOND = [25, 50, 75, 100, 150, 200, 300];

/**
 * @param {!Array<number>} ticksInMs
 * @return {!Array<!cr_slider.SliderTick>}
 */
function ticksWithLabelsInSec(ticksInMs) {
  // Dividing by 1000 to convert milliseconds to seconds for the label.
  return ticksInMs.map(x => ({label: `${x / 1000}`, value: x}));
}

/**
 * @param {!Array<number>} ticks
 * @return {!Array<!cr_slider.SliderTick>}
 */
function ticksWithCountingLabels(ticks) {
  return ticks.map((x, i) => ({label: i + 1, value: x}));
}

Polymer({
  is: 'settings-switch-access-subpage',

  behaviors: [
    DeepLinkingBehavior,
    I18nBehavior,
    PrefsBehavior,
    settings.RouteObserverBehavior,
    WebUIListenerBehavior,
  ],

  properties: {
    /**
     * Preferences state.
     */
    prefs: {
      type: Object,
      notify: true,
    },

    /** @private {!Array<{key: string, device: !SwitchAccessDeviceType}>} */
    selectAssignments_: {
      type: Array,
      value: [],
      notify: true,
    },

    /** @private {!Array<{key: string, device: !SwitchAccessDeviceType}>} */
    nextAssignments_: {
      type: Array,
      value: [],
      notify: true,
    },

    /** @private {!Array<{key: string, device: !SwitchAccessDeviceType}>} */
    previousAssignments_: {
      type: Array,
      value: [],
      notify: true,
    },

    /** @private {Array<number>} */
    autoScanSpeedRangeMs_: {
      readOnly: true,
      type: Array,
      value: ticksWithLabelsInSec(AUTO_SCAN_SPEED_RANGE_MS),
    },

    /** @private {Array<number>} */
    pointScanSpeedRangeDipsPerSecond_: {
      readOnly: true,
      type: Array,
      value: ticksWithCountingLabels(POINT_SCAN_SPEED_RANGE_DIPS_PER_SECOND),
    },

    /** @private {Object} */
    formatter_: {
      type: Object,
      value() {
        // navigator.language actually returns a locale, not just a language.
        const locale = window.navigator.language;
        const options = {minimumFractionDigits: 1, maximumFractionDigits: 1};
        return new Intl.NumberFormat(locale, options);
      },
    },

    /** @private {number} */
    maxScanSpeedMs_: {
      readOnly: true,
      type: Number,
      value: AUTO_SCAN_SPEED_RANGE_MS[AUTO_SCAN_SPEED_RANGE_MS.length - 1]
    },

    /** @private {string} */
    maxScanSpeedLabelSec_: {
      readOnly: true,
      type: String,
      value() {
        return this.scanSpeedStringInSec_(this.maxScanSpeedMs_);
      },
    },

    /** @private {number} */
    minScanSpeedMs_:
        {readOnly: true, type: Number, value: AUTO_SCAN_SPEED_RANGE_MS[0]},

    /** @private {string} */
    minScanSpeedLabelSec_: {
      readOnly: true,
      type: String,
      value() {
        return this.scanSpeedStringInSec_(this.minScanSpeedMs_);
      },
    },

    /** @private {number} */
    maxPointScanSpeed_: {
      readOnly: true,
      type: Number,
      value: POINT_SCAN_SPEED_RANGE_DIPS_PER_SECOND.length
    },

    /** @private {number} */
    minPointScanSpeed_: {readOnly: true, type: Number, value: 1},

    /**
     * Used by DeepLinkingBehavior to focus this page's deep links.
     * @type {!Set<!chromeos.settings.mojom.Setting>}
     */
    supportedSettingIds: {
      type: Object,
      value: () => new Set([
        chromeos.settings.mojom.Setting.kSwitchActionAssignment,
        chromeos.settings.mojom.Setting.kSwitchActionAutoScan,
        chromeos.settings.mojom.Setting.kSwitchActionAutoScanKeyboard,
      ]),
    },

    /** @private */
    showSwitchAccessActionAssignmentDialog_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    showSwitchAccessSetupGuideDialog_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    showSwitchAccessSetupGuideWarningDialog_: {
      type: Boolean,
      value: false,
    },

    /** @private {?SwitchAccessCommand} */
    action_: {
      type: String,
      value: null,
      notify: true,
    },
  },

  /** @private {?SwitchAccessSubpageBrowserProxy} */
  switchAccessBrowserProxy_: null,

  /** @override */
  created() {
    this.switchAccessBrowserProxy_ =
        SwitchAccessSubpageBrowserProxyImpl.getInstance();
  },

  /** @override */
  ready() {
    this.addWebUIListener(
        'switch-access-assignments-changed',
        this.onAssignmentsChanged_.bind(this));
    this.switchAccessBrowserProxy_.refreshAssignmentsFromPrefs();
  },

  /**
   * @param {!settings.Route} route
   * @param {!settings.Route} oldRoute
   */
  currentRouteChanged(route, oldRoute) {
    // Does not apply to this page.
    if (route !== settings.routes.MANAGE_SWITCH_ACCESS_SETTINGS) {
      return;
    }

    this.attemptDeepLink();
  },

  /** @private */
  onSetupGuideRerunClick_() {
    if (this.showSetupGuide_()) {
      this.showSwitchAccessSetupGuideWarningDialog_ = true;
    }
  },

  /** @private */
  onSetupGuideWarningDialogCancel_() {
    this.showSwitchAccessSetupGuideWarningDialog_ = false;
  },

  /** @private */
  onSetupGuideWarningDialogClose_() {
    // The on_cancel is followed by on_close, so check cancel didn't happen
    // first.
    if (this.showSwitchAccessSetupGuideWarningDialog_) {
      this.openSetupGuide_();
      this.showSwitchAccessSetupGuideWarningDialog_ = false;
    }
  },

  /** @private */
  openSetupGuide_() {
    this.showSwitchAccessSetupGuideWarningDialog_ = false;
    if (this.showSetupGuide_()) {
      this.showSwitchAccessSetupGuideDialog_ = true;
    }
  },

  /** @private */
  onSelectAssignClick_() {
    this.action_ = SwitchAccessCommand.SELECT;
    this.showSwitchAccessActionAssignmentDialog_ = true;
    this.focusAfterDialogClose_ = this.$.selectLinkRow;
  },

  /** @private */
  onNextAssignClick_() {
    this.action_ = SwitchAccessCommand.NEXT;
    this.showSwitchAccessActionAssignmentDialog_ = true;
    this.focusAfterDialogClose_ = this.$.nextLinkRow;
  },

  /** @private */
  onPreviousAssignClick_() {
    this.action_ = SwitchAccessCommand.PREVIOUS;
    this.showSwitchAccessActionAssignmentDialog_ = true;
    this.focusAfterDialogClose_ = this.$.previousLinkRow;
  },

  /** @private */
  onSwitchAccessSetupGuideDialogClose_() {
    this.showSwitchAccessSetupGuideDialog_ = false;
    this.$.setupGuideLink.focus();
  },

  /** @private */
  onSwitchAccessActionAssignmentDialogClose_() {
    this.showSwitchAccessActionAssignmentDialog_ = false;
    this.focusAfterDialogClose_.focus();
  },

  /**
   * @param {!Object<SwitchAccessCommand, !Array<{key: string, device:
   *     !SwitchAccessDeviceType}>>} value
   * @private
   */
  onAssignmentsChanged_(value) {
    this.selectAssignments_ = value[SwitchAccessCommand.SELECT];
    this.nextAssignments_ = value[SwitchAccessCommand.NEXT];
    this.previousAssignments_ = value[SwitchAccessCommand.PREVIOUS];

    // Any complete assignment will have at least one switch assigned to SELECT.
    // If this method is called with no SELECT switches, then the page has just
    // loaded, and we should open the setup guide.
    if (Object.keys(this.selectAssignments_).length === 0 &&
        this.showSetupGuide_) {
      this.openSetupGuide_();
    }
  },

  /**
   * @param {{key: string, device: !SwitchAccessDeviceType}} assignment
   * @return {string}
   * @private
   */
  getLabelForAssignment_(assignment) {
    return getLabelForAssignment(assignment);
  },

  /**
   * @param {!Array<{key: string, device: !SwitchAccessDeviceType}>} assignments
   *     List of assignments
   * @return {string} (e.g. 'Alt (USB), Backspace, Enter, and 4 more switches')
   * @private
   */
  getAssignSwitchSubLabel_(assignments) {
    const switches =
        assignments.map(assignment => this.getLabelForAssignment_(assignment));
    switch (switches.length) {
      case 0:
        return this.i18n('assignSwitchSubLabel0Switches');
      case 1:
        return this.i18n('assignSwitchSubLabel1Switch', switches[0]);
      case 2:
        return this.i18n('assignSwitchSubLabel2Switches', ...switches);
      case 3:
        return this.i18n('assignSwitchSubLabel3Switches', ...switches);
      case 4:
        return this.i18n(
            'assignSwitchSubLabel4Switches', ...switches.slice(0, 3));
      default:
        return this.i18n(
            'assignSwitchSubLabel5OrMoreSwitches', ...switches.slice(0, 3),
            switches.length - 3);
    }
  },

  /**
   * @return {boolean} Whether to show settings for auto-scan within the
   *     keyboard.
   * @private
   */
  showKeyboardScanSettings_() {
    const improvedTextInputEnabled = loadTimeData.getBoolean(
        'showExperimentalAccessibilitySwitchAccessImprovedTextInput');
    const autoScanEnabled = /** @type {boolean} */
        (this.getPref(PREFIX + 'auto_scan.enabled').value);
    return improvedTextInputEnabled && autoScanEnabled;
  },

  /**
   * @return {boolean} Whether to show the Switch Access setup guide.
   * @private
   */
  showSetupGuide_() {
    return loadTimeData.getBoolean('showSwitchAccessSetupGuide');
  },

  /**
   * @return {boolean} Whether Switch Access point scanning is enabled.
   * @private
   */
  isSwitchAccessPointScanningEnabled_() {
    return loadTimeData.getBoolean('isSwitchAccessPointScanningEnabled');
  },

  /**
   * @param {number} scanSpeedValueMs
   * @return {string} a string representing the scan speed in seconds.
   * @private
   */
  scanSpeedStringInSec_(scanSpeedValueMs) {
    const scanSpeedValueSec = scanSpeedValueMs / 1000;
    return this.i18n(
        'durationInSeconds', this.formatter_.format(scanSpeedValueSec));
  },
});
