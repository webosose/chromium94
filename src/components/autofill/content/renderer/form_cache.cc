// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/form_cache.h"

#include <algorithm>
#include <set>
#include <string>
#include <utility>

#include "base/check_op.h"
#include "base/containers/contains.h"
#include "base/containers/cxx20_erase.h"
#include "base/feature_list.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/content/renderer/form_autofill_util.h"
#include "components/autofill/content/renderer/page_form_analyser_logger.h"
#include "components/autofill/core/common/autofill_constants.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/autofill/core/common/form_data_predictions.h"
#include "components/strings/grit/components_strings.h"
#include "third_party/blink/public/common/metrics/form_element_pii_type.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_form_control_element.h"
#include "third_party/blink/public/web/web_form_element.h"
#include "third_party/blink/public/web/web_input_element.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_select_element.h"
#include "ui/base/l10n/l10n_util.h"

using blink::WebAutofillState;
using blink::WebConsoleMessage;
using blink::WebDocument;
using blink::WebElement;
using blink::WebFormControlElement;
using blink::WebFormElement;
using blink::WebInputElement;
using blink::WebLocalFrame;
using blink::WebNode;
using blink::WebSelectElement;
using blink::WebString;
using blink::WebVector;

namespace autofill {

namespace {

blink::FormElementPiiType MapTypePredictionToFormElementPiiType(
    base::StringPiece type) {
  if (type == "NO_SERVER_DATA" || type == "UNKNOWN_TYPE" ||
      type == "EMPTY_TYPE" || type == "") {
    return blink::FormElementPiiType::kUnknown;
  }

  if (base::StartsWith(type, "EMAIL_"))
    return blink::FormElementPiiType::kEmail;
  if (base::StartsWith(type, "PHONE_"))
    return blink::FormElementPiiType::kPhone;
  return blink::FormElementPiiType::kOthers;
}

void LogDeprecationMessages(const WebFormControlElement& element) {
  std::string autocomplete_attribute =
      element.GetAttribute("autocomplete").Utf8();

  static const char* const deprecated[] = {"region", "locality"};
  for (const char* str : deprecated) {
    if (autocomplete_attribute.find(str) == std::string::npos)
      continue;
    std::string msg = base::StrCat(
        {"autocomplete='", str,
         "' is deprecated and will soon be ignored. See http://goo.gl/YjeSsW"});
    WebConsoleMessage console_message = WebConsoleMessage(
        blink::mojom::ConsoleMessageLevel::kWarning, WebString::FromASCII(msg));
    element.GetDocument().GetFrame()->AddMessageToConsole(console_message);
  }
}

// Determines whether the form is interesting enough to be sent to the browser
// for further operations.
bool IsFormInteresting(const FormData& form, size_t num_editable_elements) {
  if (form.fields.empty() && form.child_frames.empty())
    return false;

  // If the form has at least one field with an autocomplete attribute, it is a
  // candidate for autofill.
  bool all_fields_are_passwords = true;
  for (const FormFieldData& field : form.fields) {
    if (!field.autocomplete_attribute.empty())
      return true;
    if (field.form_control_type != "password")
      all_fields_are_passwords = false;
  }

  // If there are no autocomplete attributes, the form needs to have at least
  // the required number of editable fields for the prediction routines to be a
  // candidate for autofill.
  return num_editable_elements >= kMinRequiredFieldsForHeuristics ||
         num_editable_elements >= kMinRequiredFieldsForQuery ||
         num_editable_elements >= kMinRequiredFieldsForUpload ||
         (all_fields_are_passwords &&
          num_editable_elements >=
              kRequiredFieldsForFormsWithOnlyPasswordFields) ||
         form.child_frames.size() > 0;
}

}  // namespace

FormCache::FormCache(WebLocalFrame* frame) : frame_(frame) {}
FormCache::~FormCache() = default;

struct FormCache::CachedFormData {
  explicit CachedFormData(const FormData& form)
      : child_frames(form.child_frames) {
    for (const auto& field : form.fields)
      field_renderer_ids.push_back(field.unique_renderer_id);
  }
  CachedFormData(CachedFormData&& cached_form) = default;
  CachedFormData& operator=(CachedFormData&& cached_form) = default;
  ~CachedFormData() = default;

  std::vector<FieldRendererId> field_renderer_ids;
  std::vector<FrameTokenWithPredecessor> child_frames;
};

std::vector<FormData> FormCache::ModifiedExtractNewForms(
    const FieldDataManager* field_data_manager) {
  std::vector<FormData> forms;
  WebDocument document = frame_->GetDocument();
  if (document.IsNull())
    return forms;

  initial_checked_state_.clear();
  initial_select_values_.clear();

  std::set<FieldRendererId> observed_unique_renderer_ids;

  // Log an error message for deprecated attributes, but only the first time
  // the form is parsed.
  bool log_deprecation_messages = parsed_forms_rendererid_.empty();

  std::map<FormRendererId, CachedFormData> old_parsed_forms =
      std::move(parsed_forms_rendererid_);
  parsed_forms_rendererid_.clear();

  size_t num_fields_seen = 0;
  size_t num_frames_seen = 0;
  std::vector<WebFormControlElement> control_elements;

  // Helper function that stores new autofillable forms. Returns false if the
  // number of fields or frames would be too much if we extracted the new form.
  auto ProcessForm = [&](FormData form) {
    for (const auto& field : form.fields)
      observed_unique_renderer_ids.insert(field.unique_renderer_id);

    num_fields_seen += form.fields.size();
    num_frames_seen += form.child_frames.size();
    if (num_fields_seen > kMaxParseableFields ||
        num_frames_seen > kMaxParseableFrames) {
      // Restore |parsed_forms_rendererid_|.
      parsed_forms_rendererid_.insert(
          std::make_move_iterator(old_parsed_forms.begin()),
          std::make_move_iterator(old_parsed_forms.end()));
      PruneInitialValueCaches(observed_unique_renderer_ids);
      return false;
    }

    size_t num_editable_elements =
        ScanFormControlElements(control_elements, log_deprecation_messages);

    // Store only "interesting" forms.
    if (IsFormInteresting(form, num_editable_elements)) {
      DCHECK(parsed_forms_rendererid_.find(form.unique_renderer_id) ==
             parsed_forms_rendererid_.end());
      parsed_forms_rendererid_.insert(
          {form.unique_renderer_id, CachedFormData(form)});
      // If it is a new form or an input field of the form changed,
      // re-extract the form.
      auto it = old_parsed_forms.find(form.unique_renderer_id);
      if (it == old_parsed_forms.end() ||
          form.child_frames != it->second.child_frames ||
          !base::ranges::equal(form.fields, it->second.field_renderer_ids, {},
                               &FormFieldData::unique_renderer_id)) {
        SaveInitialValues(control_elements);
        forms.push_back(std::move(form));
      }
    }
    return true;
  };

  constexpr form_util::ExtractMask extract_mask =
      static_cast<form_util::ExtractMask>(form_util::EXTRACT_VALUE |
                                          form_util::EXTRACT_OPTIONS);

  for (const WebFormElement& form_element : document.Forms()) {
    control_elements =
        form_util::ExtractAutofillableElementsInForm(form_element);

    FormData form;
    if (!WebFormElementToFormData(form_element, WebFormControlElement(),
                                  field_data_manager, extract_mask, &form,
                                  nullptr)) {
      continue;
    }
    if (!ProcessForm(std::move(form)))
      return forms;
  }

  // Look for more parseable fields outside of forms. Create a synthetic form
  // from them.
  std::vector<WebElement> fieldsets;
  control_elements =
      form_util::GetUnownedAutofillableFormFieldElements(document, &fieldsets);
  std::vector<WebElement> iframe_elements =
      form_util::GetUnownedIframeElements(document);

  FormData synthetic_form;
  if (!UnownedFormElementsAndFieldSetsToFormData(
          fieldsets, control_elements, iframe_elements, nullptr, document,
          field_data_manager, extract_mask, &synthetic_form, nullptr)) {
    PruneInitialValueCaches(observed_unique_renderer_ids);
    return forms;
  }
  if (!ProcessForm(std::move(synthetic_form)))
    return forms;

  PruneInitialValueCaches(observed_unique_renderer_ids);
  return forms;
}

std::vector<FormData> FormCache::ExtractNewForms(
    const FieldDataManager* field_data_manager) {
  if (base::FeatureList::IsEnabled(features::kAutofillUseNewFormExtraction)) {
    return ModifiedExtractNewForms(field_data_manager);
  }
  std::vector<FormData> forms;
  WebDocument document = frame_->GetDocument();
  if (document.IsNull())
    return forms;

  initial_checked_state_.clear();
  initial_select_values_.clear();

  std::set<FieldRendererId> observed_unique_renderer_ids;

  // Log an error message for deprecated attributes, but only the first time
  // the form is parsed.
  bool log_deprecation_messages = parsed_forms_.empty();

  const form_util::ExtractMask extract_mask =
      static_cast<form_util::ExtractMask>(form_util::EXTRACT_VALUE |
                                          form_util::EXTRACT_OPTIONS);

  size_t num_fields_seen = 0;
  size_t num_frames_seen = 0;
  for (const WebFormElement& form_element : document.Forms()) {
    std::vector<WebFormControlElement> control_elements =
        form_util::ExtractAutofillableElementsInForm(form_element);

    FormData form;
    if (!WebFormElementToFormData(form_element, WebFormControlElement(),
                                  field_data_manager, extract_mask, &form,
                                  nullptr)) {
      continue;
    }

    for (const auto& field : form.fields)
      observed_unique_renderer_ids.insert(field.unique_renderer_id);

    num_fields_seen += form.fields.size();
    num_frames_seen += form.child_frames.size();
    if (num_fields_seen > kMaxParseableFields ||
        num_frames_seen > kMaxParseableFrames) {
      PruneInitialValueCaches(observed_unique_renderer_ids);
      return forms;
    }

    size_t num_editable_elements =
        ScanFormControlElements(control_elements, log_deprecation_messages);

    if (!base::Contains(parsed_forms_, form) &&
        IsFormInteresting(form, num_editable_elements)) {
      for (auto it = parsed_forms_.begin(); it != parsed_forms_.end(); ++it) {
        // We don't want to store twice forms that have the same rendererID or
        // the same attributes/fields.
        if (base::FeatureList::IsEnabled(
                features::
                    kAutofillUseOnlyFormRendererIDForOldDuplicateFormRemoval)) {
          if (it->unique_renderer_id == form.unique_renderer_id) {
            parsed_forms_.erase(it);
            break;
          }
        } else {
          if (it->SameFormAs(form)) {
            parsed_forms_.erase(it);
            break;
          }
        }
      }

      SaveInitialValues(control_elements);
      forms.push_back(form);
      parsed_forms_.insert(form);
    }
  }

  // Look for more parseable fields outside of forms.
  std::vector<WebElement> fieldsets;
  std::vector<WebFormControlElement> control_elements =
      form_util::GetUnownedAutofillableFormFieldElements(document, &fieldsets);
  std::vector<WebElement> iframe_elements =
      form_util::GetUnownedIframeElements(document);

  FormData synthetic_form;
  if (!UnownedFormElementsAndFieldSetsToFormData(
          fieldsets, control_elements, iframe_elements, nullptr, document,
          field_data_manager, extract_mask, &synthetic_form, nullptr)) {
    PruneInitialValueCaches(observed_unique_renderer_ids);
    return forms;
  }

  for (const auto& field : synthetic_form.fields)
    observed_unique_renderer_ids.insert(field.unique_renderer_id);

  num_fields_seen += synthetic_form.fields.size();
  num_frames_seen += synthetic_form.child_frames.size();
  if (num_fields_seen > kMaxParseableFields ||
      num_frames_seen > kMaxParseableFrames) {
    PruneInitialValueCaches(observed_unique_renderer_ids);
    return forms;
  }

  size_t num_editable_elements =
      ScanFormControlElements(control_elements, log_deprecation_messages);

  if (!base::Contains(parsed_forms_, synthetic_form) &&
      IsFormInteresting(synthetic_form, num_editable_elements)) {
    SaveInitialValues(control_elements);
    forms.push_back(synthetic_form);
    parsed_forms_.insert(synthetic_form);
    parsed_forms_.erase(synthetic_form_);
    synthetic_form_ = synthetic_form;
  }

  PruneInitialValueCaches(observed_unique_renderer_ids);
  return forms;
}

void FormCache::Reset() {
  // Record the size of |parsed_forms_| every time it reaches its peak size. The
  // peak size is reached right before the cache is cleared.
  UMA_HISTOGRAM_COUNTS_1000("Autofill.FormCacheSize", parsed_forms_.size());

  synthetic_form_ = FormData();
  parsed_forms_.clear();
  // Remove after the `AutofillUseNewFormExtraction` feature is deleted.
  parsed_forms_rendererid_.clear();
  initial_select_values_.clear();
  initial_checked_state_.clear();
  fields_eligible_for_manual_filling_.clear();
}

void FormCache::ClearElement(WebFormControlElement& control_element,
                             const WebFormControlElement& element) {
  // Don't modify the value of disabled fields.
  if (!control_element.IsEnabled())
    return;

  // Don't clear the fields that were not autofilled.
  if (!control_element.IsAutofilled())
    return;

  if (control_element.AutofillSection() != element.AutofillSection())
    return;

  control_element.SetAutofillState(WebAutofillState::kNotFilled);

  WebInputElement* input_element = ToWebInputElement(&control_element);
  if (form_util::IsTextInput(input_element) ||
      form_util::IsMonthInput(input_element)) {
    input_element->SetAutofillValue(blink::WebString());

    // Clearing the value in the focused node (above) can cause the selection
    // to be lost. We force the selection range to restore the text cursor.
    if (element == *input_element) {
      size_t length = input_element->Value().length();
      input_element->SetSelectionRange(length, length);
    }
  } else if (form_util::IsTextAreaElement(control_element)) {
    control_element.SetAutofillValue(blink::WebString());
  } else if (form_util::IsSelectElement(control_element)) {
    WebSelectElement select_element = control_element.To<WebSelectElement>();
    auto initial_value_iter = initial_select_values_.find(
        FieldRendererId(select_element.UniqueRendererFormControlId()));
    if (initial_value_iter != initial_select_values_.end() &&
        select_element.Value().Utf16() != initial_value_iter->second) {
      select_element.SetAutofillValue(
          blink::WebString::FromUTF16(initial_value_iter->second));
      select_element.SetUserHasEditedTheField(false);
    }
  } else {
    WebInputElement input_element = control_element.To<WebInputElement>();
    DCHECK(form_util::IsCheckableElement(&input_element));
    auto checkable_element_it = initial_checked_state_.find(
        FieldRendererId(input_element.UniqueRendererFormControlId()));
    if (checkable_element_it != initial_checked_state_.end() &&
        input_element.IsChecked() != checkable_element_it->second) {
      input_element.SetChecked(checkable_element_it->second, true);
    }
  }
}

bool FormCache::ClearSectionWithElement(const WebFormControlElement& element) {
  // The intended behaviour is:
  // * Clear the currently focused element.
  // * Send the blur event.
  // * For each other element, focus -> clear -> blur.
  // * Send the focus event.
  WebFormElement form_element = element.Form();
  std::vector<WebFormControlElement> control_elements =
      form_element.IsNull()
          ? form_util::GetUnownedAutofillableFormFieldElements(
                element.GetDocument(), nullptr)
          : form_util::ExtractAutofillableElementsInForm(form_element);

  if (control_elements.empty())
    return true;

  if (control_elements.size() < 2 && control_elements[0].Focused()) {
    // If there is no other field to be cleared, sending the blur event and then
    // the focus event for the currently focused element does not make sense.
    ClearElement(control_elements[0], element);
    return true;
  }

  WebFormControlElement* initially_focused_element = nullptr;
  for (WebFormControlElement& control_element : control_elements) {
    if (control_element.Focused()) {
      initially_focused_element = &control_element;
      ClearElement(control_element, element);
      // A blur event is emitted for the focused element if it is an initiating
      // element before the clearing happens.
      initially_focused_element->DispatchBlurEvent();
      break;
    }
  }

  for (WebFormControlElement& control_element : control_elements) {
    if (control_element.Focused())
      continue;
    ClearElement(control_element, element);
  }

  // A focus event is emitted for the initiating element after clearing is
  // completed.
  if (initially_focused_element)
    initially_focused_element->DispatchFocusEvent();

  return true;
}

bool FormCache::ShowPredictions(const FormDataPredictions& form,
                                bool attach_predictions_to_dom) {
  DCHECK_EQ(form.data.fields.size(), form.fields.size());

  std::vector<WebFormControlElement> control_elements;

  if (form.data.unique_renderer_id.is_null()) {  // Form is synthetic.
    WebDocument document = frame_->GetDocument();
    control_elements =
        form_util::GetUnownedAutofillableFormFieldElements(document, nullptr);
  } else {
    for (const WebFormElement& form_element : frame_->GetDocument().Forms()) {
      FormRendererId form_id(form_element.UniqueRendererFormId());
      if (form_id == form.data.unique_renderer_id) {
        control_elements =
            form_util::ExtractAutofillableElementsInForm(form_element);
        break;
      }
    }
  }

  if (control_elements.size() != form.fields.size()) {
    // Keep things simple.  Don't show predictions for forms that were modified
    // between page load and the server's response to our query.
    return false;
  }

  PageFormAnalyserLogger logger(frame_);
  for (size_t i = 0; i < control_elements.size(); ++i) {
    WebFormControlElement& element = control_elements[i];

    const FormFieldData& field_data = form.data.fields[i];
    FieldRendererId field_id(element.UniqueRendererFormControlId());
    if (field_id != field_data.unique_renderer_id)
      continue;
    const FormFieldDataPredictions& field = form.fields[i];

    element.SetFormElementPiiType(
        MapTypePredictionToFormElementPiiType(field.overall_type));

    // If the flag is enabled, attach the prediction to the field.
    if (attach_predictions_to_dom) {
      constexpr size_t kMaxLabelSize = 100;
      const std::u16string truncated_label = field_data.label.substr(
          0, std::min(field_data.label.length(), kMaxLabelSize));

      std::string form_id =
          base::NumberToString(form.data.unique_renderer_id.value());
      std::string field_id =
          base::NumberToString(field_data.unique_renderer_id.value());

      blink::LocalFrameToken frame_token;
      if (auto* frame = element.GetDocument().GetFrame())
        frame_token = frame->GetLocalFrameToken();

      std::string title = base::StrCat({"overall type: ",
                                        field.overall_type,
                                        "\nserver type: ",
                                        field.server_type,
                                        "\nheuristic type: ",
                                        field.heuristic_type,
                                        "\nlabel: ",
                                        base::UTF16ToUTF8(truncated_label),
                                        "\nparseable name: ",
                                        field.parseable_name,
                                        "\nsection: ",
                                        field.section,
                                        "\nfield signature: ",
                                        field.signature,
                                        "\nform signature: ",
                                        form.signature,
                                        "\nform signature in host form: ",
                                        field.host_form_signature,
                                        "\nfield frame token: ",
                                        frame_token.ToString(),
                                        "\nform renderer id: ",
                                        form_id,
                                        "\nfield renderer id: ",
                                        field_id});

      // Set this debug string to the title so that a developer can easily debug
      // by hovering the mouse over the input field.
      element.SetAttribute("title", WebString::FromUTF8(title));

      // Set the same debug string to an attribute that does not get mangled if
      // Google Translate is triggered for the site. This is useful for
      // automated processing of the data.
      element.SetAttribute("autofill-information", WebString::FromUTF8(title));

      element.SetAttribute("autofill-prediction",
                           WebString::FromUTF8(field.overall_type));
    }
  }
  logger.Flush();

  return true;
}

void FormCache::SetFieldsEligibleForManualFilling(
    const std::vector<FieldRendererId>& fields_eligible_for_manual_filling) {
  fields_eligible_for_manual_filling_ = base::flat_set<FieldRendererId>(
      std::move(fields_eligible_for_manual_filling));
}

size_t FormCache::ScanFormControlElements(
    const std::vector<WebFormControlElement>& control_elements,
    bool log_deprecation_messages) {
  size_t num_editable_elements = 0;
  for (const WebFormControlElement& element : control_elements) {
    if (log_deprecation_messages)
      LogDeprecationMessages(element);

    // Save original values of <select> elements so we can restore them
    // when |ClearFormWithNode()| is invoked.
    if (form_util::IsSelectElement(element) ||
        form_util::IsTextAreaElement(element)) {
      ++num_editable_elements;
    } else {
      const WebInputElement input_element = element.ToConst<WebInputElement>();
      if (!form_util::IsCheckableElement(&input_element))
        ++num_editable_elements;
    }
  }
  return num_editable_elements;
}

void FormCache::SaveInitialValues(
    const std::vector<WebFormControlElement>& control_elements) {
  for (const WebFormControlElement& element : control_elements) {
    if (form_util::IsSelectElement(element)) {
      const WebSelectElement select_element =
          element.ToConst<WebSelectElement>();
      initial_select_values_.insert(
// (neva) GCC 8.x.x
#if !defined(__clang__)
          std::make_pair(FieldRendererId(select_element.UniqueRendererFormControlId()),
#else
          std::make_pair(select_element.UniqueRendererFormControlId(),
#endif
                         select_element.Value().Utf16()));
    } else {
      const WebInputElement* input_element = ToWebInputElement(&element);
      if (form_util::IsCheckableElement(input_element)) {
        initial_checked_state_.insert(
// (neva) GCC 8.x.x
#if !defined(__clang__)
            std::make_pair(FieldRendererId(input_element->UniqueRendererFormControlId()),
#else
            std::make_pair(input_element->UniqueRendererFormControlId(),
#endif
                           input_element->IsChecked()));
      }
    }
  }
}

void FormCache::PruneInitialValueCaches(
    const std::set<FieldRendererId>& ids_to_retain) {
  auto should_not_retain = [&ids_to_retain](const auto& p) {
    return !base::Contains(ids_to_retain, p.first);
  };
  base::EraseIf(initial_select_values_, should_not_retain);
  base::EraseIf(initial_checked_state_, should_not_retain);
}

}  // namespace autofill
