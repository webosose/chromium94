// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

GEN_INCLUDE([
  '//chrome/browser/resources/chromeos/accessibility/chromevox/testing/chromevox_next_e2e_test_base.js'
]);

/**
 * Gets the braille output and asserts that it matches expected values.
 * Annotations in the output that are primitive strings are ignored.
 */
function checkBrailleOutput(expectedText, expectedSpans, output) {
  const actualOutput = output.brailleOutputForTest;
  // Remove string annotations.  These are tested in the speech output and
  // there's no need to clutter the tests with the corresponding braille
  // annotations.
  const actualSpans = actualOutput.spans_.filter(function(span) {
    return (typeof span.value !== 'string');
  });
  checkOutput_(
      expectedText, expectedSpans, actualOutput.toString(), actualSpans);
}

function checkSpeechOutput(expectedText, expectedSpans, output) {
  const actualOutput = output.speechOutputForTest;
  checkOutput_(
      expectedText, expectedSpans, actualOutput.toString(),
      actualOutput.spans_);
}

/** @private */
function checkOutput_(expectedText, expectedSpans, actualText, actualSpans) {
  assertEquals(expectedText, actualText);

  function describeSpanPrettyPrint(span) {
    return describeSpan(span)
        .replace(':', ': ')
        .replace('"value":', 'value:')
        .replace('"start":', 'start:')
        .replace('"end":', 'end:')
        .replace('"', '\'');
  }

  function describeSpan(span) {
    const obj = {value: span.value, start: span.start, end: span.end};
    if (obj.value instanceof OutputNodeSpan) {
      obj.value.node =
          (obj.value.node.name || '') + ' ' + obj.value.node.toString();
    }
    return JSON.stringify(obj);
  }

  function describeActualSpans() {
    return '\nAll actual spans:\n' +
        actualSpans.map(describeSpanPrettyPrint).join('\n');
  }

  for (let i = 0, max = Math.max(expectedSpans.length, actualSpans.length);
       i < max; ++i) {
    const expectedSpan = expectedSpans[i];
    const actualSpan = actualSpans[i];
    if (!expectedSpan) {
      throw Error(
          'Unexpected span in ' + expectedText + ': ' +
          describeSpan(actualSpan) + describeActualSpans());
    }
    if (!actualSpan) {
      throw Error(
          'Missing expected span in ' + expectedText + ': ' +
          describeSpan(expectedSpan) + describeActualSpans());
    }
    let equal = true;
    if (expectedSpan.start !== actualSpan.start ||
        expectedSpan.end !== actualSpan.end) {
      equal = false;
    } else if (
        expectedSpan.value instanceof OutputNodeSpan &&
        (!(actualSpan.value instanceof OutputNodeSpan) ||
         expectedSpan.value.node !== actualSpan.value.node)) {
      equal = false;
    } else {
      equal =
          (JSON.stringify(expectedSpan.value) ===
           JSON.stringify(actualSpan.value));
    }
    if (!equal) {
      throw Error(
          'Spans differ in ' + expectedText + ':\n' +
          'Expected: ' + describeSpan(expectedSpan) + '\n' +
          'Got     : ' + describeSpan(actualSpan) + describeActualSpans());
    }
  }
}

/**
 * Test fixture for output.js.
 */
ChromeVoxOutputE2ETest = class extends ChromeVoxNextE2ETest {
  /** @override */
  setUp() {
    super.setUp();
    window.Dir = AutomationUtil.Dir;
    this.forceContextualLastOutput();
  }
};


TEST_F('ChromeVoxOutputE2ETest', 'Links', function() {
  this.runWithLoadedTree('<a href="#">Click here</a>', function(root) {
    const el = root.firstChild.firstChild;
    const range = cursors.Range.fromNode(el);
    const o = new Output().withSpeechAndBraille(range, null, 'navigate');
    assertEqualsJSON(
        {
          string_: 'Click here|Internal link|Press Search+Space to activate',
          'spans_': [
            // Attributes.
            {value: 'name', start: 0, end: 10},

            // Link earcon (based on the name).
            {value: {earconId: 'LINK'}, start: 0, end: 10},

            {value: {'delay': true}, start: 25, end: 55}
          ]
        },
        o.speechOutputForTest);
    checkBrailleOutput(
        'Click here intlnk',
        [{value: new OutputNodeSpan(el), start: 0, end: 17}], o);
  });
});

TEST_F('ChromeVoxOutputE2ETest', 'Checkbox', function() {
  this.runWithLoadedTree('<input type="checkbox">', function(root) {
    const el = root.firstChild.firstChild;
    const range = cursors.Range.fromNode(el);
    const o = new Output().withSpeechAndBraille(range, null, 'navigate');
    checkSpeechOutput(
        '|Check box|Not checked|Press Search+Space to toggle',
        [
          {value: new OutputEarconAction('CHECK_OFF'), start: 0, end: 0},
          {value: 'role', start: 1, end: 10},
          {value: {'delay': true}, start: 23, end: 51}
        ],
        o);
    checkBrailleOutput(
        'chk ( )', [{value: new OutputNodeSpan(el), start: 0, end: 7}], o);
  });
});

TEST_F('ChromeVoxOutputE2ETest', 'InLineTextBoxValueGetsIgnored', function() {
  this.runWithLoadedTree('<p>OK', function(root) {
    let el = root.firstChild.firstChild.firstChild;
    assertEquals('inlineTextBox', el.role);
    let range = cursors.Range.fromNode(el);
    let o = new Output().withSpeechAndBraille(range, null, 'navigate');
    assertEqualsJSON(
        {
          string_: 'OK',
          'spans_': [
            // Attributes.
            {value: 'name', start: 0, end: 2}
          ]
        },
        o.speechOutputForTest);
    checkBrailleOutput(
        'OK', [{value: new OutputNodeSpan(el), start: 0, end: 2}], o);

    el = root.firstChild.firstChild;
    assertEquals('staticText', el.role);
    range = cursors.Range.fromNode(el);
    o = new Output().withSpeechAndBraille(range, null, 'navigate');
    assertEqualsJSON(
        {
          string_: 'OK',
          'spans_': [
            // Attributes.
            {value: 'name', start: 0, end: 2}
          ]
        },
        o.speechOutputForTest);
    checkBrailleOutput(
        'OK', [{value: new OutputNodeSpan(el), start: 0, end: 2}], o);
  });
});

TEST_F('ChromeVoxOutputE2ETest', 'Headings', function() {
  this.runWithLoadedTree(
      `
      <h1>a</h1><h2>b</h2><h3>c</h3><h4>d</h4><h5>e</h5><h6>f</h6>
      <h1><a href="a.com">b</a></h1> `,
      function(root) {
        let el = root.firstChild;
        for (let i = 1; i <= 6; ++i) {
          const range = cursors.Range.fromNode(el);
          const o = new Output().withSpeechAndBraille(range, null, 'navigate');
          const letter = String.fromCharCode('a'.charCodeAt(0) + i - 1);
          assertEqualsJSON(
              {
                string_: letter + '|Heading ' + i,
                'spans_': [
                  // Attributes.
                  {value: 'nameOrDescendants', start: 0, end: 1}
                ]
              },
              o.speechOutputForTest);
          checkBrailleOutput(
              letter + ' h' + i,
              [{value: new OutputNodeSpan(el), start: 0, end: 4}], o);
          el = el.nextSibling;
        }

        range = cursors.Range.fromNode(el);
        o = new Output().withSpeechAndBraille(range, null, 'navigate');
        assertEqualsJSON(
            {
              string_: 'b|Link|Heading 1',
              'spans_': [
                {value: 'name', start: 0, end: 1},
                {value: new OutputEarconAction('LINK'), start: 0, end: 1},
                {value: 'role', start: 2, end: 6}
              ]
            },
            o.speechOutputForTest);
        checkBrailleOutput(
            'b lnk h1',
            [
              {
                value: new OutputNodeSpan(el.firstChild.firstChild),
                start: 0,
                end: 1
              },
              {value: new OutputNodeSpan(el), start: 0, end: 8},
              {value: new OutputNodeSpan(el.firstChild), start: 2, end: 5}
            ],
            o);
      });
});

// TODO(crbug.com/901725): test is flaky.
TEST_F('ChromeVoxOutputE2ETest', 'DISABLED_Audio', function() {
  this.runWithLoadedTree(
      '<audio src="foo.mp3" controls></audio>', function(root) {
        let el = root.find({role: RoleType.BUTTON});
        let range = cursors.Range.fromNode(el);
        let o = new Output().withoutHints().withSpeechAndBraille(
            range, null, 'navigate');

        checkSpeechOutput(
            'play|Disabled|Button|audio|Tool bar',
            [
              {value: new OutputEarconAction('BUTTON'), start: 0, end: 4},
              {value: 'name', start: 21, end: 26},
              {value: 'role', start: 27, end: 35}
            ],
            o);

        checkBrailleOutput(
            'play xx btn audio tlbar',
            [
              {value: new OutputNodeSpan(el), start: 0, end: 11},
              {value: new OutputNodeSpan(el.parent), start: 12, end: 23}
            ],
            o);

        // TODO(dmazzoni/dtseng): Replace with a query.
        el = el.nextSibling.nextSibling.nextSibling;
        const prevRange = range;
        range = cursors.Range.fromNode(el);
        o = new Output().withoutHints().withSpeechAndBraille(
            range, prevRange, 'navigate');
        checkSpeechOutput(
            '|audio time scrubber|Slider|0:00|Min 0|Max 0',
            [
              {value: 'name', start: 0, end: 0},
              {value: new OutputEarconAction('SLIDER'), start: 0, end: 0},
              {value: 'description', start: 1, end: 20},
              {value: 'role', start: 21, end: 27},
              {value: 'value', start: 28, end: 32}
            ],
            o);
        checkBrailleOutput(
            'audio time scrubber sldr 0:00 min:0 max:0',
            [{value: new OutputNodeSpan(el), start: 0, end: 41}], o);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'Input', function() {
  this.runWithLoadedTree(
      '<input type="text"></input>' +
          '<input type="email"></input>' +
          '<input type="password"></input>' +
          '<input type="tel"></input>' +
          '<input type="number"></input>' +
          '<input type="time"></input>' +
          '<input type="date"></input>' +
          '<input type="file"</input>' +
          '<input type="search"</input>' +
          '<input type="invalidType"</input>',
      function(root) {
        const expectedSpansNonSearchBox = [
          {value: 'name', start: 0, end: 0},
          {value: new OutputEarconAction('EDITABLE_TEXT'), start: 0, end: 0},
          {value: new OutputSelectionSpan(0, 0, 0), start: 1, end: 1},
          {value: 'value', start: 1, end: 1}, {value: 'inputType', start: 2}
        ];
        const expectedSpansForSearchBox = [
          {value: 'name', start: 0, end: 0},
          {value: new OutputEarconAction('EDITABLE_TEXT'), start: 0, end: 0},
          {value: new OutputSelectionSpan(0, 0, 0), start: 1, end: 1},
          {value: 'value', start: 1, end: 1}, {value: 'role', start: 2, end: 8}
        ];

        const expectedSpeechValues = [
          '||Edit text', '||Edit text, email entry', '||Password edit text',
          '||Edit text numeric only',
          [
            '|Spin button',
            [
              {value: 'name', start: 0, end: 0},
              {value: new OutputEarconAction('LISTBOX'), start: 0, end: 0},
              {value: 'role', start: 1, end: 12}
            ]
          ],
          ['Time control', [{value: 'role', start: 0, end: 12}]],
          ['Date control', [{value: 'role', start: 0, end: 12}]],
          [
            'No file chosen, Choose File|Button',
            [
              {value: 'name', start: 0, end: 27},
              {value: new OutputEarconAction('BUTTON'), start: 0, end: 27},
              {value: 'role', start: 28, end: 34}
            ]
          ],
          '||Search', '||Edit text'
        ];
        // TODO(plundblad): Some of these are wrong, there should be an initial
        // space for the cursor in edit fields.
        const expectedBrailleValues = [
          ' ed', ' @ed 8dot', ' pwded', ' #ed', {string_: 'spnbtn', spans_: []},
          {string_: 'time'}, {string_: 'date'},
          {string_: 'No file chosen, Choose File btn'}, ' search', ' ed'
        ];
        assertEquals(expectedSpeechValues.length, expectedBrailleValues.length);

        let el = root.firstChild.firstChild;
        expectedSpeechValues.forEach(function(expectedValue) {
          const range = cursors.Range.fromNode(el);
          const o = new Output().withoutHints().withSpeechAndBraille(
              range, null, 'navigate');
          let expectedSpansForValue = null;
          if (typeof expectedValue === 'object') {
            checkSpeechOutput(expectedValue[0], expectedValue[1], o);
          } else {
            expectedSpansForValue = expectedValue === '||Search' ?
                expectedSpansForSearchBox :
                expectedSpansNonSearchBox;
            expectedSpansForValue[4].end = expectedValue.length;
            checkSpeechOutput(expectedValue, expectedSpansForValue, o);
          }
          el = el.nextSibling;
        });

        el = root.firstChild.firstChild;
        expectedBrailleValues.forEach(function(expectedValue) {
          const range = cursors.Range.fromNode(el);
          const o =
              new Output().withoutHints().withBraille(range, null, 'navigate');
          if (typeof expectedValue === 'string') {
            checkBrailleOutput(
                expectedValue,
                [
                  {value: {startIndex: 0, endIndex: 0}, start: 0, end: 0}, {
                    value: new OutputNodeSpan(el),
                    start: 0,
                    end: expectedValue.length
                  }
                ],
                o);
          } else {
            let spans = [{
              value: new OutputNodeSpan(el),
              start: 0,
              end: expectedValue.string_.length
            }];
            if (expectedValue.spans_) {
              spans = spans.concat(expectedValue.spans_);
            }

            checkBrailleOutput(expectedValue.string_, spans, o);
          }
          el = el.nextSibling;
        });
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'List', function() {
  this.runWithLoadedTree(
      '<ul aria-label="first"><li aria-label="a">a<li>b<li>c</ul>',
      function(root) {
        const el = root.firstChild.firstChild;
        const range = cursors.Range.fromNode(el);
        const o = new Output().withSpeechAndBraille(range, null, 'navigate');
        checkSpeechOutput(
            'a|List item|first|List|with 3 items',
            [
              {value: {earconId: 'LIST_ITEM'}, start: 0, end: 1},
              {value: 'name', start: 12, end: 17},
              {value: 'role', start: 18, end: 22}
            ],
            o);
        // TODO(plundblad): This output is wrong.  Add special handling for
        // braille here.
        checkBrailleOutput(
            'a lstitm first lst +3',
            [
              {value: new OutputNodeSpan(el), start: 0, end: 8},
              {value: new OutputNodeSpan(el.parent), start: 9, end: 21}
            ],
            o);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'Tree', function() {
  this.runWithLoadedTree(
      `
    <ul role="tree" style="list-style-type:none">
      <li aria-expanded="true" role="treeitem">a
      <li role="treeitem">b
      <li aria-expanded="false" role="treeitem">c
    </ul>
  `,
      function(root) {
        let el = root.firstChild.children[0].firstChild;
        let range = cursors.Range.fromNode(el);
        let o = new Output().withSpeechAndBraille(range, null, 'navigate');
        checkSpeechOutput(
            'a|Tree item|Expanded| 1 of 3 | level 1 |Tree|with 3 items',
            [
              {value: 'name', 'start': 0, end: 1},
              {value: 'state', start: 12, end: 20},
              {value: 'role', 'start': 40, end: 44},
            ],
            o);
        checkBrailleOutput(
            'a tritm - 1/3 level 1 tree +3',
            [
              {value: new OutputNodeSpan(el), start: 0, end: 1},
              {value: new OutputNodeSpan(el.parent), start: 2, end: 22},
              {value: new OutputNodeSpan(el.parent.parent), start: 22, end: 29}
            ],
            o);

        el = root.firstChild.children[1].firstChild;
        range = cursors.Range.fromNode(el);
        o = new Output().withSpeechAndBraille(range, null, 'navigate');
        checkSpeechOutput(
            'b|Tree item| 2 of 3 | level 1 |Tree|with 3 items',
            [
              {value: 'name', start: 0, end: 1},
              {value: 'role', 'start': 31, end: 35}
            ],
            o);
        checkBrailleOutput(
            'b tritm 2/3 level 1 tree +3',
            [
              {value: new OutputNodeSpan(el), start: 0, end: 1},
              {value: new OutputNodeSpan(el.parent), start: 2, end: 20},
              {value: new OutputNodeSpan(el.parent.parent), start: 20, end: 27}
            ],
            o);

        el = root.firstChild.children[2].firstChild;
        range = cursors.Range.fromNode(el);
        o = new Output().withSpeechAndBraille(range, null, 'navigate');
        checkSpeechOutput(
            'c|Tree item|Collapsed| 3 of 3 | level 1 |Tree|with 3 items',
            [
              {value: 'name', 'start': 0, end: 1},
              {value: 'state', start: 12, end: 21},
              {value: 'role', 'start': 41, end: 45},
            ],
            o);
        checkBrailleOutput(
            'c tritm + 3/3 level 1 tree +3',
            [
              {value: new OutputNodeSpan(el), start: 0, end: 1},
              {value: new OutputNodeSpan(el.parent), start: 2, end: 22},
              {value: new OutputNodeSpan(el.parent.parent), start: 22, end: 29}
            ],
            o);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'Menu', function() {
  const site = `
    <div role="menu">
      <div role="menuitem">a</div>
      <div role="menuitemcheckbox">b</div>
      <div role="menuitemradio">c</div>
    </div>
    <div role="menubar" aria-orientation="horizontal"></div>
  `;
  this.runWithLoadedTree(site, function(root) {
    let el = root.firstChild.firstChild;
    let range = cursors.Range.fromNode(el);
    let o = new Output().withSpeechAndBraille(range, null, 'navigate');
    checkSpeechOutput(
        'a|Menu item| 1 of 3 |Menu',
        [
          {value: 'name', start: 0, end: 1}, {value: 'role', start: 21, end: 25}
        ],
        o);
    checkBrailleOutput(
        'a mnuitm 1/3 mnu',
        [
          {value: new OutputNodeSpan(el), start: 0, end: 12},
          {value: new OutputNodeSpan(el.parent), start: 13, end: 16}
        ],
        o);

    // Ancestry.
    el = root.firstChild;
    range = cursors.Range.fromNode(el);
    o = new Output().withSpeechAndBraille(range, null, 'navigate');
    checkSpeechOutput(
        'Menu|with 3 items|' +
            'Press up or down arrow to navigate; enter to activate',
        [
          {value: 'role', start: 0, end: 4},
          {value: {delay: true}, start: 18, end: 71}
        ],
        o);

    el = root.lastChild;
    range = cursors.Range.fromNode(el);
    o = new Output().withSpeechAndBraille(range, null, 'navigate');
    checkSpeechOutput(
        'Menu bar|Press left or right arrow to navigate; enter to activate',
        [
          {value: 'role', start: 0, end: 8},
          {value: {delay: true}, start: 9, end: 65}
        ],
        o);
  });
});

TEST_F('ChromeVoxOutputE2ETest', 'ListBox', function() {
  this.runWithLoadedTree(
      `
    <select multiple>
      <option>1</option>
      <option>2</option>
    </select>
  `,
      function(root) {
        const el = root.firstChild.firstChild.firstChild;
        const range = cursors.Range.fromNode(el);
        const o = new Output().withSpeechAndBraille(range, null, 'navigate');
        checkSpeechOutput(
            '1|List item| 1 of 2 |Not selected|List box|with 2 items',
            [
              {value: 'name', start: 0, end: 1},
              {value: new OutputEarconAction('LIST_ITEM'), start: 0, end: 1},
              {value: 'role', start: 34, end: 42}
            ],
            o);
        checkBrailleOutput(
            '1 lstitm 1/2 ( ) lstbx +2',
            [
              {value: new OutputNodeSpan(el), start: 0, end: 16},
              {value: new OutputNodeSpan(el.parent), start: 17, end: 25}
            ],
            o);
      });
});

SYNC_TEST_F('ChromeVoxOutputE2ETest', 'MessageIdAndEarconValidity', function() {
  const kNoBrailleMessageRequired = new Set([
    'comment',
    'contentDeletion',
    'contentInsertion',
    'docAbstract',
    'docAcknowledgments',
    'docAfterword',
    'docAppendix',
    'docBackLink',
    'docBiblioEntry',
    'docBibliography',
    'docBiblioRef',
    'docChapter',
    'docColophon',
    'docConclusion',
    'docCover',
    'docCredit',
    'docCredits',
    'docDedication',
    'docEndnote',
    'docEndnotes',
    'docEpigraph',
    'docEpilogue',
    'docErrata',
    'docExample',
    'docFootnote',
    'docForeword',
    'docGlossary',
    'docGlossRef',
    'docIndex',
    'docIntroduction',
    'docNoteRef',
    'docNotice',
    'docPageBreak',
    'docPageFooter',
    'docPageHeader',
    'docPageList',
    'docPart',
    'docPreface',
    'docPrologue',
    'docPullquote',
    'docQna',
    'docSubtitle',
    'docTip',
    'docToc',
    'graphicsDocument',
    'graphicsObject',
    'graphicsSymbol',
    'suggestion',
  ]);
  for (const key in Output.ROLE_INFO) {
    const value = Output.ROLE_INFO[key];
    if (value.msgId) {
      Msgs.getMsg(value.msgId);
      if (!kNoBrailleMessageRequired.has(key)) {
        Msgs.getMsg(value.msgId + '_brl');
      }
      assertFalse(/[A-Z]+/.test(value.msgId));
    }
    if (value.earconId) {
      assertNotNullNorUndefined(Earcon[value.earconId]);
    }
  }
  for (const key in Output.STATE_INFO_) {
    const value = Output.STATE_INFO_[key];
    for (innerKey in value) {
      const innerValue = value[innerKey];
      if (typeof (innerValue) === 'boolean') {
        assertEquals('isRoleSpecific', innerKey);
        continue;
      }
      Msgs.getMsg(innerValue.msgId);
      Msgs.getMsg(innerValue.msgId + '_brl');
      assertFalse(/[A-Z]+/.test(innerValue.msgId));
      if (innerValue.earconId) {
        assertNotNullNorUndefined(Earcon[innerValue.earconId]);
      }
    }
  }
  for (const key in Output.INPUT_TYPE_MESSAGE_IDS_) {
    const msgId = Output.INPUT_TYPE_MESSAGE_IDS_[key];
    assertFalse(/[A-Z]+/.test(msgId));
    Msgs.getMsg(msgId);
    Msgs.getMsg(msgId + '_brl');
  }
});

TEST_F('ChromeVoxOutputE2ETest', 'DivOmitsRole', function() {
  this.runWithLoadedTree(
      `
    <div>that has content</div>
    <div></div>
    <div role='group'><div>nested content</div></div>
  `,
      function(root) {
        const el = root.firstChild.firstChild;
        const range = cursors.Range.fromNode(el);
        const o = new Output().withSpeechAndBraille(range, null, 'navigate');
        checkSpeechOutput(
            'that has content', [{value: 'name', start: 0, end: 16}], o);
        checkBrailleOutput(
            'that has content',
            [{value: new OutputNodeSpan(el), start: 0, end: 16}], o);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'LessVerboseAncestry', function() {
  this.runWithLoadedTree(
      `
    <div role="banner"><p>inside</p></div>
    <div role="banner"><p>inside</p></div>
    <div role="navigation"><p>inside</p></div>
  `,
      function(root) {
        const first = root.children[0].firstChild;
        const second = root.children[1].firstChild;
        const third = root.children[2].firstChild;
        const firstRange = cursors.Range.fromNode(first);
        const secondRange = cursors.Range.fromNode(second);
        const thirdRange = cursors.Range.fromNode(third);

        const oWithoutPrev =
            new Output().withSpeech(firstRange, null, 'navigate');
        const oWithPrev =
            new Output().withSpeech(secondRange, firstRange, 'navigate');
        const oWithPrevExit =
            new Output().withSpeech(thirdRange, secondRange, 'navigate');
        assertEquals('inside|Banner', oWithoutPrev.speechOutputForTest.string_);

        // Make sure we don't read the exited ancestry change.
        assertEquals('inside|Banner', oWithPrev.speechOutputForTest.string_);

        // Different role; do read the exited ancestry here.
        assertEquals(
            'inside|Navigation', oWithPrevExit.speechOutputForTest.string_);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'Brief', function() {
  this.runWithLoadedTree(
      `
    <div role="article"><p>inside</p></div>
  `,
      function(root) {
        const node = root.children[0].firstChild;
        const range = cursors.Range.fromNode(node);

        localStorage['useVerboseMode'] = 'false';
        const oWithoutPrev = new Output().withSpeech(range, null, 'navigate');
        assertEquals('inside', oWithoutPrev.speechOutputForTest.string_);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'AuralStyledHeadings', function() {
  function toFixed(num) {
    return parseFloat(Number(num).toFixed(1));
  }
  this.runWithLoadedTree(
      `
      <h1>a</h1><h2>b</h2><h3>c</h3><h4>d</h4><h5>e</h5><h6>f</h6>
      <h1><a href="a.com">b</a></h1> `,
      function(root) {
        let el = root.firstChild;
        for (let i = 1; i <= 6; ++i) {
          const range = cursors.Range.fromNode(el);
          const o = new Output().withRichSpeech(range, null, 'navigate');
          const letter = String.fromCharCode('a'.charCodeAt(0) + i - 1);
          assertEqualsJSON(
              {
                string_: letter + '|Heading ' + i,
                'spans_': [
                  // Aural styles.
                  {
                    value: {'relativePitch': toFixed(-0.1 * i)},
                    start: 0,
                    end: 0
                  },

                  // Attributes.
                  {value: 'nameOrDescendants', start: 0, end: 1},

                  {value: {'relativePitch': -0.2}, start: 2, end: 2}
                ]
              },
              o.speechOutputForTest);
          el = el.nextSibling;
        }
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'ToggleButton', function() {
  this.runWithLoadedTree(
      `
      <div role="button" aria-pressed="true">Subscribe</div>`,
      function(root) {
        const el = root.firstChild;
        const o = new Output().withSpeechAndBraille(cursors.Range.fromNode(el));
        assertEqualsJSON(
            {
              string_:
                  '|Subscribe|Toggle Button|Pressed|Press Search+Space to toggle',
              spans_: [
                {value: {earconId: 'CHECK_ON'}, start: 0, end: 0},
                {value: 'name', start: 1, end: 10},
                {value: 'role', start: 11, end: 24},
                {value: {'delay': true}, start: 33, end: 61}
              ]
            },
            o.speechOutputForTest);
        assertEquals('Subscribe tgl btn =', o.brailleOutputForTest.string_);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'JoinDescendants', function() {
  this.runWithLoadedTree(
      `
      <p>This</p>
      <p>fragment</p>
      <p>Should be separated</p>
      <p>with spaces</p>
    `,
      function(root) {
        const unjoined = new Output().format('$descendants', root);
        assertEquals(
            'This|fragment|Should be separated|with spaces',
            unjoined.speechOutputForTest.string_);

        const joined = new Output().format('$joinedDescendants', root);
        assertEquals(
            'This fragment Should be separated with spaces',
            joined.speechOutputForTest.string_);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'ComplexDiv', function() {
  this.runWithLoadedTree(
      `
      <div><button>ok</button></div>
    `,
      function(root) {
        const div = root.find({role: RoleType.GENERIC_CONTAINER});
        const o = new Output().withSpeech(cursors.Range.fromNode(div));
        assertEquals('ok', o.speechOutputForTest.string_);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'ContainerFocus', function() {
  this.runWithLoadedTree(
      `
      <div role="grid">
        <div role="row" tabindex=0 aria-label="start"></div>
        <div role="row" tabindex=0 aria-label="end"></div>
      </div>
    `,
      function(root) {
        const r1 = cursors.Range.fromNode(root.firstChild.firstChild);
        const r2 = cursors.Range.fromNode(root.firstChild.lastChild);
        assertEquals(
            'start|Row',
            new Output().withSpeech(r1, r2).speechOutputForTest.string_);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'BraileWhitespace', function() {
  this.runWithLoadedTree(
      `
    <p>this is a <em>test</em>of emphasized text</p>
  `,
      function(root) {
        const start = root.firstChild.firstChild;
        const end = root.firstChild.lastChild;
        const range = new cursors.Range(
            cursors.Cursor.fromNode(start), cursors.Cursor.fromNode(end));
        const o = new Output().withBraille(range, null, 'navigate');
        checkBrailleOutput(
            'this is a test of emphasized text',
            [
              {value: new OutputNodeSpan(start), start: 0, end: 10}, {
                value: new OutputNodeSpan(start.nextSibling),
                start: 10,
                end: 14
              },
              {value: new OutputNodeSpan(end), start: 15, end: 33}
            ],
            o);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'BrailleAncestry', function() {
  this.runWithLoadedTree(
      `
    <ul><li><a href="#">test</a></li></ul>
  `,
      function(root) {
        const link = root.find({role: RoleType.LINK});
        // The 'inlineTextBox' found from root would return the inlineTextBox of
        // the list marker. Here we want the link's inlineTextBox.
        const text = link.find({role: RoleType.INLINE_TEXT_BOX});
        const listItem = root.find({role: RoleType.LIST_ITEM});
        const list = root.find({role: RoleType.LIST});
        let range = cursors.Range.fromNode(text);
        let o = new Output().withBraille(range, null, 'navigate');
        checkBrailleOutput(
            'test lnk lstitm lst end',
            [
              {value: new OutputNodeSpan(text), start: 0, end: 4},
              {value: new OutputNodeSpan(link), start: 5, end: 8},
              {value: new OutputNodeSpan(listItem), start: 9, end: 15}
            ],
            o);

        // Now, test the "bullet" which comes before the above.
        const bullet = root.find({role: RoleType.INLINE_TEXT_BOX});
        range = cursors.Range.fromNode(bullet);
        o = new Output().withBraille(range, null, 'navigate');
        checkBrailleOutput(
            '\u2022 lstitm lst +1',
            [
              {value: new OutputNodeSpan(bullet), start: 0, end: 2},
              {value: new OutputNodeSpan(listItem), start: 2, end: 8},
              {value: new OutputNodeSpan(list), start: 9, end: 15}
            ],
            o);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'RangeOutput', function() {
  this.runWithLoadedTree(
      `
    <div role="slider" aria-valuemin="1" aria-valuemax="10" aria-valuenow="2"
                       aria-label="volume"></div>
    <progress aria-valuemin="1" aria-valuemax="10"
              aria-valuenow="2" aria-label="volume"></progress>
    <meter aria-valuemin="1" aria-valuemax="10" aria-valuenow="2"
           aria-label="volume"></meter>
    <div role="spinbutton" aria-valuemin="1" aria-valuemax="10"
                           aria-valuenow="2" aria-label="volume"></div>
  `,
      function(root) {
        let obj = root.find({role: RoleType.SLIDER});
        let o =
            new Output().withoutHints().withSpeech(cursors.Range.fromNode(obj));
        checkSpeechOutput(
            'volume|Slider|2|Min 1|Max 10',
            [
              {value: 'name', start: 0, end: 6},
              {value: new OutputEarconAction('SLIDER'), start: 0, end: 6},
              {value: 'role', start: 7, end: 13},
              {value: 'value', start: 14, end: 15}
            ],
            o);

        obj = root.find({role: RoleType.PROGRESS_INDICATOR});
        o = new Output().withoutHints().withSpeech(cursors.Range.fromNode(obj));
        checkSpeechOutput(
            'volume|Progress indicator|2|Min 1|Max 10',
            [
              {value: 'name', start: 0, end: 6},
              {value: 'role', start: 7, end: 25},
              {value: 'value', start: 26, end: 27}
            ],
            o);

        obj = root.find({role: RoleType.METER});
        o = new Output().withoutHints().withSpeech(cursors.Range.fromNode(obj));
        checkSpeechOutput(
            'volume|Meter|2|Min 1|Max 10',
            [
              {value: 'name', start: 0, end: 6},
              {value: 'role', start: 7, end: 12},
              {value: 'value', start: 13, end: 14}
            ],
            o);

        obj = root.find({role: RoleType.SPIN_BUTTON});
        o = new Output().withoutHints().withSpeech(cursors.Range.fromNode(obj));
        checkSpeechOutput(
            'volume|Spin button|2|Min 1|Max 10',
            [
              {value: 'name', start: 0, end: 6},
              {value: new OutputEarconAction('LISTBOX'), start: 0, end: 6},
              {value: 'role', start: 7, end: 18},
              {value: 'value', start: 19, end: 20}
            ],
            o);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'RoleDescription', function() {
  this.runWithLoadedTree(
      `
    <div aria-label="hi" role="button" aria-roledescription="foo"></div>
  `,
      function(root) {
        const obj = root.find({role: RoleType.BUTTON});
        const o =
            new Output().withoutHints().withSpeech(cursors.Range.fromNode(obj));
        checkSpeechOutput(
            'hi|foo',
            [
              {value: 'name', start: 0, end: 2},
              {value: new OutputEarconAction('BUTTON'), start: 0, end: 2},
              {value: 'role', start: 3, end: 6}
            ],
            o);
      });
});

SYNC_TEST_F('ChromeVoxOutputE2ETest', 'ValidateCommonProperties', function() {
  // If you fail this test, you likely need to insert a $state, $restriction or
  // $description into the output rules for the printed roles. Typically,
  // $description goes towards the end of the output rule, though this depends
  // on the role. For example, it could make sense to put $description before
  // $value or $state.

  // You can also add the role to be excluded from this check. You are
  // encouraged to write a more intelligent output rule to provide friendlier
  // feedback. For example, 'not selected apple item 2 out of 3' coming from a
  // message template like '@smart_selection($state, $name, $indexInParent,
  // $childCount)'.
  // In such cases, you are responsible for ensuring you include all states and
  // descriptions somewhere in the output.

  const stateStr = '$state';
  const restrictionStr = '$restriction';
  const descStr = '$description';
  let missingState = [];
  let missingRestriction = [];
  let missingDescription = [];
  for (const key in Output.RULES.navigate) {
    const speak = Output.RULES.navigate[key].speak;
    if (!speak) {
      continue;
    }

    if (speak.indexOf(stateStr) === -1) {
      missingState.push(key);
    }
    if (speak.indexOf(restrictionStr) === -1) {
      missingRestriction.push(key);
    }
    if (speak.indexOf(descStr) === -1) {
      missingDescription.push(key);
    }
  }

  // This filters out known roles that don't have states or descriptions.
  const notStated = [
    RoleType.CLIENT, RoleType.EMBEDDED_OBJECT, RoleType.IME_CANDIDATE,
    RoleType.INLINE_TEXT_BOX, RoleType.LINE_BREAK, RoleType.LIST_MARKER,
    RoleType.PARAGRAPH, RoleType.ROOT_WEB_AREA, RoleType.STATIC_TEXT,
    RoleType.PLUGIN_OBJECT, RoleType.WINDOW
  ];
  const notRestricted = [
    RoleType.ALERT,
    RoleType.ALERT_DIALOG,
    RoleType.CELL,
    RoleType.CLIENT,
    RoleType.EMBEDDED_OBJECT,
    RoleType.GENERIC_CONTAINER,
    RoleType.IMAGE,
    RoleType.IME_CANDIDATE,
    RoleType.INLINE_TEXT_BOX,
    RoleType.LINE_BREAK,
    RoleType.LIST,
    RoleType.LIST_MARKER,
    RoleType.PARAGRAPH,
    RoleType.PLUGIN_OBJECT,
    RoleType.REGION,
    RoleType.ROOT_WEB_AREA,
    RoleType.ROW_HEADER,
    RoleType.STATIC_TEXT,
    RoleType.TABLE_HEADER_CONTAINER,
    RoleType.TIMER,
    RoleType.WINDOW
  ];
  const notDescribed = [
    RoleType.CLIENT, RoleType.EMBEDDED_OBJECT, RoleType.IME_CANDIDATE,
    RoleType.INLINE_TEXT_BOX, RoleType.LINE_BREAK, RoleType.LIST_MARKER,
    RoleType.PARAGRAPH, RoleType.PLUGIN_OBJECT, RoleType.ROOT_WEB_AREA,
    RoleType.STATIC_TEXT, RoleType.WINDOW
  ];
  missingState = missingState.filter(function(state) {
    return notStated.indexOf(state) === -1;
  });
  missingRestriction = missingRestriction.filter(function(restriction) {
    return notRestricted.indexOf(restriction) === -1;
  });
  missingDescription = missingDescription.filter(function(desc) {
    return notDescribed.indexOf(desc) === -1;
  });

  assertEquals(
      0, missingState.length,
      'Unexpected missing states for output rules ' + missingState.join(' '));
  assertEquals(
      0, missingRestriction.length,
      'Unexpected missing restriction for output rules ' +
          missingRestriction.join(' '));
  assertEquals(
      0, missingDescription.length,
      'Unexpected missing descriptions for output rules ' +
          missingDescription.join(' '));
});

SYNC_TEST_F('ChromeVoxOutputE2ETest', 'ValidateRoles', function() {
  // If you fail this test, you likely need to insert a $role or
  // $roledescription into the output rules for the printed roles. Typically,
  // roles can be omitted (intentionally), but role descriptions cannot by W3C
  // spec (valid on all base markup). However, not all roles come from the web.
  //
  // Some enter rules do not need roles, depending on how they are used such as
  // if their speak rule is always triggered during navigation e.g. buttons.
  //
  // You can also add the role to be excluded from this check. You are
  // encouraged to write a more intelligent output rule to provide friendlier
  // feedback, but keep in mind role descriptions are required on all web-based
  // role.

  const roleOrRoleDescStr = '$role';
  const missingRole = [];
  const allowedMissingRoles = [
    RoleType.CLIENT, RoleType.GENERIC_CONTAINER, RoleType.EMBEDDED_OBJECT,
    RoleType.IME_CANDIDATE, RoleType.INLINE_TEXT_BOX, RoleType.LINE_BREAK,
    RoleType.LIST_MARKER, RoleType.ROOT_WEB_AREA, RoleType.STATIC_TEXT,
    RoleType.WINDOW
  ];
  for (const key in Output.RULES.navigate) {
    if (allowedMissingRoles.indexOf(key) !== -1) {
      continue;
    }
    const speak = Output.RULES.navigate[key].speak;
    let enter = Output.RULES.navigate[key].enter;
    if (enter && enter.speak) {
      enter = enter.speak;
    }

    if (speak && speak.indexOf(roleOrRoleDescStr) === -1) {
      missingRole.push(key);
    }
    if (enter && enter.indexOf(roleOrRoleDescStr) === -1) {
      missingRole.push(key);
    }
  }
  assertEquals(
      0, missingRole.length,
      'Unexpected missing role or role description for output rules ' +
          missingRole.join(' '));
});

TEST_F('ChromeVoxOutputE2ETest', 'InlineBraille', function() {
  this.runWithLoadedTree(
      `
    <table border=1>
      <tr><td>Name</td><td id="active">Age</td><td>Address</td></tr>
    </table>
  `,
      function(root) {
        const obj = root.find({role: RoleType.CELL});
        const o =
            new Output().withRichSpeechAndBraille(cursors.Range.fromNode(obj));
        assertEquals(
            'Name|row 1 column 1|Table , 1 by 3',
            o.speechOutputForTest.string_);
        assertEquals(
            'Name r1c1 Age r1c2 Address r1c3', o.brailleOutputForTest.string_);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'TextFieldObeysRoleDescription', function() {
  this.runWithLoadedTree(
      `
    <div role="textbox" aria-roledescription="square"></div>
    <div role="region" aria-roledescription="circle"></div>
  `,
      function(root) {
        const text = root.find({role: RoleType.TEXT_FIELD});

        // True even though |text| does not have editable state.
        assertTrue(AutomationPredicate.editText(text));

        let o =
            new Output().withRichSpeechAndBraille(cursors.Range.fromNode(text));
        assertEquals('|square', o.speechOutputForTest.string_);
        assertEquals('square', o.brailleOutputForTest.string_);

        const region = root.find({role: RoleType.REGION});
        o = new Output().withRichSpeechAndBraille(
            cursors.Range.fromNode(region));
        assertEquals('circle', o.speechOutputForTest.string_);
        assertEquals('circle', o.brailleOutputForTest.string_);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'NestedList', function() {
  this.runWithLoadedTree(
      `
    <ul role="tree">schedule
      <li role="treeitem">wake up
      <li role="treeitem">drink coffee
      <ul role="tree">tasks
        <li role="treeitem" aria-level='2'>meeting
        <li role="treeitem" aria-level='2'>lunch
      </ul>
      <li role="treeitem">cook dinner
    </ul>
  `,

      function(root) {
        const lists = root.findAll({role: RoleType.TREE});
        const outerList = lists[0];
        const innerList = lists[1];

        let el = outerList.children[0];
        let startRange = cursors.Range.fromNode(el);
        let o = new Output().withSpeech(startRange, null, 'navigate');
        assertEquals(
            'schedule|Tree|with 3 items', o.speechOutputForTest.string_);

        el = outerList.children[1];
        startRange = cursors.Range.fromNode(el);
        o = new Output().withSpeech(
            startRange, cursors.Range.fromNode(outerList.children[0]),
            'navigate');
        assertEquals(
            'wake up|Tree item|Not selected| 1 of 3 | level 1 ',
            o.speechOutputForTest.string_);

        el = outerList.children[2];
        startRange = cursors.Range.fromNode(el);
        o = new Output().withSpeech(
            startRange, cursors.Range.fromNode(outerList.children[0]),
            'navigate');
        assertEquals(
            'drink coffee|Tree item|Not selected| 2 of 3 | level 1 ',
            o.speechOutputForTest.string_);

        el = outerList.children[3];
        startRange = cursors.Range.fromNode(el);
        o = new Output().withSpeech(
            startRange, cursors.Range.fromNode(outerList.children[0]),
            'navigate');
        assertEquals(
            'cook dinner|Tree item|Not selected| 3 of 3 | level 1 ',
            o.speechOutputForTest.string_);

        el = innerList.children[0];
        startRange = cursors.Range.fromNode(el);
        o = new Output().withSpeech(
            startRange, cursors.Range.fromNode(outerList.children[2]),
            'navigate');
        assertEquals('tasks|Tree|with 2 items', o.speechOutputForTest.string_);

        el = innerList.children[1];
        startRange = cursors.Range.fromNode(el);
        o = new Output().withSpeech(
            startRange, cursors.Range.fromNode(innerList.children[0]),
            'navigate');
        assertEquals(
            'meeting|Tree item|Not selected| 1 of 2 | level 2 ',
            o.speechOutputForTest.string_);

        el = innerList.children[2];
        startRange = cursors.Range.fromNode(el);
        o = new Output().withSpeech(
            startRange, cursors.Range.fromNode(innerList.children[0]),
            'navigate');
        assertEquals(
            'lunch|Tree item|Not selected| 2 of 2 | level 2 ',
            o.speechOutputForTest.string_);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'NoTooltipWithNameTitle', function() {
  this.runWithLoadedTree(
      `
    <div title="title"></div>
    <div aria-label="label" title="title"></div>
    <div aria-describedby="desc" title="title"></div>
    <div aria-label="label" aria-describedby="desc" title="title"></div>
    <div aria-label=""></div>
    <p id="desc">describedby</p>
  `,
      function(root) {
        const title = root.children[0];
        let o = new Output().withSpeech(
            cursors.Range.fromNode(title), null, 'navigate');
        assertEqualsJSON(
            {string_: 'title', spans_: [{value: 'name', start: 0, end: 5}]},
            o.speechOutputForTest);

        const labelTitle = root.children[1];
        o = new Output().withSpeech(
            cursors.Range.fromNode(labelTitle), null, 'navigate');
        assertEqualsJSON(
            {
              string_: 'label|title',
              spans_: [
                {value: 'name', start: 0, end: 5},
                {value: 'description', start: 6, end: 11}
              ]
            },
            o.speechOutputForTest);

        const describedByTitle = root.children[2];
        o = new Output().withSpeech(
            cursors.Range.fromNode(describedByTitle), null, 'navigate');
        assertEqualsJSON(
            {
              string_: 'title|describedby',
              spans_: [
                {value: 'name', start: 0, end: 5},
                {value: 'description', start: 6, end: 17}
              ]
            },
            o.speechOutputForTest);

        const labelDescribedByTitle = root.children[3];
        o = new Output().withSpeech(
            cursors.Range.fromNode(labelDescribedByTitle), null, 'navigate');
        assertEqualsJSON(
            {
              string_: 'label|describedby',
              spans_: [
                {value: 'name', start: 0, end: 5},
                {value: 'description', start: 6, end: 17}
              ]
            },
            o.speechOutputForTest);

        // Hijack the 4th node to force tooltip to return a value. This can only
        // occur on ARC++ where tooltip gets set even if name and description
        // are both empty.
        const tooltip = root.children[4];
        Object.defineProperty(
            root.children[4], 'tooltip', {get: () => 'tooltip'});

        o = new Output().withSpeech(
            cursors.Range.fromNode(tooltip), null, 'navigate');
        assertEqualsJSON(
            {
              string_: 'tooltip',
              spans_: [{value: {'delay': true}, start: 0, end: 7}]
            },
            o.speechOutputForTest);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'InitialSpeechProperties', function() {
  this.runWithLoadedTree(
      `
    <p>test</p>  `,
      function(root) {
        // Capture speech properties sent to tts.
        this.currentProperties = [];
        ChromeVox.tts.speak = (textString, queueMode, properties) => {
          this.currentProperties.push(properties);
        };

        const o =
            new Output().withSpeech(cursors.Range.fromNode(root.firstChild));
        o.go();
        assertEqualsJSON([{category: TtsCategory.NAV}], this.currentProperties);
        this.currentProperties = [];

        o.withInitialSpeechProperties({
          phoneticCharacters: true,
          // This should not override existing value.
          category: TtsCategory.LIVE
        });
        o.go();
        assertEqualsJSON(
            [{phoneticCharacters: true, category: TtsCategory.NAV}],
            this.currentProperties);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'NameOrTextContent', function() {
  this.runWithLoadedTree(
      `
        <div tabindex=-1>
          <div aria-label="hello there world">
            <p>hello world</p>
          </div>
        </div>
      `,
      function(root) {
        const focusableDiv = root.firstChild;
        assertEquals(RoleType.GENERIC_CONTAINER, focusableDiv.role);
        assertEquals(
            chrome.automation.NameFromType.CONTENTS, focusableDiv.nameFrom);
        const o = new Output().withSpeech(cursors.Range.fromNode(focusableDiv));
        assertEquals('hello there world', o.speechOutputForTest.string_);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'AriaCurrentHint', function() {
  const site = `
      <div aria-current="page">Home</div>
      <div aria-current="false">About</div>
      `;
  this.runWithLoadedTree(site, function(root) {
    const currentDiv = root.firstChild;
    assertEquals(
        chrome.automation.AriaCurrentState.PAGE, currentDiv.ariaCurrentState);
    const o = new Output().withSpeech(
        cursors.Range.fromNode(currentDiv), null, 'navigate');
    assertEquals('Home|Current page', o.speechOutputForTest.string_);
  });
});

TEST_F('ChromeVoxOutputE2ETest', 'DelayHintVariants', function() {
  this.runWithLoadedTree(
      `
    <div aria-errormessage="error" aria-invalid="true">OK</div>
    <div id="error" aria-label="error"></div>
  `,
      function(root) {
        const div = root.children[0];
        const range = cursors.Range.fromNode(div);

        let o = new Output().withSpeech(range, null, 'navigate');
        assertEqualsJSON(
            {string_: 'OK|error', spans_: [{value: 'name', start: 3, end: 8}]},
            o.speechOutputForTest);

        // Force a few properties to be set so that hints are triggered.
        Object.defineProperty(div, 'clickable', {get: () => true});

        o = new Output().withSpeech(range, null, 'navigate');
        assertEqualsJSON(
            {
              string_: 'OK|error|Press Search+Space to activate',
              spans_: [
                {value: 'name', start: 3, end: 8},
                {value: {delay: true}, start: 9, end: 39}
              ]
            },
            o.speechOutputForTest);

        Object.defineProperty(div, 'placeholder', {get: () => 'placeholder'});
        o = new Output().withSpeech(range, null, 'navigate');
        assertEqualsJSON(
            {
              string_: 'OK|error|placeholder|Press Search+Space to activate',
              spans_: [
                {value: 'name', start: 3, end: 8},
                {value: {delay: true}, start: 9, end: 20}, {start: 21, end: 51}
              ]
            },
            o.speechOutputForTest);
      });
});

TEST_F('ChromeVoxOutputE2ETest', 'WithoutFocusRing', function() {
  const site = `<button></button>`;
  this.runWithLoadedTree(site, function(root) {
    let called = false;
    ChromeVoxState.instance.setFocusBounds = this.newCallback(() => {
      called = true;
    });

    const button = root.find({role: RoleType.BUTTON});

    // Triggers drawing of the focus ring.
    new Output().withSpeech(cursors.Range.fromNode(button)).go();
    assertTrue(called);
    called = false;

    // Does not trigger drawing of the focus ring.
    new Output()
        .withSpeech(cursors.Range.fromNode(button))
        .withoutFocusRing()
        .go();
    assertFalse(called);
  });
});

TEST_F('ChromeVoxOutputE2ETest', 'ARCCheckbox', function() {
  this.runWithLoadedTree('<input type="checkbox">', function(root) {
    const checkbox = root.firstChild.firstChild;

    Object.defineProperty(
        checkbox, 'checkedStateDescription',
        {get: () => 'checked state description'});
    const range = cursors.Range.fromNode(checkbox);
    const o = new Output().withoutHints().withSpeechAndBraille(
        range, null, 'navigate');
    checkSpeechOutput(
        '|Check box|checked state description',
        [
          {value: new OutputEarconAction('CHECK_OFF'), start: 0, end: 0},
          {value: 'role', start: 1, end: 10},
          {value: 'checkedStateDescription', start: 11, end: 36}
        ],
        o);
  });
});

TEST_F('ChromeVoxOutputE2ETest', 'ARCCustomAction', function() {
  this.runWithLoadedTree('<p>test</p>', function(root) {
    const actionable = root.firstChild.firstChild;
    Object.defineProperty(actionable, 'customActions', {
      get: () => [{id: 0, description: 'custom action description'}],
    });
    const range = cursors.Range.fromNode(actionable);
    const o = new Output().withSpeechAndBraille(range, null, 'navigate');
    checkSpeechOutput(
        'test|Actions available. Press Search+Ctrl+A to view',
        [
          {value: 'name', start: 0, end: 4},
          {value: {delay: true}, start: 5, end: 51}
        ],
        o);
  });
});

TEST_F('ChromeVoxOutputE2ETest', 'ContextOrder', function() {
  this.resetContextualOutput();
  this.runWithLoadedTree('<p>test</p><div role="menu">a</div>', function(root) {
    let o = new Output().withSpeech(cursors.Range.fromNode(root));
    assertEquals('last', o.contextOrder_);

    const p = root.find({role: RoleType.PARAGRAPH});
    const menu = root.find({role: RoleType.MENU});
    o = new Output().withSpeech(
        cursors.Range.fromNode(p), cursors.Range.fromNode(menu));
    assertEquals('last', o.contextOrder_);

    o = new Output().withSpeech(
        cursors.Range.fromNode(menu), cursors.Range.fromNode(p));
    assertEquals('first', o.contextOrder_);

    o = new Output().withSpeech(
        cursors.Range.fromNode(menu.firstChild), cursors.Range.fromNode(p));
    assertEquals('first', o.contextOrder_);
  });
});
