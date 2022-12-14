// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/scoped_feature_list.h"

#include "base/strings/utf_string_conversions.h"
#include "components/autofill/content/renderer/focus_test_utils.h"
#include "components/autofill/content/renderer/form_autofill_util.h"
#include "components/autofill/content/renderer/form_cache.h"
#include "components/autofill/content/renderer/form_cache_test_api.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/autofill/core/common/form_field_data.h"
#include "content/public/test/render_view_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_input_element.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_remote_frame.h"
#include "third_party/blink/public/web/web_select_element.h"

using base::ASCIIToUTF16;
using blink::WebDocument;
using blink::WebElement;
using blink::WebInputElement;
using blink::WebSelectElement;
using blink::WebString;
using testing::AllOf;
using testing::ElementsAre;
using testing::Field;

namespace autofill {

const FormData* GetFormByName(const std::vector<FormData>& forms,
                              base::StringPiece name) {
  for (const FormData& form : forms) {
    if (form.name == ASCIIToUTF16(name))
      return &form;
  }
  return nullptr;
}

FrameToken GetFrameToken(blink::WebElement iframe_element) {
  blink::WebFrame* frame =
      blink::WebFrame::FromFrameOwnerElement(iframe_element);
  if (frame && frame->IsWebLocalFrame()) {
    return LocalFrameToken(
        frame->ToWebLocalFrame()->GetLocalFrameToken().value());
  } else if (frame && frame->IsWebRemoteFrame()) {
    return RemoteFrameToken(
        frame->ToWebRemoteFrame()->GetRemoteFrameToken().value());
  } else {
    NOTREACHED();
    return FrameToken();
  }
}

class FormCacheBrowserTest : public content::RenderViewTest {
 public:
  FormCacheBrowserTest() {
    focus_test_utils_ = std::make_unique<test::FocusTestUtils>(
        base::BindRepeating(&FormCacheBrowserTest::ExecuteJavaScriptForTests,
                            base::Unretained(this)));
  }
  ~FormCacheBrowserTest() override = default;
  FormCacheBrowserTest(const FormCacheBrowserTest&) = delete;
  FormCacheBrowserTest& operator=(const FormCacheBrowserTest&) = delete;

 protected:
  std::string GetFocusLog() {
    return focus_test_utils_->GetFocusLog(GetMainFrame()->GetDocument());
  }

  std::unique_ptr<test::FocusTestUtils> focus_test_utils_;
};

class ParameterizedFormCacheBrowserTest
    : public FormCacheBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  ParameterizedFormCacheBrowserTest() {
    bool use_new_form_extraction = GetParam();
    std::vector<base::Feature> enabled;
    std::vector<base::Feature> disabled;
    (use_new_form_extraction ? &enabled : &disabled)
        ->push_back(features::kAutofillUseNewFormExtraction);
    scoped_features_.InitWithFeatures(enabled, disabled);
  }

 private:
  base::test::ScopedFeatureList scoped_features_;
};

TEST_P(ParameterizedFormCacheBrowserTest, ExtractForms) {
  LoadHTML(R"(
    <form id="form1">
      <input type="text" name="foo1">
      <input type="text" name="foo2">
      <input type="text" name="foo3">
    </form>
    <input type="text" name="unowned_element">
  )");

  FormCache form_cache(GetMainFrame());
  std::vector<FormData> forms = form_cache.ExtractNewForms(nullptr);

  EXPECT_EQ(forms.size(), 2u);

  const FormData* form1 = GetFormByName(forms, "form1");
  ASSERT_TRUE(form1);
  EXPECT_EQ(3u, form1->fields.size());
  EXPECT_TRUE(form1->child_frames.empty());

  const FormData* unowned_form = GetFormByName(forms, "");
  ASSERT_TRUE(unowned_form);
  EXPECT_EQ(1u, unowned_form->fields.size());
  EXPECT_TRUE(unowned_form->child_frames.empty());
}

class FormCacheIframeBrowserTest : public ParameterizedFormCacheBrowserTest {
 public:
  FormCacheIframeBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kAutofillAcrossIframes);
  }
  ~FormCacheIframeBrowserTest() override = default;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_P(FormCacheIframeBrowserTest, ExtractFrames) {
  LoadHTML(R"(
    <form id="form1">
      <iframe id="frame1"></iframe>
    </form>
    <iframe id="frame2"></iframe>
  )");

  FrameToken frame1_token =
      GetFrameToken(GetMainFrame()->GetDocument().GetElementById("frame1"));
  FrameToken frame2_token =
      GetFrameToken(GetMainFrame()->GetDocument().GetElementById("frame2"));

  FormCache form_cache(GetMainFrame());
  std::vector<FormData> forms = form_cache.ExtractNewForms(nullptr);

  EXPECT_EQ(forms.size(), 2u);

  const FormData* form1 = GetFormByName(forms, "form1");
  ASSERT_TRUE(form1);
  EXPECT_TRUE(form1->fields.empty());
  EXPECT_THAT(
      form1->child_frames,
      ElementsAre(AllOf(Field(&FrameTokenWithPredecessor::token, frame1_token),
                        Field(&FrameTokenWithPredecessor::predecessor, -1))));

  const FormData* unowned_form = GetFormByName(forms, "");
  ASSERT_TRUE(unowned_form);
  EXPECT_TRUE(unowned_form->fields.empty());
  EXPECT_THAT(
      unowned_form->child_frames,
      ElementsAre(AllOf(Field(&FrameTokenWithPredecessor::token, frame2_token),
                        Field(&FrameTokenWithPredecessor::predecessor, -1))));
}

TEST_P(ParameterizedFormCacheBrowserTest, ExtractFormsTwice) {
  LoadHTML(R"(
    <form id="form1">
      <input type="text" name="foo1">
      <input type="text" name="foo2">
      <input type="text" name="foo3">
    </form>
    <input type="text" name="unowned_element">
  )");

  FormCache form_cache(GetMainFrame());
  std::vector<FormData> forms = form_cache.ExtractNewForms(nullptr);
  EXPECT_EQ(forms.size(), 2u);

  forms = form_cache.ExtractNewForms(nullptr);
  // As nothing has changed, there are no new forms and |forms| should be empty.
  EXPECT_TRUE(forms.empty());
}

TEST_P(FormCacheIframeBrowserTest, ExtractFramesTwice) {
  LoadHTML(R"(
    <form id="form1">
      <iframe></iframe>
    </form>
    <iframe></iframe>
  )");

  FormCache form_cache(GetMainFrame());
  std::vector<FormData> forms = form_cache.ExtractNewForms(nullptr);
  EXPECT_EQ(forms.size(), 2u);

  forms = form_cache.ExtractNewForms(nullptr);
  // As nothing has changed, there are no new forms and |forms| should be empty.
  EXPECT_TRUE(forms.empty());
}

TEST_P(FormCacheIframeBrowserTest, ExtractFramesAfterVisibilityChange) {
  LoadHTML(R"(
    <form id="form1">
      <iframe id="frame1" style="display: none;"></iframe>
      <iframe id="frame2" style="display: none;"></iframe>
    </form>
    <iframe id="frame3" style="display: none;"></iframe>
  )");

  WebElement iframe1 = GetMainFrame()->GetDocument().GetElementById("frame1");
  WebElement iframe2 = GetMainFrame()->GetDocument().GetElementById("frame2");
  WebElement iframe3 = GetMainFrame()->GetDocument().GetElementById("frame3");

  auto GetSize = [](const WebElement& element) {
    gfx::Rect bounds = element.BoundsInViewport();
    return bounds.width() * bounds.height();
  };

  ASSERT_LE(GetSize(iframe1), 0);
  ASSERT_LE(GetSize(iframe2), 0);
  ASSERT_LE(GetSize(iframe3), 0);

  FormCache form_cache(GetMainFrame());
  std::vector<FormData> forms = form_cache.ExtractNewForms(nullptr);
  EXPECT_EQ(forms.size(), 0u);

  iframe1.SetAttribute("style", "display: block;");
  iframe2.SetAttribute("style", "display: block;");
  iframe3.SetAttribute("style", "display: block;");

  ASSERT_GT(GetSize(iframe1), 0);
  ASSERT_GT(GetSize(iframe2), 0);
  ASSERT_GT(GetSize(iframe3), 0);

  forms = form_cache.ExtractNewForms(nullptr);
  EXPECT_EQ(forms.size(), 2u);

  iframe2.SetAttribute("style", "display: none;");
  iframe3.SetAttribute("style", "display: none;");

  ASSERT_GT(GetSize(iframe1), 0);
  ASSERT_LE(GetSize(iframe2), 0);
  ASSERT_LE(GetSize(iframe3), 0);

  // TODO(crbug/896689#c14) It wouldn't hurt if both forms (the synthetic form
  // as well as form1) were updated.
  forms = form_cache.ExtractNewForms(nullptr);
  EXPECT_EQ(forms.size(), 1u);
  EXPECT_EQ(forms.front().name, u"form1");
}

TEST_P(ParameterizedFormCacheBrowserTest, ExtractFormsAfterModification) {
  LoadHTML(R"(
    <form id="form1">
      <input type="text" name="foo1">
      <input type="text" name="foo2">
      <input type="text" name="foo3">
    </form>
    <input type="text" name="unowned_element">
  )");

  FormCache form_cache(GetMainFrame());
  std::vector<FormData> forms = form_cache.ExtractNewForms(nullptr);

  // Append an input element to the form and to the list of unowned inputs.
  ExecuteJavaScriptForTests(R"(
    var new_input_1 = document.createElement("input");
    new_input_1.setAttribute("type", "text");
    new_input_1.setAttribute("name", "foo4");

    var form1 = document.getElementById("form1");
    form1.appendChild(new_input_1);

    var new_input_2 = document.createElement("input");
    new_input_2.setAttribute("type", "text");
    new_input_2.setAttribute("name", "unowned_element_2");
    document.body.appendChild(new_input_2);
  )");

  forms = form_cache.ExtractNewForms(nullptr);

  const FormData* form1 = GetFormByName(forms, "form1");
  ASSERT_TRUE(form1);
  EXPECT_EQ(4u, form1->fields.size());

  const FormData* unowned_form = GetFormByName(forms, "");
  ASSERT_TRUE(unowned_form);
  EXPECT_EQ(2u, unowned_form->fields.size());
}

TEST_P(ParameterizedFormCacheBrowserTest, FillAndClear) {
  LoadHTML(R"(
    <input type="text" name="text" id="text">
    <input type="checkbox" checked name="checkbox" id="checkbox">
    <select name="select" id="select">
      <option value="first">first</option>
      <option value="second" selected>second</option>
    </select>
  )");

  FormCache form_cache(GetMainFrame());
  std::vector<FormData> forms = form_cache.ExtractNewForms(nullptr);

  ASSERT_EQ(1u, forms.size());
  FormData values_to_fill = forms[0];
  values_to_fill.fields[0].value = u"test";
  values_to_fill.fields[0].is_autofilled = true;
  values_to_fill.fields[1].check_status =
      FormFieldData::CheckStatus::kCheckableButUnchecked;
  values_to_fill.fields[1].is_autofilled = true;
  values_to_fill.fields[2].value = u"first";
  values_to_fill.fields[2].is_autofilled = true;

  WebDocument doc = GetMainFrame()->GetDocument();
  auto text = doc.GetElementById("text").To<WebInputElement>();
  auto checkbox = doc.GetElementById("checkbox").To<WebInputElement>();
  auto select_element = doc.GetElementById("select").To<WebSelectElement>();

  form_util::FillOrPreviewForm(values_to_fill, text,
                               mojom::RendererFormDataAction::kFill);

  EXPECT_EQ("test", text.Value().Ascii());
  EXPECT_FALSE(checkbox.IsChecked());
  EXPECT_EQ("first", select_element.Value().Ascii());

  // Validate that clearing works, in particular that the previous values
  // were saved correctly.
  form_cache.ClearSectionWithElement(text);

  EXPECT_EQ("", text.Value().Ascii());
  EXPECT_TRUE(checkbox.IsChecked());
  EXPECT_EQ("second", select_element.Value().Ascii());
}

// Tests that correct focus, change and blur events are emitted during the
// autofilling and clearing of the form with an initially focused element.
TEST_P(ParameterizedFormCacheBrowserTest,
       VerifyFocusAndBlurEventsAfterAutofillAndClearingWithFocusElement) {
  // Load a form.
  LoadHTML(
      "<html><form id='myForm'>"
      "<label>First Name:</label><input id='fname' name='0'/><br/>"
      "<label>Last Name:</label> <input id='lname' name='1'/><br/>"
      "</form></html>");

  focus_test_utils_->SetUpFocusLogging();
  focus_test_utils_->FocusElement("fname");

  FormCache form_cache(GetMainFrame());
  std::vector<FormData> forms = form_cache.ExtractNewForms(nullptr);

  ASSERT_EQ(2u, forms.size());
  FormData values_to_fill = forms[0];
  values_to_fill.fields[0].value = u"John";
  values_to_fill.fields[0].is_autofilled = true;
  values_to_fill.fields[1].value = u"Smith";
  values_to_fill.fields[1].is_autofilled = true;

  auto fname = GetMainFrame()
                   ->GetDocument()
                   .GetElementById("fname")
                   .To<WebInputElement>();

  // Simulate filling the form using Autofill.
  form_util::FillOrPreviewForm(values_to_fill, fname,
                               mojom::RendererFormDataAction::kFill);

  // Simulate clearing the form.
  form_cache.ClearSectionWithElement(fname);

  // Expected Result in order:
  // - from filling
  //  * Change fname
  //  * Blur fname
  //  * Focus lname
  //  * Change lname
  //  * Blur lname
  //  * Focus fname
  // - from clearing
  //  * Change fname
  //  * Blur fname
  //  * Focus lname
  //  * Change lname
  //  * Blur lname
  //  * Focus fname
  EXPECT_EQ(GetFocusLog(), "c0b0f1c1b1f0c0b0f1c1b1f0");
}

TEST_P(ParameterizedFormCacheBrowserTest, FreeDataOnElementRemoval) {
  LoadHTML(R"(
    <div id="container">
      <input type="text" name="text" id="text">
      <input type="checkbox" checked name="checkbox" id="checkbox">
      <select name="select" id="select">
        <option value="first">first</option>
        <option value="second" selected>second</option>
      </select>
    </div>
  )");

  FormCache form_cache(GetMainFrame());
  form_cache.ExtractNewForms(nullptr);

  EXPECT_EQ(1u, FormCacheTestApi(&form_cache).initial_select_values_size());
  EXPECT_EQ(1u, FormCacheTestApi(&form_cache).initial_checked_state_size());

  ExecuteJavaScriptForTests(R"(
    const container = document.getElementById('container');
    while (container.childElementCount > 0) {
      container.removeChild(container.children.item(0));
    }
  )");

  std::vector<FormData> forms = form_cache.ExtractNewForms(nullptr);
  EXPECT_EQ(0u, forms.size());
  EXPECT_EQ(0u, FormCacheTestApi(&form_cache).initial_select_values_size());
  EXPECT_EQ(0u, FormCacheTestApi(&form_cache).initial_checked_state_size());
}

// Test that the select element's user edited field state is set
// to false after clearing the form.
TEST_P(ParameterizedFormCacheBrowserTest,
       ClearFormSelectElementEditedStateReset) {
  LoadHTML(R"(
    <input type="text" name="text" id="text">
    <select name="date" id="date">
      <option value="first">first</option>
      <option value="second" selected>second</option>
      <option value="third">third</option>
    </select>
    <select name="month" id="month">
      <option value="january">january</option>
      <option value="february">february</option>
      <option value="march" selected>march</option>
    </select>
  )");

  FormCache form_cache(GetMainFrame());
  std::vector<FormData> forms = form_cache.ExtractNewForms(nullptr);

  ASSERT_EQ(1u, forms.size());
  FormData values_to_fill = forms[0];
  values_to_fill.fields[0].value = u"test";
  values_to_fill.fields[0].is_autofilled = true;
  values_to_fill.fields[1].value = u"first";
  values_to_fill.fields[1].is_autofilled = true;
  values_to_fill.fields[2].value = u"january";
  values_to_fill.fields[2].is_autofilled = true;

  WebDocument doc = GetMainFrame()->GetDocument();
  auto text = doc.GetElementById("text").To<WebInputElement>();
  auto select_date = doc.GetElementById("date").To<WebSelectElement>();
  auto select_month = doc.GetElementById("month").To<WebSelectElement>();

  form_util::FillOrPreviewForm(values_to_fill, text,
                               mojom::RendererFormDataAction::kFill);

  EXPECT_EQ("test", text.Value().Ascii());
  EXPECT_EQ("first", select_date.Value().Ascii());
  EXPECT_EQ("january", select_month.Value().Ascii());

  // Expect that the 'user has edited field' state is set
  EXPECT_TRUE(select_date.UserHasEditedTheField());
  EXPECT_TRUE(select_month.UserHasEditedTheField());

  // Clear form
  form_cache.ClearSectionWithElement(text);

  // Expect that the state is now cleared
  EXPECT_FALSE(select_date.UserHasEditedTheField());
  EXPECT_FALSE(select_month.UserHasEditedTheField());

  // Fill the form again, this time the select elements are being filled
  // with different values just for additional check.
  values_to_fill.fields[1].value = u"third";
  values_to_fill.fields[1].is_autofilled = true;
  values_to_fill.fields[2].value = u"february";
  values_to_fill.fields[2].is_autofilled = true;
  form_util::FillOrPreviewForm(values_to_fill, text,
                               mojom::RendererFormDataAction::kFill);

  // Ensure the form is filled correctly, including the select elements.
  EXPECT_EQ("test", text.Value().Ascii());
  EXPECT_EQ("third", select_date.Value().Ascii());
  EXPECT_EQ("february", select_month.Value().Ascii());

  // Expect that the state is set again
  EXPECT_TRUE(select_date.UserHasEditedTheField());
  EXPECT_TRUE(select_month.UserHasEditedTheField());
}

TEST_P(ParameterizedFormCacheBrowserTest,
       IsFormElementEligibleForManualFilling) {
  // Load a form.
  LoadHTML(
      "<html><form id='myForm'>"
      "<label>First Name:</label><input id='fname' name='0'/><br/>"
      "<label>Middle Name:</label> <input id='mname' name='1'/><br/>"
      "<label>Last Name:</label> <input id='lname' name='2'/><br/>"
      "</form></html>");

  WebDocument doc = GetMainFrame()->GetDocument();
  auto first_name_element = doc.GetElementById("fname").To<WebInputElement>();
  auto middle_name_element = doc.GetElementById("mname").To<WebInputElement>();
  auto last_name_element = doc.GetElementById("lname").To<WebInputElement>();

  FormCache form_cache(GetMainFrame());
  std::vector<FormData> forms =
      form_cache.ExtractNewForms(/*field_data_manager=*/nullptr);
  const FormData* form_data = GetFormByName(forms, "myForm");
  EXPECT_EQ(3u, form_data->fields.size());

  // Set the first_name and last_name fields as eligible for manual filling.
  std::vector<FieldRendererId> fields_eligible_for_manual_filling;
  fields_eligible_for_manual_filling.push_back(
      form_data->fields[0].unique_renderer_id);
  fields_eligible_for_manual_filling.push_back(
      form_data->fields[2].unique_renderer_id);
  form_cache.SetFieldsEligibleForManualFilling(
      fields_eligible_for_manual_filling);

  EXPECT_TRUE(FormCacheTestApi(&form_cache)
                  .IsFormElementEligibleForManualFilling(first_name_element));
  EXPECT_FALSE(FormCacheTestApi(&form_cache)
                   .IsFormElementEligibleForManualFilling(middle_name_element));
  EXPECT_TRUE(FormCacheTestApi(&form_cache)
                  .IsFormElementEligibleForManualFilling(last_name_element));
}

// Test that after adding an input element to an already extracted non-synthetic
// form, the form (has the same rendererId) is not added twice to the extracted
// forms.
TEST_P(ParameterizedFormCacheBrowserTest,
       RemoveReextractedModifiedNonSyntheticFormsWithSameRendererID) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      features::kAutofillUseOnlyFormRendererIDForOldDuplicateFormRemoval);

  LoadHTML(R"(
    <form id="form1">
      <input type="text">
    </form>
  )");

  FormCache form_cache(GetMainFrame());
  std::vector<FormData> forms = form_cache.ExtractNewForms(nullptr);

  // Append an input element to the form.
  ExecuteJavaScriptForTests(R"(
    var form1 = document.getElementById("form1");
    form1.appendChild(document.createElement("input"));
  )");

  forms = form_cache.ExtractNewForms(nullptr);

  // Check if a field was truly added to the form.
  const FormData* form1 = GetFormByName(forms, "form1");
  ASSERT_TRUE(form1);
  EXPECT_EQ(2u, form1->fields.size());

  // Check if the modified form with the same rendererId was not added again.
  if (base::FeatureList::IsEnabled(features::kAutofillUseNewFormExtraction)) {
    EXPECT_EQ(1u, FormCacheTestApi(&form_cache).parsed_forms_rendererid_size());
  } else {
    EXPECT_EQ(1u, FormCacheTestApi(&form_cache).parsed_forms_size());
  }
}

// Test that after adding an unowned input element to an already extracted
// synthetic form, the form (has the same rendererId) is not added twice to the
// extracted forms.
TEST_P(ParameterizedFormCacheBrowserTest,
       RemoveReextractedModifiedSyntheticFormsWithSameRendererID) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      features::kAutofillUseOnlyFormRendererIDForOldDuplicateFormRemoval);

  LoadHTML(R"(
    <input type="text" name="unowned_element">
  )");

  FormCache form_cache(GetMainFrame());
  std::vector<FormData> forms = form_cache.ExtractNewForms(nullptr);

  // Append the document with a new unowned input.
  ExecuteJavaScriptForTests(R"(
    var new_unowned_input = document.createElement("input");
    document.body.appendChild(new_unowned_input);
  )");

  forms = form_cache.ExtractNewForms(nullptr);

  // Check if the unowned field was truly added.
  const FormData* unowned_form = GetFormByName(forms, "");
  ASSERT_TRUE(unowned_form);
  EXPECT_EQ(2u, unowned_form->fields.size());

  // Check if the modified form with the same rendererId was not added again.
  // (We expect that all the unowned fields have the same rendererId.)
  if (base::FeatureList::IsEnabled(features::kAutofillUseNewFormExtraction)) {
    EXPECT_EQ(1u, FormCacheTestApi(&form_cache).parsed_forms_rendererid_size());
  } else {
    EXPECT_EQ(1u, FormCacheTestApi(&form_cache).parsed_forms_size());
  }
}

// Test that the FormCache does not contain empty forms.
TEST_F(FormCacheBrowserTest, DoNotStoreEmptyForms) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      features::kAutofillUseNewFormExtraction);

  LoadHTML(R"(<form></form>)");

  FormCache form_cache(GetMainFrame());
  form_cache.ExtractNewForms(nullptr);

  EXPECT_EQ(1u, GetMainFrame()->GetDocument().Forms().size());
  EXPECT_EQ(0u, FormCacheTestApi(&form_cache).parsed_forms_rendererid_size());
}

// Test that the FormCache never contains more than |kMaxParseableFields|
// non-empty parsed forms.
TEST_F(FormCacheBrowserTest, FormCacheSizeUpperBound) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      features::kAutofillUseNewFormExtraction);

  // Create a HTML page that contains `kMaxParseableFields + 1` non-empty
  // forms.
  std::string html;
  for (unsigned int i = 0; i < kMaxParseableFields + 1; i++) {
    html += "<form><input></form>";
  }
  LoadHTML(html.c_str());

  FormCache form_cache(GetMainFrame());
  form_cache.ExtractNewForms(nullptr);

  EXPECT_EQ(kMaxParseableFields + 1,
            GetMainFrame()->GetDocument().Forms().size());
  EXPECT_EQ(kMaxParseableFields,
            FormCacheTestApi(&form_cache).parsed_forms_rendererid_size());
}

INSTANTIATE_TEST_SUITE_P(All,
                         ParameterizedFormCacheBrowserTest,
                         testing::Bool());
INSTANTIATE_TEST_SUITE_P(All, FormCacheIframeBrowserTest, testing::Bool());

}  // namespace autofill
