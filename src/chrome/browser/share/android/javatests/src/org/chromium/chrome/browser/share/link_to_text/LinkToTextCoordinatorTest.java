// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.share.link_to_text;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Activity;

import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.robolectric.Robolectric;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.JniMocker;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.share.ChromeShareExtras;
import org.chromium.chrome.browser.share.share_sheet.ChromeOptionShareCallback;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.components.browser_ui.share.ShareParams;
import org.chromium.components.dom_distiller.core.DomDistillerUrlUtils;
import org.chromium.components.dom_distiller.core.DomDistillerUrlUtilsJni;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.url.JUnitTestGURLs;

/**
 * Tests for {@link LinkToTextCoordinator}.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Features.EnableFeatures({ChromeFeatureList.PREEMPTIVE_LINK_TO_TEXT_GENERATION})
public class LinkToTextCoordinatorTest {
    @Rule
    public JniMocker jniMocker = new JniMocker();

    @Rule
    public TestRule mFeatureProcessor = new Features.JUnitProcessor();

    @Mock
    private ChromeOptionShareCallback mShareCallback;
    @Mock
    private WindowAndroid mWindow;
    @Mock
    private Tab mTab;
    @Mock
    private WebContents mWebContents;
    @Mock
    private DomDistillerUrlUtils.Natives mDistillerUrlUtilsJniMock;

    private Activity mActivity;
    private static final String SELECTED_TEXT = "selection";
    private static final String VISIBLE_URL = JUnitTestGURLs.EXAMPLE_URL;
    private static final String AMP_URL = JUnitTestGURLs.AMP_URL;
    private static final String MOBILE_URL = "https://mobile.foo.com";
    private static final String AMP_MOBILE_URL =
            "https://mobile.google.com/amp/www.nyt.com/ampthml/blogs.html";
    private static final String MOBILE_SUBDOMAIN_URL = "https://m.foo.com";
    private static final String AMP_MOBILE_SUBDOMAIN_URL =
            "https://m.google.com/amp/www.nyt.com/ampthml/blogs.html";
    private static final String SELECTED_TEXT_LONG =
            "This textbook has more freedom than most (but see some exceptions).";

    @Before
    public void setUpTest() {
        mActivity = Robolectric.setupActivity(Activity.class);
        MockitoAnnotations.initMocks(this);
        jniMocker.mock(DomDistillerUrlUtilsJni.TEST_HOOKS, mDistillerUrlUtilsJniMock);
        when(mDistillerUrlUtilsJniMock.getOriginalUrlFromDistillerUrl(any(String.class)))
                .thenReturn(JUnitTestGURLs.getGURL(VISIBLE_URL));

        doNothing().when(mShareCallback).showThirdPartyShareSheet(any(), any(), anyLong());
        doNothing().when(mShareCallback).showShareSheet(any(), any(), anyLong());
        Mockito.when(mTab.getWebContents()).thenReturn(mWebContents);
        Mockito.when(mTab.getWindowAndroid()).thenReturn(mWindow);
    }

    @Test
    @SmallTest
    public void getUrlToShareTest() {
        String selector = "selector";
        String expectedUrlToShare = VISIBLE_URL + "#:~:text=selector";
        LinkToTextCoordinator coordinator = new LinkToTextCoordinator(
                mActivity, mTab, mShareCallback, VISIBLE_URL, SELECTED_TEXT);
        Assert.assertEquals(expectedUrlToShare, coordinator.getUrlToShare(selector));
    }

    @Test
    @SmallTest
    public void getUrlToShareTest_URLWithFragment() {
        String selector = "selector";
        String expectedUrlToShare = VISIBLE_URL + "#:~:text=selector";
        LinkToTextCoordinator coordinator = new LinkToTextCoordinator(
                mActivity, mTab, mShareCallback, VISIBLE_URL + "#elementid", SELECTED_TEXT);
        Assert.assertEquals(expectedUrlToShare, coordinator.getUrlToShare(selector));
    }

    @Test
    @SmallTest
    public void getUrlToShareTest_EmptySelector() {
        String selector = "";
        String expectedUrlToShare = VISIBLE_URL;
        LinkToTextCoordinator coordinator = new LinkToTextCoordinator(
                mActivity, mTab, mShareCallback, VISIBLE_URL, SELECTED_TEXT);
        Assert.assertEquals(expectedUrlToShare, coordinator.getUrlToShare(selector));
    }

    @Test
    @SmallTest
    @Features.DisableFeatures({ChromeFeatureList.PREEMPTIVE_LINK_TO_TEXT_GENERATION})
    public void onSelectorReadyTest() {
        LinkToTextCoordinator coordinator = new LinkToTextCoordinator(
                mActivity, mTab, mShareCallback, VISIBLE_URL, SELECTED_TEXT);
        // OnSelectorReady should call back the share sheet.
        coordinator.onSelectorReady("selector");
        verify(mShareCallback).showThirdPartyShareSheet(any(), any(), anyLong());
    }

    @Test
    @SmallTest
    @Features.DisableFeatures({ChromeFeatureList.PREEMPTIVE_LINK_TO_TEXT_GENERATION})
    public void onSelectorReadyTest_EmptySelector() {
        LinkToTextCoordinator coordinator = new LinkToTextCoordinator(
                mActivity, mTab, mShareCallback, VISIBLE_URL, SELECTED_TEXT);
        // OnSelectorReady should call back the share sheet.
        coordinator.onSelectorReady("");
        verify(mShareCallback).showThirdPartyShareSheet(any(), any(), anyLong());
    }

    @Test
    @SmallTest
    @Features.EnableFeatures({ChromeFeatureList.PREEMPTIVE_LINK_TO_TEXT_GENERATION})
    public void onSelectorReadyTest_PreemptiveLinkToTextGeneration() {
        LinkToTextCoordinator coordinator = new LinkToTextCoordinator(
                mActivity, mTab, mShareCallback, VISIBLE_URL, SELECTED_TEXT);
        // OnSelectorReady should call back the share sheet.
        coordinator.onSelectorReady("selector");
        verify(mShareCallback).showShareSheet(any(), any(), anyLong());
    }

    @Test
    @SmallTest
    @Features.EnableFeatures({ChromeFeatureList.PREEMPTIVE_LINK_TO_TEXT_GENERATION})
    public void onSelectorReadyTest_EmptySelector_PreemptiveLinkToTextGeneration() {
        LinkToTextCoordinator coordinator = new LinkToTextCoordinator(
                mActivity, mTab, mShareCallback, VISIBLE_URL, SELECTED_TEXT);
        // OnSelectorReady should call back the share sheet.
        coordinator.onSelectorReady("");
        verify(mShareCallback).showShareSheet(any(), any(), anyLong());
    }

    @Test
    @SmallTest
    @Features.EnableFeatures({ChromeFeatureList.PREEMPTIVE_LINK_TO_TEXT_GENERATION})
    public void showShareSheetTest_PreemptiveLinkToTextGeneration_LinkGeneration() {
        ShareParams shareParams = new ShareParams.Builder(/*window=*/null, "", VISIBLE_URL)
                                          .setText(SELECTED_TEXT)
                                          .build();
        ChromeShareExtras chromeShareExtras = new ChromeShareExtras.Builder().build();
        LinkToTextCoordinator coordinator = new LinkToTextCoordinator(
                shareParams, mTab, mShareCallback, chromeShareExtras, 1, VISIBLE_URL);
        coordinator.onSelectorReady("selector");
        verify(mShareCallback).showShareSheet(any(), any(), anyLong());
    }

    @Test
    @SmallTest
    @Features.EnableFeatures({ChromeFeatureList.PREEMPTIVE_LINK_TO_TEXT_GENERATION})
    public void showShareSheetTest_EmptySelector_PreemptiveLinkToTextGeneration() {
        ShareParams shareParams = new ShareParams.Builder(/*window=*/null, "", VISIBLE_URL)
                                          .setText(SELECTED_TEXT)
                                          .build();
        ChromeShareExtras chromeShareExtras = new ChromeShareExtras.Builder().build();
        LinkToTextCoordinator coordinator = new LinkToTextCoordinator(
                shareParams, mTab, mShareCallback, chromeShareExtras, 1, VISIBLE_URL);
        coordinator.onSelectorReady("");
        verify(mShareCallback).showShareSheet(any(), any(), anyLong());
    }

    @Test
    @SmallTest
    public void isAmpUrlTest() {
        ShareParams shareParams = new ShareParams.Builder(/*window=*/null, "", AMP_URL)
                                          .setText(SELECTED_TEXT)
                                          .build();
        ChromeShareExtras chromeShareExtras = new ChromeShareExtras.Builder().build();
        LinkToTextCoordinator coordinator = new LinkToTextCoordinator(
                shareParams, mTab, mShareCallback, chromeShareExtras, 1, AMP_URL);

        Assert.assertEquals(true, coordinator.isAmpUrl(AMP_URL));
        Assert.assertEquals(false, coordinator.isAmpUrl(VISIBLE_URL));

        Assert.assertEquals(true, coordinator.isAmpUrl(AMP_MOBILE_URL));
        Assert.assertEquals(false, coordinator.isAmpUrl(MOBILE_URL));

        Assert.assertEquals(true, coordinator.isAmpUrl(AMP_MOBILE_SUBDOMAIN_URL));
        Assert.assertEquals(false, coordinator.isAmpUrl(MOBILE_SUBDOMAIN_URL));
    }

    @Test
    @SmallTest
    public void getPreviewTextLongTest() {
        ShareParams shareParams = new ShareParams.Builder(/*window=*/null, "", AMP_URL)
                                          .setText(SELECTED_TEXT_LONG)
                                          .build();
        ChromeShareExtras chromeShareExtras = new ChromeShareExtras.Builder().build();
        LinkToTextCoordinator coordinator = new LinkToTextCoordinator(
                shareParams, mTab, mShareCallback, chromeShareExtras, 1, VISIBLE_URL);
        Assert.assertEquals("This textbook has more freedom t...", coordinator.getPreviewText());
    }

    @Test
    @SmallTest
    public void getPreviewTextTest() {
        ShareParams shareParams = new ShareParams.Builder(/*window=*/null, "", AMP_URL)
                                          .setText(SELECTED_TEXT)
                                          .build();
        ChromeShareExtras chromeShareExtras = new ChromeShareExtras.Builder().build();
        LinkToTextCoordinator coordinator = new LinkToTextCoordinator(
                shareParams, mTab, mShareCallback, chromeShareExtras, 1, VISIBLE_URL);
        Assert.assertEquals("selection", coordinator.getPreviewText());
    }
}
