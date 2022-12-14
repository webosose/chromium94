// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "chrome/test/base/mojo_web_ui_browser_test.h"
#include "chrome/test/base/test_chrome_web_ui_controller_factory.h"
#include "content/public/test/scoped_web_ui_controller_factory_registration.h"

#ifndef CHROMEOS_COMPONENTS_PERSONALIZATION_APP_TEST_PERSONALIZATION_APP_BROWSERTEST_FIXTURE_H_
#define CHROMEOS_COMPONENTS_PERSONALIZATION_APP_TEST_PERSONALIZATION_APP_BROWSERTEST_FIXTURE_H_

class TestPersonalizationAppWebUIProvider
    : public TestChromeWebUIControllerFactory::WebUIProvider {
 public:
  std::unique_ptr<content::WebUIController> NewWebUI(content::WebUI* web_ui,
                                                     const GURL& url) override;
};

class PersonalizationAppBrowserTestFixture : public MojoWebUIBrowserTest {
 public:
  PersonalizationAppBrowserTestFixture() = default;

  PersonalizationAppBrowserTestFixture(
      const PersonalizationAppBrowserTestFixture&) = delete;
  PersonalizationAppBrowserTestFixture& operator=(
      const PersonalizationAppBrowserTestFixture&) = delete;

  ~PersonalizationAppBrowserTestFixture() override = default;

  void SetUpOnMainThread() override;

 private:
  TestChromeWebUIControllerFactory test_factory_;
  TestPersonalizationAppWebUIProvider test_web_ui_provider_;
  content::ScopedWebUIControllerFactoryRegistration
      scoped_controller_factory_registration_{&test_factory_};
};

#endif  // CHROMEOS_COMPONENTS_PERSONALIZATION_APP_TEST_PERSONALIZATION_APP_BROWSERTEST_FIXTURE_H_
