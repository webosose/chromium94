// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ui.quickactionsearchwidget;

import android.app.Activity;
import android.appwidget.AppWidgetManager;
import android.content.ComponentName;
import android.content.Intent;
import android.support.test.InstrumentationRegistry;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.RemoteViews;

import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseActivityTestRule;
import org.chromium.base.test.util.AdvancedMockContext;
import org.chromium.base.test.util.ApplicationTestUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.quickactionsearchwidget.QuickActionSearchWidgetReceiver;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.ChromeApplicationTestUtils;
import org.chromium.chrome.test.util.browser.Features;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests for the QuickActionSearchWidgetProviderDelegate.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add(ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE)
@Features.EnableFeatures({ChromeFeatureList.QUICK_ACTION_SEARCH_WIDGET})
public class QuickActionSearchWidgetProviderDelegateTest {
    private static final class TestContext extends AdvancedMockContext {
        public TestContext() {
            super(InstrumentationRegistry.getInstrumentation()
                            .getTargetContext()
                            .getApplicationContext());
        }
    }

    private static final class TestDelegate extends QuickActionSearchWidgetProviderDelegate {
        public TestDelegate(@QuickActionSearchWidgetType int widgetType,
                ComponentName widgetReceiverComponent, Intent startIncognitoTab) {
            super(widgetType, widgetReceiverComponent, startIncognitoTab);
        }

        public final List<RemoteViews> mRemoteViews = new ArrayList<>();

        @Override
        protected void updateWidget(
                AppWidgetManager manager, int widgetId, RemoteViews remoteViews) {
            super.updateWidget(manager, widgetId, remoteViews);
            mRemoteViews.add(remoteViews);
        }
    }

    @Rule
    public BaseActivityTestRule<Activity> mActivityTestRule =
            new BaseActivityTestRule<>(Activity.class);

    // These are random unique identifiers, the value of these numbers have no special meaning.
    // The number of identifiers has no particular meaning either.
    private static final int[] WIDGET_IDS = {1, 2};

    private final List<View> mWidgetViews = new ArrayList<>();

    private TestDelegate mDelegate;
    private TestContext mContext;

    @Before
    public void setUp() {
        ChromeApplicationTestUtils.setUp(InstrumentationRegistry.getTargetContext());

        mContext = new TestContext();

        ComponentName widgetReceiverComponent =
                new ComponentName(mContext, QuickActionSearchWidgetReceiver.class);

        mDelegate = new TestDelegate(QuickActionSearchWidgetType.SMALL, widgetReceiverComponent,
                IntentHandler.createTrustedOpenNewTabIntent(mContext, /*incognito=*/true));

        setUpViews();
    }

    @After
    public void tearDown() {
        ChromeApplicationTestUtils.tearDown(InstrumentationRegistry.getTargetContext());
    }

    @Test
    @SmallTest
    public void testSearchBarClick() throws Exception {
        for (View view : mWidgetViews) {
            // clang-format off
            QuickActionSearchWidgetTestUtils.assertSearchActivityLaunchedAfterAction(
                    mActivityTestRule,
                    () -> QuickActionSearchWidgetTestUtils.clickOnView(
                            view, R.id.quick_action_search_widget_search_bar_container),
                    /*shouldActivityLaunchVoiceMode=*/false);
            // clang-format on
            ApplicationTestUtils.finishActivity(mActivityTestRule.getActivity());
        }
    }

    @Test
    @SmallTest
    public void testIncognitoTabClick() throws Exception {
        for (View view : mWidgetViews) {
            QuickActionSearchWidgetTestUtils.assertIncognitoModeLaunchedAfterAction(
                    mActivityTestRule, () -> {
                        QuickActionSearchWidgetTestUtils.clickOnView(
                                view, R.id.incognito_quick_action_button);
                    });
            ApplicationTestUtils.finishActivity(mActivityTestRule.getActivity());
        }
    }

    @Test
    @SmallTest
    public void testVoiceButtonClick() throws Exception {
        for (View view : mWidgetViews) {
            // clang-format off
            QuickActionSearchWidgetTestUtils.assertSearchActivityLaunchedAfterAction(
                    mActivityTestRule,
                    () -> QuickActionSearchWidgetTestUtils.clickOnView(
                            view, R.id.voice_search_quick_action_button),
                    /*shouldActivityLaunchVoiceMode=*/true);
            // clang-format on
            ApplicationTestUtils.finishActivity(mActivityTestRule.getActivity());
        }
    }

    @Test
    @SmallTest
    @DisabledTest(message = "https://crbug.com/1225949")
    public void testDinoButtonClick() throws Exception {
        for (View view : mWidgetViews) {
            // clang-format off
            QuickActionSearchWidgetTestUtils.assertDinoGameLaunchedAfterAction(
                    mActivityTestRule,
                    () -> QuickActionSearchWidgetTestUtils.clickOnView(
                            view, R.id.dino_quick_action_button));
            // clang-format on
            ApplicationTestUtils.finishActivity(mActivityTestRule.getActivity());
        }
    }

    private void setUpViews() {
        FrameLayout parentView = new FrameLayout(mContext);

        AppWidgetManager widgetManager = AppWidgetManager.getInstance(mContext);
        mDelegate.updateWidgets(mContext, widgetManager, WIDGET_IDS);

        for (int i = 0; i < mDelegate.mRemoteViews.size(); i++) {
            RemoteViews views = mDelegate.mRemoteViews.get(i);
            View view = views.apply(mContext, parentView);

            parentView.addView(view);
            mWidgetViews.add(view);
        }
    }


}
