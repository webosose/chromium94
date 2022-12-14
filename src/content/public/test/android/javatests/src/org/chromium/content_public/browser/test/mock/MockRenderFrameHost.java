// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser.test.mock;

import org.chromium.base.Callback;
import org.chromium.blink.mojom.AuthenticatorStatus;
import org.chromium.content_public.browser.GlobalRenderFrameHostId;
import org.chromium.content_public.browser.LifecycleState;
import org.chromium.content_public.browser.PermissionsPolicyFeature;
import org.chromium.content_public.browser.RenderFrameHost;
import org.chromium.mojo.bindings.Interface;
import org.chromium.url.GURL;
import org.chromium.url.Origin;

/**
 * Mock class for {@link RenderFrameHost}.
 */
public class MockRenderFrameHost implements RenderFrameHost {
    @Override
    public GURL getLastCommittedURL() {
        return null;
    }

    @Override
    public Origin getLastCommittedOrigin() {
        return null;
    }

    @Override
    public void getCanonicalUrlForSharing(Callback<GURL> callback) {}

    @Override
    public boolean isFeatureEnabled(@PermissionsPolicyFeature int feature) {
        return false;
    }

    @Override
    public <I extends Interface, P extends Interface.Proxy> P getInterfaceToRendererFrame(
            Interface.Manager<I, P> manager) {
        return null;
    }

    @Override
    public void terminateRendererDueToBadMessage(int reason) {}

    @Override
    public void notifyUserActivation() {}

    @Override
    public boolean isIncognito() {
        return false;
    }

    @Override
    public boolean signalModalCloseWatcherIfActive() {
        return false;
    }

    @Override
    public boolean isRenderFrameCreated() {
        return false;
    }

    @Override
    public boolean areInputEventsIgnored() {
        return false;
    }

    @Override
    public WebAuthSecurityChecksResults performGetAssertionWebAuthSecurityChecks(
            String relyingPartyId, Origin effectiveOrigin) {
        return new WebAuthSecurityChecksResults(AuthenticatorStatus.SUCCESS, false);
    }

    @Override
    public int performMakeCredentialWebAuthSecurityChecks(
            String relyingPartyId, Origin effectiveOrigin, boolean isPaymentCredentialCreation) {
        return 0;
    }

    @Override
    public GlobalRenderFrameHostId getGlobalRenderFrameHostId() {
        return new GlobalRenderFrameHostId(-1, -1);
    }

    @Override
    public int getLifecycleState() {
        return LifecycleState.ACTIVE;
    }
}
