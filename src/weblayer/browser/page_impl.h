// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBLAYER_BROWSER_PAGE_IMPL_H_
#define WEBLAYER_BROWSER_PAGE_IMPL_H_

#include "build/build_config.h"
#include "content/public/browser/page_user_data.h"
#include "weblayer/public/page.h"

#if defined(OS_ANDROID)
#include "base/android/scoped_java_ref.h"
#endif

namespace weblayer {

class PageImpl : public Page, public content::PageUserData<PageImpl> {
 public:
  ~PageImpl() override;

#if defined(OS_ANDROID)
  void SetJavaPage(JNIEnv* env,
                   const base::android::JavaParamRef<jobject>& java_page);

  base::android::ScopedJavaGlobalRef<jobject> java_page() { return java_page_; }
#endif

 private:
  explicit PageImpl(content::Page& page);
  friend class content::PageUserData<PageImpl>;
  PAGE_USER_DATA_KEY_DECL();

#if defined(OS_ANDROID)
  base::android::ScopedJavaGlobalRef<jobject> java_page_;
#endif

  DISALLOW_COPY_AND_ASSIGN(PageImpl);
};

}  // namespace weblayer

#endif  // WEBLAYER_BROWSER_PAGE_IMPL_H_
