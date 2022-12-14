// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {$} from 'chrome://resources/js/util.m.js';

import {AudioSample, OutputPage} from './output_page.js';
import {PageNavigator} from './page.js';

export class AudioPlayer extends HTMLElement {
  private sampleIdx: number;
  private audioDiv: HTMLDivElement;
  private audioPlay: HTMLButtonElement;
  private audioCtx: AudioContext|undefined;
  private audioQuery: HTMLDivElement;
  private audioNameTag: HTMLParagraphElement;
  private audioExpectation: HTMLParagraphElement;
  private prevLink: HTMLButtonElement;
  private timer: number|undefined;
  constructor(private audioSamples: AudioSample[]) {
    super();
    this.sampleIdx = 0;
    const clone = (<HTMLTemplateElement>$('audioPlayer-template'))
                      .content.cloneNode(true);
    this.audioDiv = <HTMLDivElement>(<HTMLElement>clone).querySelector('div');
    this.audioPlay =
        <HTMLButtonElement>(this.audioDiv.querySelector('#play-btn'));
    this.audioQuery =
        <HTMLDivElement>(this.audioDiv.querySelector('#output-qs'));
    this.audioNameTag =
        <HTMLParagraphElement>(this.audioDiv.querySelectorAll('p')[0]);
    this.audioExpectation =
        <HTMLParagraphElement>(this.audioDiv.querySelectorAll('p')[1]);
    this.prevLink = <HTMLButtonElement>(this.audioDiv.querySelector('#back'));
    this.prevLink.textContent = '< Back';
    this.prevLink.addEventListener('click', () => {
      this.handleBackClick();
    });

    this.setUpAudioPlayer();
    this.setButtons();
    this.appendChild(this.audioDiv);
  }

  private get current() {
    return this.audioSamples[this.sampleIdx];
  }

  setUpAudioPlayer() {
    this.audioNameTag.innerHTML = `Playing: ${this.current!.description}`;
    this.setAudioExpectation();
  }

  setAudioExpectation() {
    if (this.current!.pan == -1) {
      this.audioExpectation.innerHTML =
          'Should hear audio coming from the left channel.';
    } else if (this.current!.pan == 1) {
      this.audioExpectation.innerHTML =
          'Should hear audio coming from the right channel.';
    } else {
      this.audioExpectation.innerHTML =
          'Should hear audio of a single pitch from all channels.';
    }
  }

  setButtons() {
    const yesLink =
        <HTMLButtonElement>(this.audioQuery.querySelector('#output-yes'));
    const noLink =
        <HTMLButtonElement>(this.audioQuery.querySelector('#output-no'));

    yesLink.addEventListener('click', () => this.handleResponse(true));
    noLink.addEventListener('click', () => this.handleResponse(false));

    this.audioPlay.addEventListener('click', () => {
      if (this.audioCtx?.state === 'running') {
        this.audioCtx.suspend();
      }

      this.audioCtx = new AudioContext({sampleRate: this.current!.sampleRate});
      const oscNode = this.audioCtx.createOscillator();
      oscNode.type = 'sine';
      oscNode.channelCount = this.current!.channelCount;
      oscNode.frequency.value = this.current!.freqency;
      if (this.current!.channelCount == 2) {
        const panNode = this.audioCtx.createStereoPanner();
        panNode.pan.value = this.current!.pan;
        oscNode.connect(panNode);
        panNode.connect(this.audioCtx.destination);
      } else {
        oscNode.connect(this.audioCtx.destination);
      }
      if (this.timer) {
        clearTimeout(this.timer);
      }
      this.timer = setTimeout(() => {
        this.audioCtx?.suspend();
        this.audioQuery.hidden = false;
      }, 3000);
      oscNode.start();
    });
  }

  createButton(buttonName: string, buttonText: string) {
    const button = document.createElement('button');
    const buttonClassName = buttonName + '-btn';
    button.setAttribute('class', buttonClassName);
    button.textContent = buttonText;
    this.audioDiv.appendChild(button);
    return button;
  }

  handleBackClick() {
    this.sampleIdx -= 1;
    this.audioCtx?.suspend();
    if (this.timer) {
      clearTimeout(this.timer);
      this.timer = undefined;
    }
    this.setUpAudioPlayer();
    this.prevLink.hidden = this.sampleIdx == 0;
    this.audioQuery.hidden = true;
  }

  handleResponse(response: boolean) {
    OutputPage.getInstance().setOutputMapEntry(this.current!, response);
    if (this.sampleIdx + 1 == this.audioSamples.length) {
      PageNavigator.getInstance().showPage('feedback');
    } else {
      this.sampleIdx += 1;
      this.setUpAudioPlayer();
      this.audioQuery.hidden = true;
      this.prevLink.hidden = this.sampleIdx == 0;
    }
  }
}
customElements.define('audio-player', AudioPlayer);
