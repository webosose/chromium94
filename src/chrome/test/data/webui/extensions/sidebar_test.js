// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extension-sidebar. */
import {navigation, Page} from 'chrome://extensions/extensions.js';
import {assert} from 'chrome://resources/js/assert.m.js';
import {flush} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {eventToPromise} from '../test_util.m.js';

import {testVisible} from './test_util.js';

window.extension_sidebar_tests = {};
extension_sidebar_tests.suiteName = 'ExtensionSidebarTest';

/** @enum {string} */
extension_sidebar_tests.TestNames = {
  LayoutAndClickHandlers: 'layout and click handlers',
  SetSelected: 'set selected',
};

suite(extension_sidebar_tests.suiteName, function() {
  /** @type {extensions.Sidebar} */
  let sidebar;

  setup(function() {
    document.body.innerHTML = '';
    sidebar = document.createElement('extensions-sidebar');
    document.body.appendChild(sidebar);
  });

  test(assert(extension_sidebar_tests.TestNames.SetSelected), function() {
    const selector = '.section-item.iron-selected';
    expectFalse(!!sidebar.shadowRoot.querySelector(selector));

    window.history.replaceState(undefined, '', '/shortcuts');
    document.body.innerHTML = '';
    sidebar = document.createElement('extensions-sidebar');
    document.body.appendChild(sidebar);
    const whenSelected = eventToPromise('iron-select', sidebar.$.sectionMenu);
    flush();
    return whenSelected
        .then(function() {
          expectEquals(
              sidebar.shadowRoot.querySelector(selector).id,
              'sections-shortcuts');

          window.history.replaceState(undefined, '', '/');
          document.body.innerHTML = '';
          sidebar = document.createElement('extensions-sidebar');
          document.body.appendChild(sidebar);
          const whenSelected =
              eventToPromise('iron-select', sidebar.$.sectionMenu);
          flush();
          return whenSelected;
        })
        .then(function() {
          expectEquals(
              sidebar.shadowRoot.querySelector(selector).id,
              'sections-extensions');
        });
  });

  test(
      assert(extension_sidebar_tests.TestNames.LayoutAndClickHandlers),
      function(done) {
        const boundTestVisible = testVisible.bind(null, sidebar);
        boundTestVisible('#sections-extensions', true);
        boundTestVisible('#sections-shortcuts', true);
        boundTestVisible('#more-extensions', true);

        let currentPage;
        navigation.addListener(newPage => {
          currentPage = newPage;
        });

        sidebar.shadowRoot.querySelector('#sections-shortcuts').click();
        expectDeepEquals(currentPage, {page: Page.SHORTCUTS});

        sidebar.shadowRoot.querySelector('#sections-extensions').click();
        expectDeepEquals(currentPage, {page: Page.LIST});

        // Clicking on the link for the current page should close the dialog.
        sidebar.addEventListener('close-drawer', () => done());
        sidebar.shadowRoot.querySelector('#sections-extensions').click();
      });
});
