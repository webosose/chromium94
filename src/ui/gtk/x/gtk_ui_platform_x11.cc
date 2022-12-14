// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gtk/x/gtk_ui_platform_x11.h"

#include "base/check.h"
#include "base/environment.h"
#include "base/strings/stringprintf.h"
#include "ui/base/x/x11_util.h"
#include "ui/events/event_constants.h"
#include "ui/events/platform/x11/x11_event_source.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/x/x11_atom_cache.h"
#include "ui/gfx/x/xlib_support.h"
#include "ui/gfx/x/xproto.h"
#include "ui/gfx/x/xproto_util.h"
#include "ui/gtk/gtk_compat.h"
#include "ui/gtk/x/gtk_event_loop_x11.h"
#include "ui/platform_window/x11/x11_window.h"
#include "ui/platform_window/x11/x11_window_manager.h"

namespace gtk {

GtkUiPlatformX11::GtkUiPlatformX11() : connection_(x11::Connection::Get()) {
  gdk_set_allowed_backends("x11");
  // GDK_BACKEND takes precedence over gdk_set_allowed_backends(), so override
  // it to ensure we get the x11 backend.
  base::Environment::Create()->SetVar("GDK_BACKEND", "x11");
  x11::InitXlib();
}

GtkUiPlatformX11::~GtkUiPlatformX11() = default;

void GtkUiPlatformX11::OnInitialized(GtkWidget* widget) {
  // Ensure the singleton instance of GtkEventLoopX11 is created and started.
  if (!event_loop_)
    event_loop_ = std::make_unique<GtkEventLoopX11>(widget);

  // GTK sets an Xlib error handler that exits the process on any async errors.
  // We don't want this behavior, so reset the error handler to something that
  // just logs the error.
  x11::SetXlibErrorHandler();
}

GdkKeymap* GtkUiPlatformX11::GetGdkKeymap() {
  DCHECK(!gtk::GtkCheckVersion(4));
  return gdk_keymap_get_for_display(GetGdkDisplay());
}

GdkWindow* GtkUiPlatformX11::GetGdkWindow(gfx::AcceleratedWidget window_id) {
  DCHECK(!gtk::GtkCheckVersion(4));
  GdkDisplay* display = GetGdkDisplay();
  GdkWindow* gdk_window = gdk_x11_window_lookup_for_display(
      display, static_cast<uint32_t>(window_id));
  if (gdk_window)
    g_object_ref(gdk_window);
  else
    gdk_window = gdk_x11_window_foreign_new_for_display(
        display, static_cast<uint32_t>(window_id));
  return gdk_window;
}

bool GtkUiPlatformX11::ExportWindowHandle(
    gfx::AcceleratedWidget window_id,
    base::OnceCallback<void(std::string)> callback) {
  std::move(callback).Run(base::StringPrintf("x11:%#x", window_id));
  return true;
}

bool GtkUiPlatformX11::SetGtkWidgetTransientFor(GtkWidget* widget,
                                                gfx::AcceleratedWidget parent) {
  auto x11_window = static_cast<x11::Window>(
      gtk::GtkCheckVersion(4)
          ? gdk_x11_surface_get_xid(
                gtk_native_get_surface(gtk_widget_get_native(widget)))
          : gdk_x11_window_get_xid(gtk_widget_get_window(widget)));
  SetProperty(x11_window, x11::Atom::WM_TRANSIENT_FOR, x11::Atom::WINDOW,
              parent);
  SetProperty(x11_window, x11::GetAtom("_NET_WM_WINDOW_TYPE"), x11::Atom::ATOM,
              x11::GetAtom("_NET_WM_WINDOW_TYPE_DIALOG"));

  ui::X11Window* parent_window =
      ui::X11WindowManager::GetInstance()->GetWindow(parent);
  parent_window->SetTransientWindow(x11_window);

  return true;
}

void GtkUiPlatformX11::ClearTransientFor(gfx::AcceleratedWidget parent) {
  ui::X11Window* parent_window =
      ui::X11WindowManager::GetInstance()->GetWindow(parent);
  // parent_window might be dead if there was a top-down window close
  if (parent_window)
    parent_window->SetTransientWindow(x11::Window::None);
}

GdkDisplay* GtkUiPlatformX11::GetGdkDisplay() {
  if (!display_)
    display_ = gdk_display_get_default();
  return display_;
}

void GtkUiPlatformX11::ShowGtkWindow(GtkWindow* window) {
  // We need to call gtk_window_present after making the widgets visible to make
  // sure window gets correctly raised and gets focus.
  DCHECK(ui::X11EventSource::HasInstance());
  gtk_window_present_with_time(
      window,
      static_cast<uint32_t>(ui::X11EventSource::GetInstance()->GetTimestamp()));
}

int GtkUiPlatformX11::GetGdkKeyState() {
  auto* xevent =
      ui::X11EventSource::GetInstance()->connection()->dispatching_event();

  if (!xevent)
    return ui::EF_NONE;

  auto* key_xevent = xevent->As<x11::KeyEvent>();
  DCHECK(key_xevent);
  return static_cast<int>(key_xevent->state);
}

}  // namespace gtk
