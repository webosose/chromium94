// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_VALUE_STORE_SETTINGS_NAMESPACE_H_
#define EXTENSIONS_BROWSER_VALUE_STORE_SETTINGS_NAMESPACE_H_

#include <string>

namespace extensions {

namespace settings_namespace {

// TODO(crbug.com/1226956): Move extensions specific namespaces out of
// ValueStore.
// The namespaces of the storage areas that have ValueStore.
enum Namespace {
  LOCAL,    // "local"    i.e. chrome.storage.local
  SYNC,     // "sync"     i.e. chrome.storage.sync
  MANAGED,  // "managed"  i.e. chrome.storage.managed
  INVALID
};

// Converts a namespace to its string representation.
// Namespace must not be INVALID.
std::string ToString(Namespace settings_namespace);

}  // namespace settings_namespace

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_VALUE_STORE_SETTINGS_NAMESPACE_H_
