// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.merchant_viewer;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.res.Resources;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;

import org.chromium.base.FeatureList;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.JniMocker;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.tab_ui.R;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.components.embedder_support.delegate.WebContentsDelegateAndroid;
import org.chromium.components.embedder_support.util.UrlUtilities;
import org.chromium.components.embedder_support.util.UrlUtilitiesJni;
import org.chromium.components.security_state.ConnectionSecurityLevel;
import org.chromium.components.security_state.SecurityStateModel;
import org.chromium.components.security_state.SecurityStateModelJni;
import org.chromium.components.thinwebview.ThinWebView;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.NavigationController;
import org.chromium.content_public.browser.NavigationHandle;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.display.DisplayAndroid;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.url.GURL;

/**
 * Tests for {@link MerchantTrustBottomSheetMediator}.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class MerchantTrustBottomSheetMediatorTest {
    @Rule
    public TestRule mProcessor = new Features.JUnitProcessor();

    @Rule
    public JniMocker mocker = new JniMocker();

    @Mock
    private WebContents mMockWebContents;

    @Mock
    private GURL mMockDestinationGurl;

    @Mock
    UrlUtilities.Natives mUrlUtilitiesJniMock;

    @Mock
    private NavigationController mMockNavigationController;

    @Mock
    private Context mMockContext;

    @Mock
    private Resources mMockResources;

    @Mock
    private WindowAndroid mMockWindowAndroid;

    @Mock
    private DisplayAndroid mMockDisplayAndroid;

    @Mock
    private MerchantTrustMetrics mMockMetrics;

    @Mock
    private ThinWebView mMockThinWebView;

    @Mock
    private NavigationHandle mMockNavigationHandle;

    @Mock
    SecurityStateModel.Natives mSecurityStateMocks;

    @Captor
    private ArgumentCaptor<WebContentsDelegateAndroid> mWebContentsDelegateCaptor;

    @Captor
    private ArgumentCaptor<WebContentsObserver> mWebContentsObserverCaptor;

    private static final String DUMMY_SHEET_TITLE = "DUMMY_TITLE";
    private static final String DUMMY_URL = "dummy://visible/url";

    private MerchantTrustBottomSheetMediator mMediator;
    private PropertyModel mToolbarModel;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        doReturn(mMockResources).when(mMockContext).getResources();
        doReturn(56)
                .when(mMockResources)
                .getDimensionPixelSize(eq(R.dimen.toolbar_height_no_shadow));
        doReturn(mMockDisplayAndroid).when(mMockWindowAndroid).getDisplay();
        doReturn(1f).when(mMockDisplayAndroid).getDipScale();
        doReturn(DUMMY_URL).when(mMockDestinationGurl).getSpec();
        doReturn(mMockDestinationGurl).when(mMockWebContents).getVisibleUrl();
        doReturn(mMockNavigationController).when(mMockWebContents).getNavigationController();
        when(mUrlUtilitiesJniMock.isGoogleDomainUrl(anyString(), anyBoolean())).thenReturn(true);
        when(mSecurityStateMocks.getSecurityLevelForWebContents(any(WebContents.class)))
                .thenReturn(ConnectionSecurityLevel.SECURE);

        mocker.mock(UrlUtilitiesJni.TEST_HOOKS, mUrlUtilitiesJniMock);
        mocker.mock(SecurityStateModelJni.TEST_HOOKS, mSecurityStateMocks);

        mMediator = new MerchantTrustBottomSheetMediator(
                mMockContext, mMockWindowAndroid, mMockMetrics);
        mMediator.setWebContentsForTesting(mMockWebContents);
        mToolbarModel = new PropertyModel.Builder(BottomSheetToolbarProperties.ALL_KEYS).build();
        setUpSheetWebContentsAndVerify();
    }

    @After
    public void tearDown() {
        mMediator.setWebContentsForTesting(null);
    }

    private void setUpSheetWebContentsAndVerify() {
        mMediator.setupSheetWebContents(mMockThinWebView, mToolbarModel);
        verify(mMockWebContents, times(1)).addObserver(mWebContentsObserverCaptor.capture());
        verify(mMockThinWebView, times(1))
                .attachWebContents(
                        eq(mMockWebContents), eq(null), mWebContentsDelegateCaptor.capture());
    }

    @Test
    public void testNavigateToUrl() {
        mMediator.navigateToUrl(mMockDestinationGurl, DUMMY_SHEET_TITLE);
        verify(mMockNavigationController, times(1)).loadUrl(any(LoadUrlParams.class));
        assertEquals(DUMMY_SHEET_TITLE, mToolbarModel.get(BottomSheetToolbarProperties.TITLE));
    }

    @Test(expected = java.lang.AssertionError.class)
    public void testNavigateToNonGoogleUrl() {
        doReturn(false).when(mUrlUtilitiesJniMock).isGoogleDomainUrl(anyString(), anyBoolean());
        mMediator.navigateToUrl(mMockDestinationGurl, DUMMY_SHEET_TITLE);
    }

    @Test
    public void testDestroyWebContents() {
        mMediator.destroyWebContents();
        verify(mMockWebContents, times(1)).destroy();
    }

    @Test
    public void testWebContentsDelegateSslChanges() {
        mWebContentsDelegateCaptor.getValue().visibleSSLStateChanged();
        assertEquals(mMockDestinationGurl, mToolbarModel.get(BottomSheetToolbarProperties.URL));
        assertEquals(R.drawable.omnibox_https_valid,
                mToolbarModel.get(BottomSheetToolbarProperties.SECURITY_ICON));
    }

    @Test
    public void testWebContentsDelegateOpenNewTab() {
        mWebContentsDelegateCaptor.getValue().openNewTab(mMockDestinationGurl, "", null, 0, true);
        verify(mMockNavigationController, times(1)).loadUrl(any(LoadUrlParams.class));
    }

    @Test
    public void testWebContentsDelegateShouldCreateWebContents() {
        mWebContentsDelegateCaptor.getValue().shouldCreateWebContents(mMockDestinationGurl);
        verify(mMockNavigationController, times(1)).loadUrl(any(LoadUrlParams.class));
    }

    @Test
    public void testWebContentsDelegateGetTopControlsHeight() {
        assertEquals(56, mWebContentsDelegateCaptor.getValue().getTopControlsHeight());
    }

    @Test
    public void testWebContentsDelegateLoadingStateChanges() {
        // Loading state.
        doReturn(true).when(mMockWebContents).isLoading();
        mWebContentsDelegateCaptor.getValue().loadingStateChanged(true);
        assertEquals(0, mToolbarModel.get(BottomSheetToolbarProperties.LOAD_PROGRESS), 0.01);
        assertEquals(true, mToolbarModel.get(BottomSheetToolbarProperties.PROGRESS_VISIBLE));
    }

    @Test
    public void testWebContentsObserverLoadProgressChanged() {
        float progress = 0.2f;
        mWebContentsObserverCaptor.getValue().loadProgressChanged(progress);
        assertEquals(progress, mToolbarModel.get(BottomSheetToolbarProperties.LOAD_PROGRESS), 0.01);
    }

    @Test
    public void testWebContentsObserverDidStartNavigation() {
        mWebContentsObserverCaptor.getValue().didStartNavigation(mMockNavigationHandle);
        verify(mMockMetrics, times(1)).recordNavigateLinkOnBottomSheet();
    }

    @Test
    public void testWebContentsObserverTitleWasSet() {
        FeatureList.TestValues testValues = new FeatureList.TestValues();
        testValues.addFieldTrialParamOverride(ChromeFeatureList.COMMERCE_MERCHANT_VIEWER,
                MerchantViewerConfig.TRUST_SIGNALS_SHEET_USE_PAGE_TITLE_PARAM, "false");
        FeatureList.setTestValues(testValues);
        mWebContentsObserverCaptor.getValue().titleWasSet(DUMMY_SHEET_TITLE);
        assertEquals(null, mToolbarModel.get(BottomSheetToolbarProperties.TITLE));

        testValues.addFieldTrialParamOverride(ChromeFeatureList.COMMERCE_MERCHANT_VIEWER,
                MerchantViewerConfig.TRUST_SIGNALS_SHEET_USE_PAGE_TITLE_PARAM, "true");
        FeatureList.setTestValues(testValues);
        mWebContentsObserverCaptor.getValue().titleWasSet(DUMMY_SHEET_TITLE);
        assertEquals(DUMMY_SHEET_TITLE, mToolbarModel.get(BottomSheetToolbarProperties.TITLE));
    }

    @Test
    public void testWebContentsObserverDidFinishNavigation() {
        doReturn(false).when(mMockNavigationHandle).isInPrimaryMainFrame();
        doReturn(false).when(mMockNavigationHandle).hasCommitted();
        mWebContentsObserverCaptor.getValue().didFinishNavigation(mMockNavigationHandle);
        assertEquals(null, mToolbarModel.get(BottomSheetToolbarProperties.URL));

        doReturn(true).when(mMockNavigationHandle).isInPrimaryMainFrame();
        doReturn(false).when(mMockNavigationHandle).hasCommitted();
        mWebContentsObserverCaptor.getValue().didFinishNavigation(mMockNavigationHandle);
        assertEquals(null, mToolbarModel.get(BottomSheetToolbarProperties.URL));

        doReturn(false).when(mMockNavigationHandle).isInPrimaryMainFrame();
        doReturn(true).when(mMockNavigationHandle).hasCommitted();
        mWebContentsObserverCaptor.getValue().didFinishNavigation(mMockNavigationHandle);
        assertEquals(null, mToolbarModel.get(BottomSheetToolbarProperties.URL));

        doReturn(true).when(mMockNavigationHandle).isInPrimaryMainFrame();
        doReturn(true).when(mMockNavigationHandle).hasCommitted();
        mWebContentsObserverCaptor.getValue().didFinishNavigation(mMockNavigationHandle);
        assertEquals(mMockDestinationGurl, mToolbarModel.get(BottomSheetToolbarProperties.URL));
    }
}
