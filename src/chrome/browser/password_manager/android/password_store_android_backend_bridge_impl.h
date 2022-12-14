// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PASSWORD_MANAGER_ANDROID_PASSWORD_STORE_ANDROID_BACKEND_BRIDGE_IMPL_H_
#define CHROME_BROWSER_PASSWORD_MANAGER_ANDROID_PASSWORD_STORE_ANDROID_BACKEND_BRIDGE_IMPL_H_

#include "base/android/scoped_java_ref.h"
#include "base/compiler_specific.h"
#include "chrome/browser/password_manager/android/password_store_android_backend_bridge.h"
#include "components/password_manager/core/browser/password_store_backend.h"

// Native side of the JNI bridge to forward password store requests to Java.
// JNI code is expensive to test. Therefore, any logic beyond data conversion
// should either live in `PasswordStoreAndroidBackend` or a component that is
// used by the java-side of this bridge.
class PasswordStoreAndroidBackendBridgeImpl
    : public password_manager::PasswordStoreAndroidBackendBridge {
 public:
  PasswordStoreAndroidBackendBridgeImpl();
  PasswordStoreAndroidBackendBridgeImpl(
      PasswordStoreAndroidBackendBridgeImpl&&) = delete;
  PasswordStoreAndroidBackendBridgeImpl(
      const PasswordStoreAndroidBackendBridgeImpl&) = delete;
  PasswordStoreAndroidBackendBridgeImpl& operator=(
      PasswordStoreAndroidBackendBridgeImpl&&) = delete;
  PasswordStoreAndroidBackendBridgeImpl& operator=(
      const PasswordStoreAndroidBackendBridgeImpl&) = delete;
  ~PasswordStoreAndroidBackendBridgeImpl() override;

  // Called via JNI. Called when the api call with `task_id` finished and
  // provides the resulting `passwords`.
  void OnCompleteWithLogins(
      JNIEnv* env,
      jint task_id,
      const base::android::JavaParamRef<jobject>& passwords);

 private:
  // Implements PasswordStoreAndroidBackendBridge interface.
  void SetConsumer(Consumer* consumer) override;
  TaskId GetAllLogins() override WARN_UNUSED_RESULT;

  TaskId GetNextTaskId() WARN_UNUSED_RESULT;

  // This member stores the unique ID last used for an API request.
  TaskId last_task_id_{0};

  // Weak reference to the `Consumer` that is notified when a task completes. It
  // is required to outlive this bridge.
  Consumer* consumer_ = nullptr;

  // This object is an instance of PasswordStoreAndroidBackendBridgeImpl, i.e.
  // the Java counterpart to this class.
  base::android::ScopedJavaGlobalRef<jobject> java_object_;
};

#endif  // CHROME_BROWSER_PASSWORD_MANAGER_ANDROID_PASSWORD_STORE_ANDROID_BACKEND_BRIDGE_IMPL_H_
