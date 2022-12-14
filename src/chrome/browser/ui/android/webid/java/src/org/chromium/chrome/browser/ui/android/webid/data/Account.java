// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ui.android.webid.data;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.url.GURL;

/**
 * This class holds the data used to represent a selectable account in the
 * Account Selection sheet.
 */
public class Account {
    private final String mSubject;
    private final String mEmail;
    private final String mName;
    private final String mGivenName;
    private final GURL mPictureUrl;
    private final GURL mOriginUrl;

    /**
     * @param subject Subject shown to the user.
     * @param email Email shown to the user.
     * @param givenName Given name.
     * @param picture picture URL of the avatar shown to the user.
     * @param originUrl Origin URL for the IDP.
     */
    @CalledByNative
    public Account(String subject, String email, String name, String givenName, GURL pictureUrl,
            GURL originUrl) {
        assert subject != null : "Account subject is null!";
        mSubject = subject;
        mEmail = email;
        mName = name;
        mGivenName = givenName;
        mPictureUrl = pictureUrl;
        mOriginUrl = originUrl;
    }

    public String getSubject() {
        return mSubject;
    }

    public String getEmail() {
        return mEmail;
    }

    public String getName() {
        return mName;
    }

    public String getGivenName() {
        return mGivenName;
    }

    public GURL getPictureUrl() {
        return mPictureUrl;
    }

    public GURL getOriginUrl() {
        return mOriginUrl;
    }

    // Return all the String fields. Note that this excludes non-string fields in particular
    // mOriginUrl and mPictureUrl.
    public String[] getStringFields() {
        return new String[] {mSubject, mEmail, mName, mGivenName};
    }
}
