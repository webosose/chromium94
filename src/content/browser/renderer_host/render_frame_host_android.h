// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_RENDER_FRAME_HOST_ANDROID_H_
#define CONTENT_BROWSER_RENDERER_HOST_RENDER_FRAME_HOST_ANDROID_H_

#include <jni.h>

#include "base/android/jni_android.h"
#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/supports_user_data.h"
#include "content/common/content_export.h"

namespace content {

class RenderFrameHostImpl;

// Android wrapper around RenderFrameHost that provides safer passage from java
// and back to native and provides java with a means of communicating with its
// native counterpart.
class RenderFrameHostAndroid : public base::SupportsUserData::Data {
 public:
  RenderFrameHostAndroid(RenderFrameHostImpl* render_frame_host);
  ~RenderFrameHostAndroid() override;

  base::android::ScopedJavaLocalRef<jobject> GetJavaObject();

  // Methods called from Java
  base::android::ScopedJavaLocalRef<jobject> GetLastCommittedURL(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>&) const;

  base::android::ScopedJavaLocalRef<jobject> GetLastCommittedOrigin(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>&);

  void GetCanonicalUrlForSharing(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>&,
      const base::android::JavaParamRef<jobject>& jcallback) const;

  bool IsFeatureEnabled(JNIEnv* env,
                        const base::android::JavaParamRef<jobject>&,
                        jint feature) const;

  // Returns UnguessableToken.
  base::android::ScopedJavaLocalRef<jobject> GetAndroidOverlayRoutingToken(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>&) const;

  void NotifyUserActivation(JNIEnv* env,
                            const base::android::JavaParamRef<jobject>&);

  jboolean SignalModalCloseWatcherIfActive(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>&) const;

  jboolean IsRenderFrameCreated(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>&) const;

  void GetInterfaceToRendererFrame(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>&,
      const base::android::JavaParamRef<jstring>& interface_name,
      jint message_pipe_handle) const;

  void TerminateRendererDueToBadMessage(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>&,
      jint reason) const;

  jboolean IsProcessBlocked(JNIEnv* env,
                            const base::android::JavaParamRef<jobject>&) const;

  base::android::ScopedJavaLocalRef<jobject>
  PerformGetAssertionWebAuthSecurityChecks(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>&,
      const base::android::JavaParamRef<jstring>&,
      const base::android::JavaParamRef<jobject>&) const;

  jint PerformMakeCredentialWebAuthSecurityChecks(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>&,
      const base::android::JavaParamRef<jstring>&,
      const base::android::JavaParamRef<jobject>&,
      jboolean is_payment_credential_creation) const;

  jint GetLifecycleState(JNIEnv* env,
                         const base::android::JavaParamRef<jobject>&) const;

  RenderFrameHostImpl* render_frame_host() const { return render_frame_host_; }

 private:
  RenderFrameHostImpl* const render_frame_host_;
  JavaObjectWeakGlobalRef obj_;

  DISALLOW_COPY_AND_ASSIGN(RenderFrameHostAndroid);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_RENDER_FRAME_HOST_ANDROID_H_
