// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MESSAGES_ANDROID_MESSAGE_DISPATCHER_BRIDGE_H_
#define COMPONENTS_MESSAGES_ANDROID_MESSAGE_DISPATCHER_BRIDGE_H_

#include "components/messages/android/message_enums.h"
#include "components/messages/android/message_wrapper.h"
#include "ui/android/window_android.h"

namespace content {
class WebContents;
}

namespace messages {

// C++ counterpart to MessageDispatcherBridge.java. Enables C++ feature code to
// enqueue/dismiss messages with MessageDispatcher.java.
class MessageDispatcherBridge {
 public:
  static MessageDispatcherBridge* Get();

  static void SetInstanceForTesting(MessageDispatcherBridge* instance);

  virtual bool EnqueueMessage(MessageWrapper* message,
                              content::WebContents* web_contents,
                              MessageScopeType scope_type,
                              MessagePriority priority);
  virtual bool EnqueueWindowScopedMessage(MessageWrapper* message,
                                          ui::WindowAndroid* window_android,
                                          MessagePriority priority);
  virtual void DismissMessage(MessageWrapper* message,
                              DismissReason dismiss_reason);

 protected:
  virtual ~MessageDispatcherBridge() = default;
};

}  // namespace messages

#endif  // COMPONENTS_MESSAGES_ANDROID_MESSAGE_DISPATCHER_BRIDGE_H_
