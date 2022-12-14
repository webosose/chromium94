// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'chrome://resources/cr_elements/hidden_style_css.m.js';
import './whats_new_error_page.js';
import './strings.m.js';

import {EventTracker} from 'chrome://resources/js/event_tracker.m.js';
import {loadTimeData} from 'chrome://resources/js/load_time_data.m.js';
import {html, microTask, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {ClickInfo, Command} from './promo_browser_command.mojom-webui.js';
import {WhatsNewCommandProxy} from './whats_new_command_proxy.js';
import {WhatsNewProxyImpl} from './whats_new_proxy.js';

type CommandData = {
  commandId: number,
  clickInfo: ClickInfo,
};

// TODO (https://www.crbug.com/1219381): Add some additional parameters so
// that we can filter the messages a bit better.
type BrowserCommandMessageData = {
  data: CommandData,
};

export class WhatsNewAppElement extends PolymerElement {
  static get is() {
    return 'whats-new-app';
  }

  static get properties() {
    return {
      showErrorPage_: Boolean,
      url_: String,
    };
  }

  private showErrorPage_: boolean = false;
  private url_: string = '';
  private eventTracker_: EventTracker = new EventTracker();

  connectedCallback() {
    super.connectedCallback();

    const queryParams = new URLSearchParams(window.location.search);
    const isAutoOpen = queryParams.has('auto');
    WhatsNewProxyImpl.getInstance().initialize(isAutoOpen).then(url => {
      if (!url) {
        this.showErrorPage_ = true;
        return;
      }

      this.url_ = isAutoOpen ? url.concat('?latest=true') : url;
      this.eventTracker_.add(
          window, 'message',
          event => this.handleMessage_(event as MessageEvent));
    });
  }

  disconnectedCallback() {
    super.disconnectedCallback();
    this.eventTracker_.removeAll();
  }

  private handleMessage_(event: MessageEvent) {
    const {data, origin} = event;
    const iframeUrl = new URL(this.url_);
    if (!data || origin !== iframeUrl.origin) {
      return;
    }

    const commandData = (data as BrowserCommandMessageData).data;

    const commandId = Object.values(Command).includes(commandData.commandId) ?
        commandData.commandId :
        Command.kUnknownCommand;

    const handler = WhatsNewCommandProxy.getInstance().handler;
    handler.canExecuteCommand(commandId).then(({canExecute}) => {
      if (canExecute) {
        handler.executeCommand(commandId, commandData.clickInfo);
      } else {
        console.warn('Received invalid command: ' + commandId);
      }
    });
  }

  static get template() {
    return html`{__html_template__}`;
  }
}
customElements.define(WhatsNewAppElement.is, WhatsNewAppElement);
