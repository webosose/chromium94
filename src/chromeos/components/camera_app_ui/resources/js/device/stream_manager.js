// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {assert} from '../chrome_util.js';
import {reportError} from '../error.js';
import {I18nString} from '../i18n_string.js';
import * as loadTimeData from '../models/load_time_data.js';
import {DeviceOperator} from '../mojo/device_operator.js';
import * as toast from '../toast.js';
// eslint-disable-next-line no-unused-vars
import {ErrorLevel, ErrorType, Facing, VideoConfig} from '../type.js';
import {WaitableEvent} from '../waitable_event.js';

import {Camera3DeviceInfo} from './camera3_device_info.js';
import {
  StreamConstraints,  // eslint-disable-line no-unused-vars
  toMediaStreamConstraints,
} from './stream_constraints.js';

/**
 * The singleton instance of StreamManager. Initialized by the first
 * invocation of getInstance().
 * @type {?StreamManager}
 */
let instance = null;

/**
 * Device information includs MediaDeviceInfo and Camera3DeviceInfo.
 * @typedef {{
 *   v1Info: !MediaDeviceInfo,
 *   v3Info: ?Camera3DeviceInfo
 * }}
 */
export let DeviceInfo;

/**
 * Real and virtual device mapping.
 * @typedef {{
 *   realId: string,
 *   virtualId: string
 * }}
 */
// eslint-disable-next-line no-unused-vars
let VirtualMap;

/**
 * Creates extra stream for the current mode.
 */
export class CaptureStream {
  /**
   * @param {string} deviceId Device id of currently working video device
   * @param {!MediaStream} stream Capture stream
   * @public
   */
  constructor(deviceId, stream) {
    /**
     * Device id of currently working video device.
     * @type {string}
     * @private
     */
    this.deviceId_ = deviceId;

    /**
     * Capture stream
     * @type {!MediaStream}
     * @protected
     */
    this.stream_ = stream;
  }

  /**
   * @return {!MediaStream}
   * @public
   */
  get stream() {
    return this.stream_;
  }

  /**
   * Closes stream.
   * @public
   */
  async close() {
    this.stream_.getVideoTracks()[0].stop();
    if (await DeviceOperator.isSupported()) {
      try {
        await StreamManager.getInstance().setMultipleStreamsEnabled(
            this.deviceId_, false);
      } catch (e) {
        reportError(ErrorType.MULTIPLE_STREAMS_FAILURE, ErrorLevel.ERROR, e);
      }
    }
  }
}

/**
 * Monitors device change and provides different listener callbacks for
 * device changes. It also provides streams for different modes.
 */
export class StreamManager {
  /**
   * @private
   */
  constructor() {
    /**
     * MediaDeviceInfo of all available video devices.
     * @type {?Promise<!Array<!MediaDeviceInfo>>}
     * @private
     */
    this.devicesInfo_ = null;

    /**
     * Camera3DeviceInfo of all available video devices. Is null on HALv1 device
     * without mojo api support.
     * @type {?Promise<?Array<!DeviceInfo>>}
     * @private
     */
    this.camera3DevicesInfo_ = null;

    /**
     * Listeners for real device change event.
     * @type {!Array<function(!Array<!DeviceInfo>): !Promise>}
     * @private
     */
    this.realListeners_ = [];

    /**
     * Latest result of Camera3DeviceInfo of all real video devices.
     * @type {!Array<!DeviceInfo>}
     * @private
     */
    this.realDevices_ = [];

    /**
     * real device id and corresponding virtual devices id mapping and it is
     * only available on HALv3.
     * @type {?VirtualMap}
     * @private
     */
    this.virtualMap_ = null;

    /**
     * Signal it to indicate that the virtual device is ready.
     * @type {?WaitableEvent<string>}
     * @private
     */
    this.waitVirtual_ = null;

    /**
     * Filter out lagging 720p on grunt. See https://crbug.com/1122852.
     * @const {!Promise<function(!VideoConfig): boolean>}
     * @private
     */
    this.videoConfigFilter_ = (async () => {
      const board = await loadTimeData.getBoard();
      return board === 'grunt' ? ({height}) => height < 720 : () => true;
    })();

    navigator.mediaDevices.addEventListener(
        'devicechange', this.deviceUpdate.bind(this));
  }

  /**
   * Creates a new instance of StreamManager if it is not set. Returns the
   *     exist instance.
   * @return {!StreamManager} The singleton instance.
   */
  static getInstance() {
    if (instance === null) {
      instance = new StreamManager();
    }
    return instance;
  }

  /**
   * Registers listener to be called when state of available real devices
   * changes.
   * @param {function(!Array<!DeviceInfo>): !Promise} listener
   */
  addRealDeviceChangeListener(listener) {
    this.realListeners_.push(listener);
  }

  /**
   * Creates extra stream according to the constraints.
   * @param {!StreamConstraints} constraints
   * @return {!Promise<!CaptureStream>}
   */
  async openCaptureStream(constraints) {
    const realDeviceId = constraints.deviceId;
    if (await DeviceOperator.isSupported()) {
      try {
        await this.setMultipleStreamsEnabled(realDeviceId, true);
        constraints.deviceId = this.virtualMap_.virtualId;
      } catch (e) {
        reportError(ErrorType.MULTIPLE_STREAMS_FAILURE, ErrorLevel.ERROR, e);
      }
    }

    const stream = await navigator.mediaDevices.getUserMedia(
        toMediaStreamConstraints(constraints));
    return new CaptureStream(realDeviceId, stream);
  }

  /**
   * Handling function for device changing.
   */
  async deviceUpdate() {
    const devices = await this.doDeviceInfoUpdate_();
    if (devices === null) {
      return;
    }
    await this.doDeviceNotify_(devices);
  }

  /**
   * Gets devices information via mojo IPC.
   * @return {?Promise<?Array<!DeviceInfo>>}
   * @private
   */
  async doDeviceInfoUpdate_() {
    this.devicesInfo_ = this.enumerateDevices_();
    this.camera3DevicesInfo_ = this.queryMojoDevicesInfo_();
    try {
      return await this.camera3DevicesInfo_;
    } catch (e) {
      reportError(ErrorType.DEVICE_INFO_UPDATE_FAILURE, ErrorLevel.ERROR, e);
    }
    return null;
  }

  /**
   * Notifies device changes to listeners and create a mapping for real and
   * virtual device.
   * @param {!Array<!DeviceInfo>} devices
   * @private
   */
  async doDeviceNotify_(devices) {
    const isVirtual = (d) => d.v3Info !== null &&
        (d.v3Info.facing === Facing.VIRTUAL_USER ||
         d.v3Info.facing === Facing.VIRTUAL_ENV ||
         d.v3Info.facing === Facing.VIRTUAL_EXT);
    const realDevices = devices.filter((d) => !isVirtual(d));
    const virtualDevices = devices.filter(isVirtual);
    // We currently only support one virtual device.
    assert(virtualDevices.length <= 1);

    if (virtualDevices.length === 1 && this.waitVirtual_ !== null) {
      this.waitVirtual_.signal(virtualDevices[0].v1Info.deviceId);
      this.waitVirtual_ = null;
    }

    let isRealDeviceChange = false;
    for (const added of this.getDifference_(realDevices, this.realDevices_)) {
      toast.speak(I18nString.STATUS_MSG_CAMERA_PLUGGED, added.v1Info.label);
      isRealDeviceChange = true;
    }
    for (const removed of this.getDifference_(this.realDevices_, realDevices)) {
      toast.speak(I18nString.STATUS_MSG_CAMERA_UNPLUGGED, removed.v1Info.label);
      isRealDeviceChange = true;
    }
    if (isRealDeviceChange) {
      this.realListeners_.map((l) => l(realDevices));
    }
    this.realDevices_ = realDevices;
  }

  /**
   * Computes |devices| - |devices2|.
   * @param {!Array<!DeviceInfo>} devices
   * @param {!Array<!DeviceInfo>} devices2
   * @return {!Array<!DeviceInfo>}
   */
  getDifference_(devices, devices2) {
    const ids = new Set(devices2.map((d) => d.v1Info.deviceId));
    return devices.filter((d) => !ids.has(d.v1Info.deviceId));
  }

  /**
   * Enumerates all available devices and gets their MediaDeviceInfo.
   * @return {!Promise<!Array<!MediaDeviceInfo>>}
   * @throws {!Error}
   * @private
   */
  async enumerateDevices_() {
    const devices = (await navigator.mediaDevices.enumerateDevices())
                        .filter((device) => device.kind === 'videoinput');

    const deviceType = loadTimeData.getDeviceType();
    const shouldHaveBuiltinCamera =
        deviceType === 'chromebook' || deviceType === 'chromebase';
    if (devices.length === 0 && shouldHaveBuiltinCamera) {
      throw new Error('Device list empty.');
    }
    return devices;
  }

  /**
   * Queries Camera3DeviceInfo of available devices through private mojo API.
   * @return {!Promise<?Array<!DeviceInfo>>} Camera3DeviceInfo of available
   *     devices. Maybe null on HALv1 devices without supporting private mojo
   *     api.
   * @throws {!Error} Thrown when camera unplugging happens between enumerating
   *     devices and querying mojo APIs with current device info results.
   * @private
   */
  async queryMojoDevicesInfo_() {
    const deviceInfos = await this.devicesInfo_;
    const videoConfigFilter = await this.videoConfigFilter_;
    const isV3Supported = await DeviceOperator.isSupported();
    return Promise.all(deviceInfos.map(
        async (d) => ({
          v1Info: d,
          v3Info: isV3Supported ?
              (await Camera3DeviceInfo.create(d, videoConfigFilter)) :
              null,
        })));
  }

  /**
   * Enables/Disables multiple streams on target camera device. The extra
   * stream will be reported as virtual video device from
   * navigator.mediaDevices.enumerateDevices().
   * @param {string} deviceId The id of target camera device.
   * @param {boolean} enabled True for eanbling multiple streams.
   */
  async setMultipleStreamsEnabled(deviceId, enabled) {
    assert(await DeviceOperator.isSupported());
    let waitEvent;
    if (enabled) {
      this.waitVirtual_ = new WaitableEvent();
      waitEvent = this.waitVirtual_;
    } else {
      this.virtualMap_ = null;
    }
    const deviceOperator = await DeviceOperator.getInstance();
    await deviceOperator.setMultipleStreamsEnabled(deviceId, enabled);
    await this.deviceUpdate();
    if (enabled) {
      try {
        const virtualId = await waitEvent.timedWait(3000);
        this.virtualMap_ = {realId: deviceId, virtualId};
      } catch (e) {
        throw new Error(
            `${deviceId} set multiple streams to ${enabled} failed`);
      }
    }
  }
}
