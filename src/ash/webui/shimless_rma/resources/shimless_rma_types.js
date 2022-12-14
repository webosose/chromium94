// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Type aliases for the mojo API.
 */

import {OncMojo} from 'chrome://resources/cr_components/chromeos/network/onc_mojo.m.js';
import 'chrome://resources/mojo/mojo/public/js/mojo_bindings_lite.js';
import 'chrome://resources/mojo/mojo/public/mojom/base/big_buffer.mojom-lite.js';
import 'chrome://resources/mojo/mojo/public/mojom/base/string16.mojom-lite.js';
import './mojom/shimless_rma.mojom-lite.js';

/**
 * Return type from state progression methods.
 * Convenience type as mojo-lite does not define types for method results and
 * this is used frequently.
 * @typedef {{state: !RmaState, error: !RmadErrorCode}}
 */
export let StateResult;

/**
 * @typedef {ash.shimlessRma.mojom.RmaState}
 */
export let RmaState = ash.shimlessRma.mojom.RmaState;

/**
 * @typedef {ash.shimlessRma.mojom.RmadErrorCode}
 */
export let RmadErrorCode = ash.shimlessRma.mojom.RmadErrorCode;

/**
 * @typedef {ash.shimlessRma.mojom.QrCode}
 */
export let QrCode = ash.shimlessRma.mojom.QrCode;

/**
 * @typedef {ash.shimlessRma.mojom.ComponentType}
 */
export let ComponentType = ash.shimlessRma.mojom.ComponentType;

/**
 * @typedef {ash.shimlessRma.mojom.ComponentRepairStatus}
 */
export let ComponentRepairStatus = ash.shimlessRma.mojom.ComponentRepairStatus;

/**
 * @typedef {ash.shimlessRma.mojom.CalibrationComponent}
 */
export let CalibrationComponent = ash.shimlessRma.mojom.CalibrationComponent;

/**
 * @typedef {ash.shimlessRma.mojom.ProvisioningStep}
 */
export let ProvisioningStep = ash.shimlessRma.mojom.ProvisioningStep;

/**
 * @typedef {ash.shimlessRma.mojom.Component}
 */
export let Component = ash.shimlessRma.mojom.Component;

/**
 * Type alias for ErrorObserverRemote.
 * @typedef {ash.shimlessRma.mojom.ErrorObserverRemote}
 */
export let ErrorObserverRemote = ash.shimlessRma.mojom.ErrorObserverRemote;

/**
 * Type alias for CalibrationObserverRemote.
 * @typedef {ash.shimlessRma.mojom.CalibrationObserverRemote}
 */
export let CalibrationObserverRemote =
    ash.shimlessRma.mojom.CalibrationObserverRemote;

/**
 * Type alias for CalibrationObserverReceiver.
 * @typedef {ash.shimlessRma.mojom.CalibrationObserverReceiver}
 */
export let CalibrationObserverReceiver =
    ash.shimlessRma.mojom.CalibrationObserverReceiver;

/**
 * Type alias for CalibrationObserverInterface.
 * @typedef {ash.shimlessRma.mojom.CalibrationObserverInterface}
 */
export let CalibrationObserverInterface =
    ash.shimlessRma.mojom.CalibrationObserverInterface;

/**
 * Type alias for ProvisioningObserverRemote.
 * @typedef {ash.shimlessRma.mojom.ProvisioningObserverRemote}
 */
export let ProvisioningObserverRemote =
    ash.shimlessRma.mojom.ProvisioningObserverRemote;

/**
 * Type alias for ProvisioningObserverReceiver.
 * @typedef {ash.shimlessRma.mojom.ProvisioningObserverReceiver}
 */
export let ProvisioningObserverReceiver =
    ash.shimlessRma.mojom.ProvisioningObserverReceiver;

/**
 * Type alias for ProvisioningObserverInterface.
 * @typedef {ash.shimlessRma.mojom.ProvisioningObserverInterface}
 */
export let ProvisioningObserverInterface =
    ash.shimlessRma.mojom.ProvisioningObserverInterface;

/**
 * Type alias for HardwareWriteProtectionStateObserverRemote.
 * @typedef {ash.shimlessRma.mojom.HardwareWriteProtectionStateObserverRemote}
 */
export let HardwareWriteProtectionStateObserverRemote =
    ash.shimlessRma.mojom.HardwareWriteProtectionStateObserverRemote;

/**
 * Type alias for HardwareWriteProtectionStateObserverReceiver.
 * @typedef {ash.shimlessRma.mojom.HardwareWriteProtectionStateObserverReceiver}
 */
export let HardwareWriteProtectionStateObserverReceiver =
    ash.shimlessRma.mojom.HardwareWriteProtectionStateObserverReceiver;

/**
 * Type alias for HardwareWriteProtectionStateObserverInterface.
 * @typedef {
 *    ash.shimlessRma.mojom.HardwareWriteProtectionStateObserverInterface
 * }
 */
export let HardwareWriteProtectionStateObserverInterface =
    ash.shimlessRma.mojom.HardwareWriteProtectionStateObserverInterface;

/**
 * Type alias for PowerCableStateObserverRemote.
 * @typedef {ash.shimlessRma.mojom.PowerCableStateObserverRemote}
 */
export let PowerCableStateObserverRemote =
    ash.shimlessRma.mojom.PowerCableStateObserverRemote;

/**
 * Type alias for the ShimlessRmaService.
 * @typedef {ash.shimlessRma.mojom.ShimlessRmaService}
 */
export let ShimlessRmaService =
    ash.shimlessRma.mojom.ShimlessRmaService;

/**
 * Type alias for the ShimlessRmaServiceInterface.
 * @typedef {ash.shimlessRma.mojom.ShimlessRmaServiceInterface}
 */
export let ShimlessRmaServiceInterface =
    ash.shimlessRma.mojom.ShimlessRmaServiceInterface;

/**
 * Type alias for NetworkConfigServiceInterface.
 * @typedef {chromeos.networkConfig.mojom.CrosNetworkConfigInterface}
 */
export let NetworkConfigServiceInterface =
    chromeos.networkConfig.mojom.CrosNetworkConfigInterface;

/**
 * Type alias for NetworkConfigServiceRemote.
 * @typedef {chromeos.networkConfig.mojom.CrosNetworkConfigRemote}
 */
export let NetworkConfigServiceRemote =
    chromeos.networkConfig.mojom.CrosNetworkConfigRemote;

/**
 * Type alias for Network
 * @typedef {chromeos.networkConfig.mojom.NetworkStateProperties}
 */
export let Network = chromeos.networkConfig.mojom.NetworkStateProperties;
