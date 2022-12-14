// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_NEW_WINDOW_DELEGATE_H_
#define ASH_PUBLIC_CPP_NEW_WINDOW_DELEGATE_H_

#include <string>

#include "ash/public/cpp/ash_public_export.h"

class GURL;

namespace ash {

// A delegate interface that an ash user sends to ash to handle certain window
// management responsibilities.
class ASH_PUBLIC_EXPORT NewWindowDelegate {
 public:
  // Sources of feedback requests.
  enum FeedbackSource {
    kFeedbackSourceAsh,
    kFeedbackSourceAssistant,
    kFeedbackSourceQuickAnswers,
  };

  virtual ~NewWindowDelegate();

  // Returns an instance connected to ash-chrome.
  static NewWindowDelegate* GetInstance();

  // Returns an instance connected to the primary browser.
  // Specifically, if Lacros is the primary browser, the instance connected
  // to the registered browser via crosapi.
  static NewWindowDelegate* GetPrimary();

  // Invoked when the user uses Ctrl+T to open a new tab.
  virtual void NewTab() = 0;

  // Opens a new tab with the specified URL. If the |from_user_interaction|
  // is true then the page will load with a user activation. This means the
  // page will be able to autoplay media without restriction.
  virtual void NewTabWithUrl(const GURL& url, bool from_user_interaction) = 0;

  // Invoked when the user uses Ctrl-N or Ctrl-Shift-N to open a new window. If
  // the |should_trigger_session_restore| is true, a new window opening should
  // be treated like the start of a session (with potential session restore,
  // startup URLs, etc.). Otherwise, don't restore the session.
  virtual void NewWindow(bool incognito,
                         bool should_trigger_session_restore) = 0;

  // Invoked when an accelerator (calculator key) is used to open calculator.
  virtual void OpenCalculator() = 0;

  // Invoked when an accelerator is used to open the file manager.
  virtual void OpenFileManager() = 0;

  // Opens File Manager in the My files/Downloads folder.
  virtual void OpenDownloadsFolder() = 0;

  // Invoked when the user opens Crosh.
  virtual void OpenCrosh() = 0;

  // Invoked when an accelerator is used to open diagnostics.
  virtual void OpenDiagnostics() = 0;

  // Invoked when an accelerator is used to open help center.
  virtual void OpenGetHelp() = 0;

  // Invoked when the user uses Shift+Ctrl+T to restore the closed tab.
  virtual void RestoreTab() = 0;

  // Show the keyboard shortcut viewer.
  virtual void ShowKeyboardShortcutViewer() = 0;

  // Shows the task manager window.
  virtual void ShowTaskManager() = 0;

  // Opens the feedback page for "Report Issue".
  // |source| indicates the source of the feedback request, which is Ash by
  // default.
  // |description_template| is the preset description when the feedback dialog
  // is opened.
  virtual void OpenFeedbackPage(
      FeedbackSource source = kFeedbackSourceAsh,
      const std::string& description_template = std::string()) = 0;

 protected:
  NewWindowDelegate();
  NewWindowDelegate(const NewWindowDelegate&) = delete;
  NewWindowDelegate& operator=(const NewWindowDelegate&) = delete;
};

// Interface to provide delegate instances for
// NewWindowDelegate::GetInstance/GetPrimary methods.
class ASH_PUBLIC_EXPORT NewWindowDelegateProvider {
 public:
  virtual ~NewWindowDelegateProvider();
  virtual NewWindowDelegate* GetInstance() = 0;
  virtual NewWindowDelegate* GetPrimary() = 0;

 protected:
  NewWindowDelegateProvider();
  NewWindowDelegateProvider(const NewWindowDelegateProvider&) = delete;
  NewWindowDelegateProvider& operator=(const NewWindowDelegateProvider&) =
      delete;
};

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_NEW_WINDOW_DELEGATE_H_
