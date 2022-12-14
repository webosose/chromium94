// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// clang-format off
// #import {Action} from 'chrome://resources/js/cr/ui/store.m.js';
// clang-format on

/**
 * @fileoverview Module for functions which produce action objects. These are
 * listed in one place to document available actions and their parameters.
 */

cr.define('app_management.actions', function() {
  /**
   * @param {App} app
   */
  /* #export */ function addApp(app) {
    return {
      name: 'add-app',
      app: app,
    };
  }

  /**
   * @param {App} app
   */
  /* #export */ function changeApp(app) {
    return {
      name: 'change-app',
      app: app,
    };
  }

  /**
   * @param {string} id
   */
  /* #export */ function removeApp(id) {
    return {
      name: 'remove-app',
      id: id,
    };
  }

  /**
   * @param {?string} appId
   */
  /* #export */ function updateSelectedAppId(appId) {
    return {
      name: 'update-selected-app-id',
      value: appId,
    };
  }

  // #cr_define_end
  return {
    addApp: addApp,
    changeApp: changeApp,
    removeApp: removeApp,
    updateSelectedAppId: updateSelectedAppId,
  };
});
