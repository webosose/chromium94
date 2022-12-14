// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GTK_GTK_UI_PLATFORM_H_
#define UI_GTK_GTK_UI_PLATFORM_H_

#include "base/callback_forward.h"
#include "ui/gfx/native_widget_types.h"

#include <string>

using GdkKeymap = struct _GdkKeymap;
using GtkWindow = struct _GtkWindow;
using GtkWidget = struct _GtkWidget;
using GdkWindow = struct _GdkWindow;

namespace gtk {

// GtkUiPlatform encapsulates platform-specific functionalities required by
// a Gtk-based LinuxUI implementation.
class GtkUiPlatform {
 public:
  virtual ~GtkUiPlatform() = default;

  // Called when the GtkUi instance initialization process finished. |widget| is
  // a dummy window passed in for context.
  virtual void OnInitialized(GtkWidget* widget) = 0;

  // Gets the GdkKeymap instance, which is used to translate KeyEvents into
  // GdkEvents before filtering them through GtkIM API.
  virtual GdkKeymap* GetGdkKeymap() = 0;

  // Creates/Gets a GdkWindow out of a Aura window id. Caller owns the returned
  // object. This function is meant to be used in GtkIM-based IME implementation
  // and is supported only in X11 backend (both Aura and Ozone).
  virtual GdkWindow* GetGdkWindow(gfx::AcceleratedWidget window_id) = 0;

  // Exports a prefixed, platform-dependent (X11 or Wayland) window handle for
  // an Aura window id, then calls the given callback with the handle.
  virtual bool ExportWindowHandle(
      gfx::AcceleratedWidget window_id,
      base::OnceCallback<void(std::string)> callback) = 0;

  // Gtk dialog windows must be set transient for the browser window. This
  // function abstracts away such functionality.
  virtual bool SetGtkWidgetTransientFor(GtkWidget* widget,
                                        gfx::AcceleratedWidget parent) = 0;
  virtual void ClearTransientFor(gfx::AcceleratedWidget parent) = 0;

  // Presents |window|, doing all the necessary platform-specific operations
  // needed, if any.
  virtual void ShowGtkWindow(GtkWindow* window) = 0;

  // Get the current keyboard modifier state.
  virtual int GetGdkKeyState() = 0;
};

}  // namespace gtk

#endif  // UI_GTK_GTK_UI_PLATFORM_H_
