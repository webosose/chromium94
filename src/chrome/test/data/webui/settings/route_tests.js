// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// clang-format off
import {isChromeOS} from 'chrome://resources/js/cr.m.js';
import {buildRouter, pageVisibility, Route, Router, routes, setPageVisibilityForTesting} from 'chrome://settings/settings.js';

// clang-format on

suite('route', function() {
  /**
   * Returns a new promise that resolves after a window 'popstate' event.
   * @return {!Promise}
   */
  function whenPopState(causeEvent) {
    const promise = new Promise(function(resolve) {
      window.addEventListener('popstate', function callback() {
        window.removeEventListener('popstate', callback);
        resolve();
      });
    });

    causeEvent();
    return promise;
  }

  teardown(function() {
    PolymerTest.clearBody();
  });

  /**
   * Tests a specific navigation situation.
   * @param {!Route} previousRoute
   * @param {!Route} currentRoute
   * @param {!Route} expectedNavigatePreviousResult
   * @return {!Promise}
   */
  function testNavigateBackUsesHistory(
      previousRoute, currentRoute, expectedNavigatePreviousResult) {
    Router.getInstance().navigateTo(previousRoute);
    Router.getInstance().navigateTo(currentRoute);

    return whenPopState(function() {
             Router.getInstance().navigateToPreviousRoute();
           })
        .then(function() {
          assertEquals(
              expectedNavigatePreviousResult,
              Router.getInstance().getCurrentRoute());
        });
  }

  /**
   * Tests that after navigating from |previousRoute| to |currentRoute1| with
   * pushing a new history entry, and then navigating from |currentRoute1| to
   * |currentRoute2| without pushing a new history entry, navigating back lands
   * at |expectedRoute|.
   * @param {!Route} route0
   * @param {!Route} route1
   * @param {!Route} route2
   * @param {!Route} expectedRoute
   * @return {!Promise}
   */
  async function testNavigateBackIgnoresNonHistoryNavigation(
      route0, route1, route2, expectedRoute) {
    Router.getInstance().navigateTo(route0);
    Router.getInstance().navigateTo(route1);
    Router.getInstance().navigateTo(
        route2, /* URLSearchParams */ null, /* opt_removeSearch */ false,
        /* opt_skipHistoryEntry */ true);

    await whenPopState(function() {
      Router.getInstance().navigateToPreviousRoute();
    });
    assertEquals(expectedRoute, Router.getInstance().getCurrentRoute());
  }

  test('tree structure', function() {
    // Set up root page routes.
    const BASIC = new Route('/');
    assertEquals(0, BASIC.depth);

    const ADVANCED = new Route('/advanced');
    assertFalse(ADVANCED.isSubpage());
    assertEquals(0, ADVANCED.depth);

    // Test a section route.
    const PRIVACY = ADVANCED.createSection('/privacy', 'privacy');
    assertEquals(ADVANCED, PRIVACY.parent);
    assertEquals(1, PRIVACY.depth);
    assertFalse(PRIVACY.isSubpage());
    assertFalse(BASIC.contains(PRIVACY));
    assertTrue(ADVANCED.contains(PRIVACY));
    assertTrue(PRIVACY.contains(PRIVACY));
    assertFalse(PRIVACY.contains(ADVANCED));

    // Test a subpage route.
    const SITE_SETTINGS = PRIVACY.createChild('/siteSettings');
    assertEquals('/siteSettings', SITE_SETTINGS.path);
    assertEquals(PRIVACY, SITE_SETTINGS.parent);
    assertEquals(2, SITE_SETTINGS.depth);
    assertFalse(!!SITE_SETTINGS.dialog);
    assertTrue(SITE_SETTINGS.isSubpage());
    assertEquals('privacy', SITE_SETTINGS.section);
    assertFalse(BASIC.contains(SITE_SETTINGS));
    assertTrue(ADVANCED.contains(SITE_SETTINGS));
    assertTrue(PRIVACY.contains(SITE_SETTINGS));

    // Test a sub-subpage route.
    const SITE_SETTINGS_ALL = SITE_SETTINGS.createChild('all');
    assertEquals('/siteSettings/all', SITE_SETTINGS_ALL.path);
    assertEquals(SITE_SETTINGS, SITE_SETTINGS_ALL.parent);
    assertEquals(3, SITE_SETTINGS_ALL.depth);
    assertTrue(SITE_SETTINGS_ALL.isSubpage());

    // Test a non-subpage child of ADVANCED.
    const CLEAR_BROWSER_DATA = ADVANCED.createChild('/clearBrowserData');
    assertFalse(CLEAR_BROWSER_DATA.isSubpage());
    assertEquals('', CLEAR_BROWSER_DATA.section);
  });

  test('no duplicate routes', function() {
    const paths = new Set();
    Object.values(routes).forEach(function(route) {
      assertFalse(paths.has(route.path), route.path);
      paths.add(route.path);
    });
  });

  test('navigate back to parent previous route', function() {
    return testNavigateBackUsesHistory(
        routes.BASIC, routes.PEOPLE, routes.BASIC);
  });

  test(
      'navigate back to parent previous route, ignore non-history navigation',
      function() {
        return testNavigateBackIgnoresNonHistoryNavigation(
            routes.BASIC, routes.PEOPLE, routes.SYNC, routes.BASIC);
      });

  test('navigate back to non-ancestor shallower route', function() {
    return testNavigateBackUsesHistory(
        routes.ADVANCED, routes.PEOPLE, routes.BASIC);
  });

  test(
      'navigate back to non-ancestor shallower route, ignore non-history navigation',
      function() {
        return testNavigateBackIgnoresNonHistoryNavigation(
            routes.ADVANCED, routes.PEOPLE, routes.SYNC, routes.BASIC);
      });

  test('navigate back to sibling route', function() {
    return testNavigateBackUsesHistory(
        routes.APPEARANCE, routes.PEOPLE, routes.APPEARANCE);
  });

  test(
      'navigate back to sibling route, ignore non-history navigation',
      function() {
        return testNavigateBackIgnoresNonHistoryNavigation(
            routes.APPEARANCE, routes.PEOPLE, routes.SYNC, routes.APPEARANCE);
      });

  test('navigate back to parent when previous route is deeper', function() {
    Router.getInstance().navigateTo(routes.SYNC);
    Router.getInstance().navigateTo(routes.PEOPLE);
    Router.getInstance().navigateToPreviousRoute();
    assertEquals(routes.BASIC, Router.getInstance().getCurrentRoute());
  });

  test('navigate back to BASIC when going back from root pages', function() {
    Router.getInstance().navigateTo(routes.PEOPLE);
    Router.getInstance().navigateTo(routes.ADVANCED);
    Router.getInstance().navigateToPreviousRoute();
    assertEquals(routes.BASIC, Router.getInstance().getCurrentRoute());
  });

  test('navigateTo respects removeSearch optional parameter', function() {
    const params = new URLSearchParams('search=foo');
    Router.getInstance().navigateTo(routes.BASIC, params);
    assertEquals(
        params.toString(),
        Router.getInstance().getQueryParameters().toString());

    Router.getInstance().navigateTo(
        routes.SITE_SETTINGS, null,
        /* removeSearch */ false);
    assertEquals(
        params.toString(),
        Router.getInstance().getQueryParameters().toString());

    Router.getInstance().navigateTo(
        routes.SEARCH_ENGINES, null,
        /* removeSearch */ true);
    assertEquals('', Router.getInstance().getQueryParameters().toString());
  });

  test('navigateTo ADVANCED forwards to BASIC', function() {
    Router.getInstance().navigateTo(routes.ADVANCED);
    assertEquals(routes.BASIC, Router.getInstance().getCurrentRoute());
  });

  test('popstate flag works', function() {
    const router = Router.getInstance();
    router.navigateTo(routes.BASIC);
    assertFalse(router.lastRouteChangeWasPopstate());

    router.navigateTo(routes.PEOPLE);
    assertFalse(router.lastRouteChangeWasPopstate());

    return whenPopState(function() {
             window.history.back();
           })
        .then(function() {
          assertEquals(routes.BASIC, router.getCurrentRoute());
          assertTrue(router.lastRouteChangeWasPopstate());

          router.navigateTo(routes.ADVANCED);
          assertFalse(router.lastRouteChangeWasPopstate());
        });
  });

  test('getRouteForPath trailing slashes', function() {
    assertEquals(routes.BASIC, Router.getInstance().getRouteForPath('/'));
    assertEquals(null, Router.getInstance().getRouteForPath('//'));

    // Simple path.
    assertEquals(
        routes.PEOPLE, Router.getInstance().getRouteForPath('/people/'));
    assertEquals(
        routes.PEOPLE, Router.getInstance().getRouteForPath('/people'));

    // Path with a slash.
    assertEquals(
        routes.SITE_SETTINGS_SITE_DETAILS,
        Router.getInstance().getRouteForPath('/content/siteDetails/'));
    assertEquals(
        routes.SITE_SETTINGS_SITE_DETAILS,
        Router.getInstance().getRouteForPath('/content/siteDetails'));
  });

  test('isNavigableDialog', function() {
    assertTrue(routes.CLEAR_BROWSER_DATA.isNavigableDialog);
    assertTrue(routes.CLEAR_BROWSER_DATA.parent === routes.PRIVACY);
    assertFalse(routes.CLEAR_BROWSER_DATA.isSubpage());

    assertTrue(routes.RESET_DIALOG.isNavigableDialog);
    assertTrue(routes.RESET_DIALOG.parent === routes.RESET);
    assertTrue(routes.TRIGGERED_RESET_DIALOG.isNavigableDialog);
    assertTrue(routes.TRIGGERED_RESET_DIALOG.parent === routes.RESET);

    assertTrue(routes.SIGN_OUT.isNavigableDialog);
    assertTrue(routes.SIGN_OUT.parent === routes.PEOPLE);

    if (!isChromeOS) {
      assertTrue(routes.IMPORT_DATA.isNavigableDialog);
      assertTrue(routes.IMPORT_DATA.parent === routes.PEOPLE);
    }

    assertFalse(routes.PRIVACY.isNavigableDialog);
    // <if expr="not chromeos and not lacros">
    assertFalse(routes.DEFAULT_BROWSER.isNavigableDialog);
    // </if>
  });

  test('pageVisibility affects route availability', function() {
    setPageVisibilityForTesting({
      appearance: false,
      autofill: false,
      defaultBrowser: false,
      onStartup: false,
      reset: false,
    });

    const router = buildRouter();
    const hasRoute = route => router.getRoutes().hasOwnProperty(route);

    assertTrue(hasRoute('BASIC'));

    assertFalse(hasRoute('APPEARANCE'));
    assertFalse(hasRoute('AUTOFILL'));
    assertFalse(hasRoute('DEFAULT_BROWSER'));
    assertFalse(hasRoute('ON_STARTUP'));
    assertFalse(hasRoute('RESET'));
  });

  test(
      'getAbsolutePath works in direct and within-settings navigation',
      function() {
        Router.getInstance().resetRouteForTesting();
        // Check getting the absolute path while not inside settings returns the
        // correct path.
        window.location.href = 'https://example.com/path/to/page.html';
        assertEquals(
            'chrome://settings/cookies', routes.COOKIES.getAbsolutePath());

        // Check getting the absolute path while inside settings returns the
        // correct path for the current route and a different route.
        Router.getInstance().navigateTo(routes.DOWNLOADS);
        assertEquals(
            'chrome://settings/downloads',
            Router.getInstance().getCurrentRoute().getAbsolutePath());
        assertEquals(
            'chrome://settings/languages', routes.LANGUAGES.getAbsolutePath());
      });
});

suite('DynamicParameters', function() {
  setup(function() {
    PolymerTest.clearBody();
    window.history.replaceState({}, '', 'search?guid=a%2Fb&foo=42');
    const settingsUi = document.createElement('settings-ui');
    document.body.appendChild(settingsUi);
  });

  test('get parameters from URL and navigation', function(done) {
    assertEquals(routes.SEARCH, Router.getInstance().getCurrentRoute());
    assertEquals('a/b', Router.getInstance().getQueryParameters().get('guid'));
    assertEquals('42', Router.getInstance().getQueryParameters().get('foo'));

    const params = new URLSearchParams();
    params.set('bar', 'b=z');
    params.set('biz', '3');
    Router.getInstance().navigateTo(routes.SEARCH_ENGINES, params);
    assertEquals(routes.SEARCH_ENGINES, Router.getInstance().getCurrentRoute());
    assertEquals('b=z', Router.getInstance().getQueryParameters().get('bar'));
    assertEquals('3', Router.getInstance().getQueryParameters().get('biz'));
    assertEquals('?bar=b%3Dz&biz=3', window.location.search);

    window.addEventListener('popstate', function(event) {
      assertEquals('/search', Router.getInstance().getCurrentRoute().path);
      assertEquals(routes.SEARCH, Router.getInstance().getCurrentRoute());
      assertEquals(
          'a/b', Router.getInstance().getQueryParameters().get('guid'));
      assertEquals('42', Router.getInstance().getQueryParameters().get('foo'));
      done();
    });
    window.history.back();
  });
});

suite('NonExistentRoute', function() {
  setup(function() {
    PolymerTest.clearBody();
    window.history.replaceState({}, '', 'non/existent/route');
    const settingsUi = document.createElement('settings-ui');
    document.body.appendChild(settingsUi);
  });

  test('redirect to basic', function() {
    assertEquals(routes.BASIC, Router.getInstance().getCurrentRoute());
    assertEquals('/', location.pathname);
  });
});
