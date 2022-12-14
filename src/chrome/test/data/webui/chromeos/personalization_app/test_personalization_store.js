// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview a mock store for personalization app. Use to monitor actions
 * and manipulate state.
 *
 * Skip type-checks for this file because ../../test_store.js is not properly
 * typed.
 * @suppress {checkTypes}
 */

import {emptyState, reduce} from 'chrome://personalization/trusted/personalization_reducers.js';
import {PersonalizationStore} from 'chrome://personalization/trusted/personalization_store.js';
import {TestStore} from '../../test_store.js';

export class TestPersonalizationStore extends TestStore {
  constructor(data) {
    super(data, PersonalizationStore, emptyState(), reduce);
  }

  /** @override */
  replaceSingleton() {
    PersonalizationStore.setInstance(this);
  }
}
