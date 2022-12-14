// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.policy;

/**
 * Delegate for cloud management functions implemented downstream for Google Chrome.
 */
public interface CloudManagementAndroidConnectionDelegate {
    /* Returns the client ID to be used in the DM token generation. */
    public String generateClientId();
}
