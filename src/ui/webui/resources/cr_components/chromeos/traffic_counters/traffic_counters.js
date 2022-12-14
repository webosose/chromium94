// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'chrome://resources/cr_components/chromeos/network/network_shared_css.m.js';

import {OncMojo} from 'chrome://resources/cr_components/chromeos/network/onc_mojo.m.js';
import {I18nBehavior, I18nBehaviorInterface} from 'chrome://resources/js/i18n_behavior.m.js';
import {CrosNetworkConfig} from 'chrome://resources/mojo/chromeos/services/network_config/public/mojom/cros_network_config.mojom-webui.js';
import {NetworkType} from 'chrome://resources/mojo/chromeos/services/network_config/public/mojom/network_types.mojom-webui.js';
import {html, mixinBehaviors, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

/**
 * @fileoverview Polymer element for a container used in displaying network
 * traffic information.
 */

/**
 * Image file name corresponding to network type.
 * @enum {string}
 */
const TechnologyIcons = {
  CELLULAR: 'cellular_0.svg',
  ETHERNET: 'ethernet.svg',
  VPN: 'vpn.svg',
  WIFI: 'wifi_0.svg',
};

/**
 * Information about a network.
 * @typedef {{
 *   guid: string,
 *   name: string,
 *   type: !chromeos.networkConfig.mojom.NetworkType,
 *   counters: !Array<!Object>,
 *   lastResetTime: ?mojoBase.mojom.Time,
 * }}
 */
let Network;

/**
 * Helper function to create a Network object.
 * @param {string} guid
 * @param {string} name
 * @param {!chromeos.networkConfig.mojom.NetworkType} type
 * @param {!Array<!Object>} counters
 * @param {?mojoBase.mojom.Time} lastResetTime
 * @return {Network} Network object
 */
function createNetwork(guid, name, type, counters, lastResetTime) {
  return {
    guid: guid,
    name: name,
    type: type,
    counters: counters,
    lastResetTime: lastResetTime,
  };
}

/**
 * Replacer function used to handle the bigint type.
 * @param {string} key
 * @param {*|null|undefined} value
 * @return {*|undefined} value in string format
 */
function replacer(key, value) {
  return typeof value === 'bigint' ? value.toString() : value;
}

/**
 * @constructor
 * @extends {PolymerElement}
 * @implements {I18nBehaviorInterface}
 */
const TrafficCountersElementBase =
    mixinBehaviors([I18nBehavior], PolymerElement);

/** @polymer */
export class TrafficCountersElement extends TrafficCountersElementBase {
  static get is() {
    return 'traffic-counters';
  }

  static get template() {
    return html`{__html_template__}`;
  }

  static get properties() {
    return {
      /**
       * Information about networks.
       * @private {!Array<!Network>}
       */
      networks_: {type: Array, value: []},
    };
  }

  constructor() {
    super();

    /**
     * Network Config mojo remote.
     * @private {?chromeos.networkConfig.mojom.CrosNetworkConfigRemote}
     */
    this.networkConfig_ =
        chromeos.networkConfig.mojom.CrosNetworkConfig.getRemote();

    /**
     * Expanded state per network type.
     * @private {!Array<boolean>}
     */
    this.typeExpanded_ = [];
  }

  /**
   * Handles requests to request traffic counters.
   * @private
   */
  onRequestTrafficCountersClick_() {
    this.fetchTrafficCountersForActiveNetworks_();
  }

  /**
   * Handles requests to reset traffic counters.
   * @param {!Event} event
   * @private
   */
  async onResetTrafficCountersClick_(event) {
    const network = event.model.network;
    this.networkConfig_.resetTrafficCounters(network.guid);
    const trafficCounters =
        await this.getTrafficCountersForNetwork_(network.guid);
    const lastResetTime = await this.getLastResetTime(network.guid);
    const foundIdx = this.networks_.findIndex(n => n.guid === network.guid);
    if (foundIdx === -1) {
      return;
    }
    this.splice(
        'networks_', foundIdx, 1,
        createNetwork(
            network.guid, network.name, network.type, trafficCounters,
            lastResetTime));
  }

  /**
   * Requests traffic counters for networks.
   * @private
   */
  async fetchTrafficCountersForActiveNetworks_() {
    this.networks_ = [];
    const filter = {
      filter: chromeos.networkConfig.mojom.FilterType.kActive,
      networkType: chromeos.networkConfig.mojom.NetworkType.kAll,
      limit: chromeos.networkConfig.mojom.NO_LIMIT,
    };
    const networkStateList =
        await this.networkConfig_.getNetworkStateList(filter);
    for (const networkState of networkStateList.result) {
      const trafficCounters =
          await this.getTrafficCountersForNetwork_(networkState.guid);
      const lastResetTime = await this.getLastResetTime(networkState.guid);
      this.push(
          'networks_',
          createNetwork(
              networkState.guid, networkState.name, networkState.type,
              trafficCounters, lastResetTime));
    }
  }

  /**
   * Requests and sets traffic counters for the given network.
   * @param {string} guid
   * @return {!Promise<!Array<!Object>>} traffic counters for network with guid
   * @private
   */
  async getTrafficCountersForNetwork_(guid) {
    const trafficCountersObj =
        await this.networkConfig_.requestTrafficCounters(guid);
    this.convertSourceEnumToString_(trafficCountersObj.trafficCounters);
    return trafficCountersObj.trafficCounters;
  }

  /**
   * Gets last reset time.
   * @param {string} guid
   * @return {?Promise<?mojoBase.mojom.Time>} last reset
   *     time for network with guid
   * @private
   */
  async getLastResetTime(guid) {
    const managedPropertiesPromise =
        await this.networkConfig_.getManagedProperties(guid);
    if (!managedPropertiesPromise) {
      return null;
    }

    return managedPropertiesPromise.result.trafficCounterResetTime || null;
  }

  /**
   * @param {!chromeos.networkConfig.mojom.NetworkType} type
   * @return {string} A string for the given NetworkType.
   * @private
   */
  getNetworkTypeString_(type) {
    return this.i18n('OncType' + OncMojo.getNetworkTypeString(type));
  }

  /**
   * @param {!chromeos.networkConfig.mojom.NetworkType} type
   * @return {string} An icon for the given NetworkType.
   * @private
   */
  getNetworkTypeIcon_(type) {
    switch (type) {
      case chromeos.networkConfig.mojom.NetworkType.kEthernet:
        return TechnologyIcons.ETHERNET;
      case chromeos.networkConfig.mojom.NetworkType.kWiFi:
        return TechnologyIcons.WIFI;
      case chromeos.networkConfig.mojom.NetworkType.kVPN:
        return TechnologyIcons.VPN;
      case chromeos.networkConfig.mojom.NetworkType.kTether:
      case chromeos.networkConfig.mojom.NetworkType.kMobile:
      case chromeos.networkConfig.mojom.NetworkType.kCellular:
        return TechnologyIcons.CELLULAR;
      default:
        return '';
    }
  }

  /**
   * @param {!chromeos.networkConfig.mojom.NetworkType} type
   * @return {boolean} Whether type should be expanded.
   * @private
   * */
  getTypeExpanded_(type) {
    if (this.typeExpanded_[type] === undefined) {
      this.set('typeExpanded_.' + type, false);
      return false;
    }
    return this.typeExpanded_[type];
  }

  /**
   * Helper function to toggle the expanded properties when the network
   * container is toggled.
   * @param {!Event} event
   * @private
   */
  onToggleExpanded_(event) {
    const type = event.model.network.type;
    this.set('typeExpanded_.' + type, !this.typeExpanded_[type]);
  }

  /**
   * Convert the traffic counters' source enum to a readable string.
   * @param {!Array<!Object>} counters
   * @private
   */
  convertSourceEnumToString_(counters) {
    for (const counter of counters) {
      switch (counter.source) {
        case chromeos.networkConfig.mojom.TrafficCounterSource.kUnknown:
          counter.source = this.i18n('TrafficCountersUnknown');
          break;
        case chromeos.networkConfig.mojom.TrafficCounterSource.kChrome:
          counter.source = this.i18n('TrafficCountersChrome');
          break;
        case chromeos.networkConfig.mojom.TrafficCounterSource.kUser:
          counter.source = this.i18n('TrafficCountersUser');
          break;
        case chromeos.networkConfig.mojom.TrafficCounterSource.kArc:
          counter.source = this.i18n('TrafficCountersArc');
          break;
        case chromeos.networkConfig.mojom.TrafficCounterSource.kCrosvm:
          counter.source = this.i18n('TrafficCountersCrosvm');
          break;
        case chromeos.networkConfig.mojom.TrafficCounterSource.kPluginvm:
          counter.source = this.i18n('TrafficCountersPluginvm');
          break;
        case chromeos.networkConfig.mojom.TrafficCounterSource.kUpdateEngine:
          counter.source = this.i18n('TrafficCountersUpdateEngine');
          break;
        case chromeos.networkConfig.mojom.TrafficCounterSource.kVpn:
          counter.source = this.i18n('TrafficCountersVpn');
          break;
        case chromeos.networkConfig.mojom.TrafficCounterSource.kSystem:
          counter.source = this.i18n('TrafficCountersSystem');
      }
    }
  }

  /**
   * @param {!Array<!Object>} counters
   * @return {string} A string representation of the traffic counters for a
   *     particular network.
   * @private
   */
  countersToString_(counters) {
    // '\t' describes the number of white space characters to use as white space
    // while forming the JSON string.
    return JSON.stringify(counters, replacer, '\t');
  }

  /**
   * @param {Network} network
   * @return {string} a representation of the last reset time for a particular
   * network.
   * @private
   */
  lastResetTimeString_(network) {
    return JSON.stringify(network.lastResetTime.internalValue, replacer, '\t');
  }
}
customElements.define(TrafficCountersElement.is, TrafficCountersElement);
