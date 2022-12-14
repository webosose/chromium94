// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import org.chromium.base.FeatureList;
import org.chromium.content.browser.ContentFeatureListImpl;

/**
 * Static public methods for ContentFeatureList.
 */
public class ContentFeatureList {
    private ContentFeatureList() {}

    /**
     * Returns whether the specified feature is enabled or not.
     *
     * @param featureName The name of the feature to query.
     * @return Whether the feature is enabled or not.
     */
    public static boolean isEnabled(String featureName) {
        Boolean testValue = FeatureList.getTestValueForFeature(featureName);
        if (testValue != null) return testValue;
        return ContentFeatureListImpl.isEnabled(featureName);
    }

    // Alphabetical:
    public static final String ACCESSIBILITY_PAGE_ZOOM = "AccessibilityPageZoom";

    public static final String ACCESSIBILITY_PAGE_ZOOM_UPDATED_UI =
            "AccessibilityPageZoomUpdatedUI";

    public static final String BACKGROUND_MEDIA_RENDERER_HAS_MODERATE_BINDING =
            "BackgroundMediaRendererHasModerateBinding";

    public static final String BINDING_MANAGEMENT_WAIVE_CPU = "BindingManagementWaiveCpu";

    public static final String EXPERIMENTAL_ACCESSIBILITY_LABELS =
            "ExperimentalAccessibilityLabels";

    public static final String ON_DEMAND_ACCESSIBILITY_EVENTS = "OnDemandAccessibilityEvents";

    public static final String PROCESS_SHARING_WITH_STRICT_SITE_INSTANCES =
            "ProcessSharingWithStrictSiteInstances";

    public static final String WEB_AUTH = "WebAuthentication";

    public static final String WEB_BLUETOOTH_NEW_PERMISSIONS_BACKEND =
            "WebBluetoothNewPermissionsBackend";

    public static final String WEB_NFC = "WebNFC";
}
