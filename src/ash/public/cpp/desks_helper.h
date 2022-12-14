// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_DESKS_HELPER_H_
#define ASH_PUBLIC_CPP_DESKS_HELPER_H_

#include <string>

#include "ash/public/cpp/ash_public_export.h"
#include "base/callback.h"
#include "base/macros.h"

namespace aura {
class Window;
}  // namespace aura

namespace ash {

class DeskTemplate;

// Interface for an ash client (e.g. Chrome) to interact with the Virtual Desks
// feature.
class ASH_PUBLIC_EXPORT DesksHelper {
 public:
  static DesksHelper* Get();

  // Returns true if |window| exists on the currently active desk.
  virtual bool BelongsToActiveDesk(aura::Window* window) = 0;

  // Returns the active desk's index.
  virtual int GetActiveDeskIndex() const = 0;

  // Returns the names of the desk at |index|. If |index| is out-of-bounds,
  // return empty string.
  virtual std::u16string GetDeskName(int index) const = 0;

  // Returns the number of desks.
  virtual int GetNumberOfDesks() const = 0;

  // Sends |window| to desk at |desk_index|. Does nothing if the desk at
  // |desk_index| is the active desk. |desk_index| must be valid.
  virtual void SendToDeskAtIndex(aura::Window* window, int desk_index) = 0;

  // Captures the active desk and returns it as a desk template containing
  // necessary information that can be used to create a same desk.
  virtual std::unique_ptr<DeskTemplate> CaptureActiveDeskAsTemplate() const = 0;

  // Creates and activates a new desk for a template with name `template_name`
  // or `template_name ({counter})` to resolve naming conflicts. Runs `callback`
  // with true if creation was successful, false otherwise.
  virtual void CreateAndActivateNewDeskForTemplate(
      const std::u16string& template_name,
      base::OnceCallback<void(bool)> callback) = 0;

  // Called when an app with `app_id` is a single instance app which is about to
  // get launched from a saved template. Moves the existing app instance to the
  // active desk without animation if it exists. Returns true if we should
  // launch the app (i.e. the app was not found and thus should be launched),
  // and false otherwise.
  virtual bool OnSingleInstanceAppLaunchingFromTemplate(
      const std::string& app_id) = 0;

 protected:
  DesksHelper();
  virtual ~DesksHelper();

 private:
  DISALLOW_COPY_AND_ASSIGN(DesksHelper);
};

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_DESKS_HELPER_H_
