// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'chrome://resources/mojo/mojo/public/js/mojo_bindings_lite.js';
import 'chrome://resources/mojo/url/mojom/url.mojom-lite.js';

import {ChromeEvent} from '/tools/typescript/definitions/chrome_event.js';
import {ClickModifiers} from 'chrome://resources/mojo/ui/base/mojom/window_open_disposition.mojom-webui.js';

import {BookmarksPageHandlerFactory, BookmarksPageHandlerRemote} from './bookmarks.mojom-webui.js';

let instance: BookmarksApiProxy|null = null;

export class BookmarksApiProxy {
  callbackRouter: {[key: string]: ChromeEvent<Function>};
  handler: BookmarksPageHandlerRemote;

  constructor() {
    this.callbackRouter = {
      onChanged: chrome.bookmarks.onChanged,
      onChildrenReordered: chrome.bookmarks.onChildrenReordered,
      onCreated: chrome.bookmarks.onCreated,
      onMoved: chrome.bookmarks.onMoved,
      onRemoved: chrome.bookmarks.onRemoved,
    };

    this.handler = new BookmarksPageHandlerRemote();

    const factory = BookmarksPageHandlerFactory.getRemote();
    factory.createBookmarksPageHandler(
        this.handler.$.bindNewPipeAndPassReceiver());
  }

  getFolders(): Promise<chrome.bookmarks.BookmarkTreeNode[]> {
    return new Promise(resolve => chrome.bookmarks.getTree(results => {
      if (results[0] && results[0].children) {
        resolve(results[0].children);
        return;
      }
      resolve([]);
    }));
  }

  openBookmark(url: string, depth: number, clickModifiers: ClickModifiers) {
    this.handler.openBookmark({url}, depth, clickModifiers);
  }

  showContextMenu(id: string, x: number, y: number) {
    this.handler.showContextMenu(id, {x, y});
  }

  static getInstance() {
    return instance || (instance = new BookmarksApiProxy());
  }

  static setInstance(obj: BookmarksApiProxy) {
    instance = obj;
  }
}