// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// clang-format off
// #import {createPermission} from './util.m.js';
// #import {assert} from 'chrome://resources/js/assert.m.js';
// #import {AppManagementStore} from "./store.m.js";
// #import {AppType, PwaPermissionType, TriState, PermissionValueType, Bool, ArcPermissionType, OptionalBool} from "./constants.m.js";
// clang-format on

cr.define('app_management', function() {
  /**
   * @implements {appManagement.mojom.PageHandlerInterface}
   */
  /* #export */ class FakePageHandler {
    /**
     * @param {Object=} options
     * @return {!Object<number, Permission>}
     */
    static createWebPermissions(options) {
      const permissionIds = [
        PwaPermissionType.GEOLOCATION,
        PwaPermissionType.NOTIFICATIONS,
        PwaPermissionType.MEDIASTREAM_MIC,
        PwaPermissionType.MEDIASTREAM_CAMERA,
      ];

      const permissions = {};

      for (const permissionId of permissionIds) {
        let permissionValue = TriState.kAllow;
        let isManaged = false;

        if (options && options[permissionId]) {
          const opts = options[permissionId];
          permissionValue = opts.permissionValue || permissionValue;
          isManaged = opts.isManaged || isManaged;
        }
        permissions[permissionId] = app_management.util.createPermission(
            permissionId, PermissionValueType.kTriState, permissionValue,
            isManaged);
      }

      return permissions;
    }

    /**
     * @param {Array<number>=} optIds
     * @return {!Object<number, Permission>}
     */
    static createArcPermissions(optIds) {
      const permissionIds = optIds || [
        ArcPermissionType.CAMERA,
        ArcPermissionType.LOCATION,
        ArcPermissionType.MICROPHONE,
        ArcPermissionType.NOTIFICATIONS,
        ArcPermissionType.CONTACTS,
        ArcPermissionType.STORAGE,
      ];

      const permissions = {};

      for (const permissionId of permissionIds) {
        permissions[permissionId] = app_management.util.createPermission(
            permissionId, PermissionValueType.kBool, Bool.kTrue,
            false /*is_managed*/);
      }

      return permissions;
    }

    /**
     * @param {AppType} appType
     * @return {!Object<number, Permission>}
     */
    static createPermissions(appType) {
      switch (appType) {
        case (AppType.kWeb):
          return FakePageHandler.createWebPermissions();
        case (AppType.kArc):
          return FakePageHandler.createArcPermissions();
        default:
          return {};
      }
    }

    /**
     * @param {string} id
     * @param {Object=} optConfig
     * @return {!App}
     */
    static createApp(id, optConfig) {
      const app = {
        id: id,
        type: apps.mojom.AppType.kWeb,
        title: 'App Title',
        description: '',
        version: '5.1',
        size: '9.0MB',
        isPinned: apps.mojom.OptionalBool.kFalse,
        isPolicyPinned: apps.mojom.OptionalBool.kFalse,
        installSource: apps.mojom.InstallSource.kUser,
        permissions: {},
        hideMoreSettings: false,
        hidePinToShelf: false,
        isPreferredApp: false,
        windowMode: apps.mojom.WindowMode.kWindow,
        resizeLocked: false,
        hideResizeLocked: true,
        supportedLinks: [],
      };

      if (optConfig) {
        Object.assign(app, optConfig);
      }

      // Only create default permissions if none were provided in the config.
      if (!optConfig || optConfig.permissions === undefined) {
        app.permissions = FakePageHandler.createPermissions(app.type);
      }

      return app;
    }

    /**
     * @param {appManagement.mojom.PageRemote} page
     */
    constructor(page) {
      this.receiver_ = new appManagement.mojom.PageHandlerReceiver(this);
      /** @type {appManagement.mojom.PageRemote} */
      this.page = page;

      /** @type {!Array<App>} */
      this.apps_ = [];

      /** @type {number} */
      this.guid = 0;
    }

    /**
     * @returns {!appManagement.mojom.PageHandlerRemote}
     */
    getRemote() {
      return this.receiver_.$.bindNewPipeAndPassRemote();
    }

    async flushPipesForTesting() {
      await this.page.$.flushForTesting();
    }

    async getApps() {
      return {apps: this.apps_};
    }

    /**
     * @param {string} appId
     * @return {!Promise}
     */
    async getExtensionAppPermissionMessages(appId) {
      return [];
    }

    /**
     * @param {!Array<App>} appList
     */
    setApps(appList) {
      this.apps_ = appList;
    }

    /**
     * @param {string} appId
     * @param {OptionalBool} pinnedValue
     */
    setPinned(appId, pinnedValue) {
      const app =
          app_management.AppManagementStore.getInstance().data.apps[appId];

      const newApp =
          /** @type {!App} */ (Object.assign({}, app, {isPinned: pinnedValue}));
      this.page.onAppChanged(newApp);
    }

    /**
     * @param {string} appId
     * @param {Permission} permission
     */
    setPermission(appId, permission) {
      const app =
          app_management.AppManagementStore.getInstance().data.apps[appId];

      // Check that the app had a previous value for the given permission
      assert(app.permissions[permission.permissionId]);

      const newPermissions = Object.assign({}, app.permissions);
      newPermissions[permission.permissionId] = permission;
      const newApp = /** @type {!App} */ (
          Object.assign({}, app, {permissions: newPermissions}));
      this.page.onAppChanged(newApp);
    }

    /**
     * @param {string} appId
     * @param {boolean} locked
     */
    setResizeLocked(appId, locked) {
      const app =
          app_management.AppManagementStore.getInstance().data.apps[appId];

      const newApp =
          /** @type {!App} */ (Object.assign({}, app, {resizeLocked: locked}));
      this.page.onAppChanged(newApp);
    }

    /**
     * @param {string} appId
     * @param {boolean} hide
     */
    setHideResizeLocked(appId, hide) {
      const app =
          app_management.AppManagementStore.getInstance().data.apps[appId];

      const newApp =
          /** @type {!App} */ (
              Object.assign({}, app, {hideResizeLocked: hide}));
      this.page.onAppChanged(newApp);
    }

    /**
     * @param {string} appId
     */
    uninstall(appId) {
      this.page.onAppRemoved(appId);
    }

    /**
     * @param {string} appId
     * @param {boolean} preferredAppValue
     */
    setPreferredApp(appId, preferredAppValue) {
      const app =
          app_management.AppManagementStore.getInstance().data.apps[appId];

      const newApp =
          /** @type {!App} */ (
              Object.assign({}, app, {isPreferredApp: preferredAppValue}));
      this.page.onAppChanged(newApp);
    }

    /**
     * @param {string} appId
     */
    openNativeSettings(appId) {}

    /**
     * @param {string} optId
     * @param {Object=} optConfig
     * @return {!Promise<!App>}
     */
    async addApp(optId, optConfig) {
      optId = optId || String(this.guid++);
      const app = FakePageHandler.createApp(optId, optConfig);
      this.page.onAppAdded(app);
      await this.flushPipesForTesting();
      return app;
    }

    /**
     * Takes an app id and an object mapping app fields to the values they
     * should be changed to, and dispatches an action to carry out these
     * changes.
     * @param {string} id
     * @param {Object} changes
     */
    async changeApp(id, changes) {
      this.page.onAppChanged(FakePageHandler.createApp(id, changes));
      await this.flushPipesForTesting();
    }
  }

  // #cr_define_end
  return {FakePageHandler: FakePageHandler};
});
