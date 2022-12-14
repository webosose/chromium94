// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {assertNotReached} from 'chrome://resources/js/assert.m.js';
import {PrinterType} from './data/destination_match.js';
import {NativeLayer, NativeLayerImpl} from './native_layer.js';

/**
 * Object used to measure usage statistics.
 * @constructor
 */
export function Metrics() {}

/**
 * Enumeration of buckets that a user can enter while using the destination
 * search widget.
 * @enum {number}
 */
Metrics.DestinationSearchBucket = {
  // Used when the print destination search widget is shown.
  DESTINATION_SHOWN: 0,
  // Used when the user selects a print destination.
  DESTINATION_CLOSED_CHANGED: 1,
  // Used when the print destination search widget is closed without selecting
  // a print destination.
  DESTINATION_CLOSED_UNCHANGED: 2,
  // Used when the Google Cloud Print promotion (shown in the destination
  // search widget) is shown to the user.
  SIGNIN_PROMPT: 3,
  // Used when the user chooses to sign-in to their Google account.
  SIGNIN_TRIGGERED: 4,
  // Used when a user selects the Privet printer in a pair of duplicate
  // Privet and cloud printers.
  PRIVET_DUPLICATE_SELECTED: 5,
  // Used when a user selects the cloud printer in a pair of duplicate
  // Privet and cloud printers.
  CLOUD_DUPLICATE_SELECTED: 6,
  // Used when a user sees a register promo for a cloud print printer.
  REGISTER_PROMO_SHOWN: 7,
  // Used when a user selects a register promo for a cloud print printer.
  REGISTER_PROMO_SELECTED: 8,
  // User changed active account.
  ACCOUNT_CHANGED: 9,
  // User tried to log into another account.
  ADD_ACCOUNT_SELECTED: 10,
  // Note: values 11-13 are intentionally unset as these correspond to
  // deprecated values in histograms/enums.xml. These enums are append-only.
  // User clicked on Manage button
  MANAGE_BUTTON_CLICKED: 14,
  // Max value.
  DESTINATION_SEARCH_MAX_BUCKET: 15
};

/**
 * Print Preview initialization events metrics buckets.
 * @enum {number}
 */
Metrics.PrintPreviewInitializationEvents = {
  // Function initiated.
  FUNCTION_INITIATED: 0,
  // Function completed succesfully.
  FUNCTION_SUCCESSFUL: 1,
  // Function failed.
  FUNCTION_FAILED: 2,
  // Max value.
  PRINT_PREVIEW_INITIALIZATION_EVENTS_MAX_BUCKET: 3
};

/**
 * Print settings UI usage metrics buckets.
 * @enum {number}
 */
Metrics.PrintSettingsUiBucket = {
  // Advanced settings dialog is shown.
  ADVANCED_SETTINGS_DIALOG_SHOWN: 0,
  // Advanced settings dialog is closed without saving a selection.
  ADVANCED_SETTINGS_DIALOG_CANCELED: 1,
  // 'More/less settings' expanded.
  MORE_SETTINGS_CLICKED: 2,
  // 'More/less settings' collapsed.
  LESS_SETTINGS_CLICKED: 3,
  // User printed with extra settings expanded.
  PRINT_WITH_SETTINGS_EXPANDED: 4,
  // User printed with extra settings collapsed.
  PRINT_WITH_SETTINGS_COLLAPSED: 5,
  // Max value.
  PRINT_SETTINGS_UI_MAX_BUCKET: 6
};

/* A context for recording a value in a specific UMA histogram. */
export class MetricsContext {
  /**
   * @param {string} histogram The name of the histogram to be recorded in.
   * @param {number} maxBucket The max value for the last histogram bucket.
   */
  constructor(histogram, maxBucket) {
    /** @private {string} */
    this.histogram_ = histogram;

    /** @private {number} */
    this.maxBucket_ = maxBucket;

    /** @private {!NativeLayer} */
    this.nativeLayer_ = NativeLayerImpl.getInstance();
  }

  /**
   * Record a histogram value in UMA. If specified value is larger than the
   * max bucket value, record the value in the largest bucket
   * @param {number} bucket Value to record.
   */
  record(bucket) {
    this.nativeLayer_.recordInHistogram(
        this.histogram_, (bucket > this.maxBucket_) ? this.maxBucket_ : bucket,
        this.maxBucket_);
  }

  /**
   * Destination Search specific usage statistics context.
   * @return {!MetricsContext}
   */
  static destinationSearch() {
    return new MetricsContext(
        'PrintPreview.DestinationAction',
        Metrics.DestinationSearchBucket.DESTINATION_SEARCH_MAX_BUCKET);
  }

  /**
   * Print settings UI specific usage statistics context
   * @return {!MetricsContext}
   */
  static printSettingsUi() {
    return new MetricsContext(
        'PrintPreview.PrintSettingsUi',
        Metrics.PrintSettingsUiBucket.PRINT_SETTINGS_UI_MAX_BUCKET);
  }

  /**
   * NativeLayer.getInitialSettings() specific usage statistics context
   * @return {!MetricsContext}
   */
  static getInitialSettings() {
    return new MetricsContext(
        'PrintPreview.Initialization.GetInitialSettings',
        Metrics.PrintPreviewInitializationEvents
            .PRINT_PREVIEW_INITIALIZATION_EVENTS_MAX_BUCKET);
  }

  /**
   * NativeLayer.getPrinterCapabilities() specific usage statistics context
   * @return {!MetricsContext}
   */
  static getPrinterCapabilities() {
    return new MetricsContext(
        'PrintPreview.Initialization.GetPrinterCapabilities',
        Metrics.PrintPreviewInitializationEvents
            .PRINT_PREVIEW_INITIALIZATION_EVENTS_MAX_BUCKET);
  }

  /**
   * NativeLayer.getPreview() specific usage statistics context
   * @return {!MetricsContext}
   */
  static getPreview() {
    return new MetricsContext(
        'PrintPreview.Initialization.GetPreview',
        Metrics.PrintPreviewInitializationEvents
            .PRINT_PREVIEW_INITIALIZATION_EVENTS_MAX_BUCKET);
  }

  /**
   * NativeLayer.getPrinters() specific usage statistics context
   * @param {!PrinterType} type
   * @return {!MetricsContext}
   */
  static getPrinters(type) {
    let histogram = '';
    switch (type) {
      case (PrinterType.EXTENSION_PRINTER):
        histogram = 'PrintPreview.Initialization.GetPrinters.Extension';
        break;
      case (PrinterType.PDF_PRINTER):
        histogram = 'PrintPreview.Initialization.GetPrinters.PDF';
        break;
      case (PrinterType.LOCAL_PRINTER):
        histogram = 'PrintPreview.Initialization.GetPrinters.Local';
        break;
      case (PrinterType.CLOUD_PRINTER):
        histogram = 'PrintPreview.Initialization.GetPrinters.Cloud';
        break;
      default:
        assertNotReached();
    }
    return new MetricsContext(
        histogram,
        Metrics.PrintPreviewInitializationEvents
            .PRINT_PREVIEW_INITIALIZATION_EVENTS_MAX_BUCKET);
  }
}
