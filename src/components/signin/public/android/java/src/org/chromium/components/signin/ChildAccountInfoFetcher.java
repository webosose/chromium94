// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.signin;

import android.accounts.Account;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.components.signin.ChildAccountStatus.Status;
import org.chromium.components.signin.base.CoreAccountId;
import org.chromium.components.signin.base.CoreAccountInfo;

/**
 * ChildAccountInfoFetcher for the Android platform.
 * Checks whether an account is a child account from the AccountManager.
 */
final class ChildAccountInfoFetcher {
    private static final String TAG = "signin";

    private static final String ACCOUNT_SERVICES_CHANGED_FILTER =
            "com.google.android.gms.auth.ACCOUNT_SERVICES_CHANGED";

    private static final String ACCOUNT_CHANGE_PERMISSION =
            "com.google.android.gms.auth.permission.GOOGLE_ACCOUNT_CHANGE";

    private static final String ACCOUNT_KEY = "account";

    private final long mNativeAccountFetcherService;
    private final CoreAccountInfo mCoreAccountInfo;
    private final BroadcastReceiver mAccountFlagsChangedReceiver;

    private ChildAccountInfoFetcher(
            long nativeAccountFetcherService, CoreAccountInfo coreAccountInfo) {
        mNativeAccountFetcherService = nativeAccountFetcherService;
        mCoreAccountInfo = coreAccountInfo;

        // Register for notifications about flag changes in the future.
        mAccountFlagsChangedReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                ThreadUtils.assertOnUiThread();
                Account changedAccount = intent.getParcelableExtra(ACCOUNT_KEY);
                Log.d(TAG, "Received account flag change broadcast for %s", changedAccount.name);

                if (mCoreAccountInfo.getEmail().equals(changedAccount.name)) {
                    fetch();
                }
            }
        };
        ContextUtils.getApplicationContext().registerReceiver(mAccountFlagsChangedReceiver,
                new IntentFilter(ACCOUNT_SERVICES_CHANGED_FILTER), ACCOUNT_CHANGE_PERMISSION, null);

        // Fetch once now to update the status in case it changed before we registered for updates.
        fetch();
    }

    @CalledByNative
    private static ChildAccountInfoFetcher create(
            long nativeAccountFetcherService, CoreAccountInfo coreAccountInfo) {
        return new ChildAccountInfoFetcher(nativeAccountFetcherService, coreAccountInfo);
    }

    private void fetch() {
        Log.d(TAG, "Checking child account status for %s", mCoreAccountInfo.getEmail());
        AccountManagerFacadeProvider.getInstance().checkChildAccountStatus(
                CoreAccountInfo.getAndroidAccountFrom(mCoreAccountInfo),
                this::onChildAccountStatusReady);
    }

    private void onChildAccountStatusReady(@Status int status) {
        final boolean isChild = ChildAccountStatus.isChild(status);
        Log.d(TAG, "Setting child account status for %s to %s", mCoreAccountInfo.getEmail(),
                isChild);
        ChildAccountInfoFetcherJni.get().setIsChildAccount(
                mNativeAccountFetcherService, mCoreAccountInfo.getId(), isChild);
    }

    @CalledByNative
    private void destroy() {
        ContextUtils.getApplicationContext().unregisterReceiver(mAccountFlagsChangedReceiver);
    }

    @NativeMethods
    interface Natives {
        void setIsChildAccount(
                long accountFetcherServicePtr, CoreAccountId accountId, boolean isChildAccount);
    }
}
