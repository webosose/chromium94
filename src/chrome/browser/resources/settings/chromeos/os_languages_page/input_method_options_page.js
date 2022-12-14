// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-input-method-options-page' is the settings sub-page
 * to allow users to change options for each input method.
 */
Polymer({
  is: 'settings-input-method-options-page',

  behaviors: [
    I18nBehavior,
    PrefsBehavior,
    settings.RouteObserverBehavior,
  ],

  properties: {
    /** @type {!LanguageHelper} */
    languageHelper: Object,

    /** Preferences state. */
    prefs: {
      type: Object,
      notify: true,
    },

    /**
     * Input method ID.
     * @private
     */
    id_: String,

    /**
     * Input method engine ID.
     * @private
     */
    engineId_: String,

    /**
     * The content to be displayed in the page, auto generated every time when
     * the user enters the page.
     * @private {!Array<!{title: string, options:!Array<!Object<string, *>>}>}
     */
    optionSections_: {
      type: Array,
      value: [],
    },
  },

  /**
   * The root path of input method options in Prefs.
   * @const {string}
   * @private
   */
  PREFS_PATH: 'settings.language.input_method_specific_settings',

  /**
   * settings.RouteObserverBehavior
   * @param {!settings.Route} route
   * @param {!settings.Route} oldRoute
   * @protected
   */
  currentRouteChanged(route, oldRoute) {
    if (route !== settings.routes.OS_LANGUAGES_INPUT_METHOD_OPTIONS) {
      this.id_ = '';
      this.parentNode.pageTitle = '';
      this.optionSections_ = [];
      return;
    }

    const queryParams = settings.Router.getInstance().getQueryParameters();
    this.id_ = queryParams.get('id') || '';
    this.parentNode.pageTitle =
        this.languageHelper.getInputMethodDisplayName(this.id_);
    this.engineId_ =
        settings.input_method_util.getFirstPartyInputMethodEngineId(this.id_);
    this.populateOptionSections_();
  },

  /**
   * Get menu items for an option, and enrich the items with selected status and
   * i18n label.
   * @param {settings.input_method_util.OptionType} name
   * @param {*} value
   */
  getMenuItems(name, value) {
    return settings.input_method_util.getOptionMenuItems(name).map(menuItem => {
      menuItem['selected'] = menuItem['value'] === value;
      menuItem['label'] =
          menuItem['name'] ? this.i18n(menuItem['name']) : menuItem['value'];
      return menuItem;
    });
  },

  /**
   * Generate the sections of options according to the engine ID and Prefs.
   * @private
   */
  populateOptionSections_() {
    const options = settings.input_method_util.generateOptions(this.engineId_);

    const prefValue = this.getPref(this.PREFS_PATH).value;
    const prefix = this.getPrefsPrefix_();
    const currentSettings = prefix in prefValue ? prefValue[prefix] : {};

    const makeOption = (option) => {
      const name = option.name;
      const uiType = settings.input_method_util.getOptionUiType(name);
      const value = name in currentSettings ?
          currentSettings[name] :
          settings.input_method_util.OPTION_DEFAULT[name];
      const label = settings.input_method_util.isOptionLabelTranslated(name) ?
          this.i18n(settings.input_method_util.getOptionLabelName(name)) :
          settings.input_method_util.getUntranslatedOptionLabelName(name);
      return {
        name: name,
        uiType: uiType,
        value: value,
        label: label,
        menuItems: this.getMenuItems(name, value),
        url: settings.input_method_util.getOptionUrl(name),
        dependentOptions: option.dependentOptions ?
            option.dependentOptions.map(t => makeOption({name: t})) :
            []
      };
    };

    // If there is no option name in a section, this section, including the
    // section title, should not be displayed.
    this.optionSections_ =
        options.filter(section => section.optionNames.length > 0)
            .map(section => {
              return {
                title: this.getSectionTitleI18n_(section.title),
                options: section.optionNames.map(makeOption, false),
              };
            });
  },

  /**
   * @return {string} Prefs prefix for the current engine ID, which is usually
   *     just the engine ID itself, but pinyin and zhuyin are special for legacy
   *     compatibility reason.
   * @private
   */
  getPrefsPrefix_() {
    if (this.engineId_ === 'zh-t-i0-pinyin') {
      return 'pinyin';
    } else if (this.engineId_ === 'zh-hant-t-i0-und') {
      return 'zhuyin';
    }
    return this.engineId_;
  },

  /**
   *
   * @param {*} value
   * @private
   */
  dependentOptionsDisabled_(value) {
    // TODO(b/189909728): Sometimes the value comes as a string, other times as
    // an integer, so handle both cases. Try to understand and fix this.
    return value === '0' || value === 0;
  },

  /**
   * Handler for toggle button and dropdown change. Update the value of the
   * changing option in Cros prefs.
   * @param {!{model: !{option: Object}}} e
   * @private
   */
  onToggleButtonOrDropdownChange_(e) {
    // Get the existing settings dictionary, in order to update it later.
    // |PrefsBehavior.setPrefValue| will update Cros Prefs only if the reference
    // of variable has changed, so we need to copy the current content into a
    // new variable.
    const updatedSettings = {};
    Object.assign(updatedSettings, this.getPref(this.PREFS_PATH)['value']);
    const prefix = this.getPrefsPrefix_();
    if (!(prefix in updatedSettings)) {
      updatedSettings[prefix] = {};
    }
    // e.model isn't correctly set for dependent options, due to nested
    // dom-repeat, so figure out what option was actually set.
    const option = e.model.dependant ? e.model.dependant : e.model.option;
    // The value of dropdown is not updated immediately when the event is fired.
    // Wait for the polymer state to update to make sure we write the latest
    // to Cros Prefs.
    Polymer.RenderStatus.afterNextRender(this, () => {
      let newValue = option.value;
      // The value of dropdown in html is always string, but some of the prefs
      // values are used as integer or enum by IME, so we need to store numbers
      // for them to function correctly.
      if (settings.input_method_util.isNumberValue(option.name)) {
        newValue = parseInt(newValue, 10);
      }
      updatedSettings[prefix][option.name] = newValue;
      this.setPrefValue(this.PREFS_PATH, updatedSettings);
    });
  },

  /**
   * Opens external link in Chrome.
   * @param {!{model: !{option: !{url: !settings.Route}}}} e
   * @private
   */
  navigateToOtherPageInSettings_(e) {
    settings.Router.getInstance().navigateTo(e.model.option.url);
  },

  /**
   * @param {string} section the name of the section.
   * @return {string} the i18n string for the section title.
   * @private
   */
  getSectionTitleI18n_(section) {
    switch (section) {
      case 'basic':
        return this.i18n('inputMethodOptionsBasicSectionTitle');
      case 'advanced':
        return this.i18n('inputMethodOptionsAdvancedSectionTitle');
      case 'physicalKeyboard':
        return this.i18n('inputMethodOptionsPhysicalKeyboardSectionTitle');
      case 'virtualKeyboard':
        return this.i18n('inputMethodOptionsVirtualKeyboardSectionTitle');
      default:
        assertNotReached();
    }
  },

  /**
   * @param {!settings.input_method_util.UiType} item
   * @return {boolean} true if |item| is a toggle button.
   * @private
   */
  isToggleButton_(item) {
    return item === settings.input_method_util.UiType.TOGGLE_BUTTON;
  },

  /**
   * @param {!settings.input_method_util.UiType} item
   * @return {boolean} true if |item| is a dropdown.
   * @private
   */
  isDropdown_(item) {
    return item === settings.input_method_util.UiType.DROPDOWN;
  },

  /**
   * @param {!settings.input_method_util.UiType} item
   * @return {boolean} true if |item| is an external link.
   * @private
   */
  isLink_(item) {
    return item === settings.input_method_util.UiType.LINK;
  },
});
