// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_string.h"
#include "base/time/time.h"
#include "chrome/browser/federated_learning/floc_id_provider_factory.h"
#include "chrome/browser/privacy_sandbox/android/jni_headers/PrivacySandboxBridge_jni.h"
#include "chrome/browser/privacy_sandbox/privacy_sandbox_settings.h"
#include "chrome/browser/privacy_sandbox/privacy_sandbox_settings_factory.h"
#include "chrome/browser/profiles/profile_manager.h"

using base::android::ConvertUTF16ToJavaString;
using base::android::ScopedJavaLocalRef;

static jboolean JNI_PrivacySandboxBridge_IsPrivacySandboxSettingsFunctional(
    JNIEnv* env) {
  return PrivacySandboxSettingsFactory::GetForProfile(
             ProfileManager::GetActiveUserProfile())
      ->PrivacySandboxSettingsFunctional();
}

static jboolean JNI_PrivacySandboxBridge_IsPrivacySandboxEnabled(JNIEnv* env) {
  return PrivacySandboxSettingsFactory::GetForProfile(
             ProfileManager::GetActiveUserProfile())
      ->IsPrivacySandboxEnabled();
}

static jboolean JNI_PrivacySandboxBridge_IsPrivacySandboxManaged(JNIEnv* env) {
  return PrivacySandboxSettingsFactory::GetForProfile(
             ProfileManager::GetActiveUserProfile())
      ->IsPrivacySandboxManaged();
}

static void JNI_PrivacySandboxBridge_SetPrivacySandboxEnabled(
    JNIEnv* env,
    jboolean enabled) {
  PrivacySandboxSettingsFactory::GetForProfile(
      ProfileManager::GetActiveUserProfile())
      ->SetPrivacySandboxEnabled(enabled);
}

static jboolean JNI_PrivacySandboxBridge_IsFlocEnabled(JNIEnv* env) {
  return PrivacySandboxSettingsFactory::GetForProfile(
             ProfileManager::GetActiveUserProfile())
      ->IsFlocPrefEnabled();
}

static void JNI_PrivacySandboxBridge_SetFlocEnabled(JNIEnv* env,
                                                    jboolean enabled) {
  PrivacySandboxSettingsFactory::GetForProfile(
      ProfileManager::GetActiveUserProfile())
      ->SetFlocPrefEnabled(enabled);
}

static jboolean JNI_PrivacySandboxBridge_IsFlocIdResettable(JNIEnv* env) {
  return PrivacySandboxSettingsFactory::GetForProfile(
             ProfileManager::GetActiveUserProfile())
      ->IsFlocIdResettable();
}

static void JNI_PrivacySandboxBridge_ResetFlocId(JNIEnv* env) {
  PrivacySandboxSettingsFactory::GetForProfile(
      ProfileManager::GetActiveUserProfile())
      ->ResetFlocId();
}

static ScopedJavaLocalRef<jstring> JNI_PrivacySandboxBridge_GetFlocStatusString(
    JNIEnv* env) {
  return ConvertUTF16ToJavaString(env,
                                  PrivacySandboxSettingsFactory::GetForProfile(
                                      ProfileManager::GetActiveUserProfile())
                                      ->GetFlocStatusForDisplay());
}

static ScopedJavaLocalRef<jstring> JNI_PrivacySandboxBridge_GetFlocGroupString(
    JNIEnv* env) {
  return ConvertUTF16ToJavaString(env,
                                  PrivacySandboxSettingsFactory::GetForProfile(
                                      ProfileManager::GetActiveUserProfile())
                                      ->GetFlocIdForDisplay());
}

static ScopedJavaLocalRef<jstring> JNI_PrivacySandboxBridge_GetFlocUpdateString(
    JNIEnv* env) {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  return ConvertUTF16ToJavaString(
      env,
      PrivacySandboxSettingsFactory::GetForProfile(profile)
          ->GetFlocIdNextUpdateForDisplay(
              federated_learning::FlocIdProviderFactory::GetForProfile(profile),
              profile->GetOriginalProfile()->GetPrefs(), base::Time::Now()));
}

static ScopedJavaLocalRef<jstring>
JNI_PrivacySandboxBridge_GetFlocDescriptionString(JNIEnv* env) {
  return ConvertUTF16ToJavaString(env,
                                  PrivacySandboxSettingsFactory::GetForProfile(
                                      ProfileManager::GetActiveUserProfile())
                                      ->GetFlocDescriptionForDisplay());
}

static ScopedJavaLocalRef<jstring>
JNI_PrivacySandboxBridge_GetFlocResetExplanationString(JNIEnv* env) {
  return ConvertUTF16ToJavaString(env,
                                  PrivacySandboxSettingsFactory::GetForProfile(
                                      ProfileManager::GetActiveUserProfile())
                                      ->GetFlocResetExplanationForDisplay());
}
