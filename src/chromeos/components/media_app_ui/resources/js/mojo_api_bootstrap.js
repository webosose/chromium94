// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import './media_app_ui.mojom-lite.js';

export const mediaAppPageHandler = new mediaAppUi.mojom.PageHandlerRemote();

// Set up a page handler to talk to the browser process.
mediaAppUi.mojom.PageHandlerFactory.getRemote().createPageHandler(
    mediaAppPageHandler.$.bindNewPipeAndPassReceiver());
