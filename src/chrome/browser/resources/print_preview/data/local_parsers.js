// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {assertNotReached} from 'chrome://resources/js/assert.m.js';
import {isChromeOS, isLacros} from 'chrome://resources/js/cr.m.js';

import {Destination, DestinationConnectionStatus, DestinationOrigin, DestinationProvisionalType, DestinationType} from './destination.js';
import {PrinterType} from './destination_match.js';

/**
 * @typedef {{
 *   deviceName: string,
 *   printerName: string,
 *   printerDescription: (string | undefined),
 *   cupsEnterprisePrinter: (boolean | undefined),
 *   printerOptions: (Object | undefined),
 * }}
 */
export let LocalDestinationInfo;

/**
 * @typedef {{
 *   serviceName: string,
 *   name: string,
 *   hasLocalPrinting: boolean,
 *   isUnregistered: boolean,
 *   cloudID: string,
 * }}
 * @see PrintPreviewHandler::FillPrinterDescription in
 * print_preview_handler.cc
 */
export let PrivetPrinterDescription;

/**
 * @typedef {{
 *   extensionId: string,
 *   extensionName: string,
 *   id: string,
 *   name: string,
 *   description: (string|undefined),
 * }}
 */
export let ProvisionalDestinationInfo;

/**
 * @param{!PrinterType} type The type of printer to parse.
 * @param{!LocalDestinationInfo |
 *        !PrivetPrinterDescription |
 *        !ProvisionalDestinationInfo} printer Information
 *     about the printer. Type expected depends on |type|:
 *       For LOCAL_PRINTER => LocalDestinationInfo
 *       For EXTENSION_PRINTER => ProvisionalDestinationInfo
 * @return {?Destination} Only returns null if an invalid value
 *     is provided for |type|.
 */
export function parseDestination(type, printer) {
  if (type === PrinterType.LOCAL_PRINTER) {
    return parseLocalDestination(
        /** @type {!LocalDestinationInfo} */ (printer));
  }
  if (type === PrinterType.EXTENSION_PRINTER) {
    return parseExtensionDestination(
        /** @type {!ProvisionalDestinationInfo} */ (printer));
  }
  assertNotReached('Unknown printer type ' + type);
  return null;
}

/**
 * Parses a local print destination.
 * @param {!LocalDestinationInfo} destinationInfo Information
 *     describing a local print destination.
 * @return {!Destination} Parsed local print destination.
 */
function parseLocalDestination(destinationInfo) {
  const options = {
    description: destinationInfo.printerDescription,
    isEnterprisePrinter: destinationInfo.cupsEnterprisePrinter,
  };
  if (destinationInfo.printerOptions) {
    // Convert options into cloud print tags format.
    options.tags =
        Object.keys(destinationInfo.printerOptions).map(function(key) {
          return '__cp__' + key + '=' + this[key];
        }, destinationInfo.printerOptions);
  }
  return new Destination(
      destinationInfo.deviceName, DestinationType.LOCAL,
      (isChromeOS || isLacros) ? DestinationOrigin.CROS :
                                 DestinationOrigin.LOCAL,
      destinationInfo.printerName, DestinationConnectionStatus.ONLINE, options);
}

/**
 * Parses an extension destination from an extension supplied printer
 * description.
 * @param {!ProvisionalDestinationInfo} destinationInfo Object
 *     describing an extension printer.
 * @return {!Destination} Parsed destination.
 */
export function parseExtensionDestination(destinationInfo) {
  const provisionalType = destinationInfo.provisional ?
      DestinationProvisionalType.NEEDS_USB_PERMISSION :
      DestinationProvisionalType.NONE;

  return new Destination(
      destinationInfo.id, DestinationType.LOCAL, DestinationOrigin.EXTENSION,
      destinationInfo.name, DestinationConnectionStatus.ONLINE, {
        description: destinationInfo.description || '',
        extensionId: destinationInfo.extensionId,
        extensionName: destinationInfo.extensionName || '',
        provisionalType: provisionalType
      });
}
