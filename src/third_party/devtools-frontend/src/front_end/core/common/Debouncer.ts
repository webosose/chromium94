// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Debounce utility function, ensures that the function passed in is only called once the function stops being called and the delay has expired.
 */
export const debounce = function(func: Function, delay: number): Function {
  let timer = 0;
  const debounced = (): void => {
    clearTimeout(timer);
    timer = setTimeout(() => func(), delay);
  };
  return debounced;
};
