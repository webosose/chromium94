// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/web_apps/web_app_url_handler_intent_picker_dialog_view.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/barrier_closure.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/check.h"
#include "base/compiler_specific.h"
#include "base/containers/flat_set.h"
#include "base/feature_list.h"
#include "base/location.h"
#include "base/metrics/histogram_functions.h"
#include "base/stl_util.h"
#include "base/strings/string_piece_forward.h"
#include "base/strings/utf_string_conversions.h"
#include "base/timer/elapsed_timer.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/web_apps/web_app_url_handler_hover_button.h"
#include "chrome/browser/web_applications/components/url_handler_launch_params.h"
#include "chrome/browser/web_applications/components/url_handler_prefs.h"
#include "chrome/browser/web_applications/web_app_provider.h"
#include "chrome/browser/web_applications/web_app_registrar.h"
#include "chrome/grit/generated_resources.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#include "extensions/browser/extension_dialog_auto_confirm.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/ui_base_types.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_delegate.h"
#include "url/gurl.h"

namespace {

// Maximum numbers of web apps we want to show at a time in the dialog.
// The height of the scroll in the dialog depends on how many app
// candidates we got and how many we want to show. If there is more than
// |KMaxAppResults| app candidates, we will show 3.5 apps to let the user
// know there are more than |kMaxAppResults| apps accessible by scrolling
// the list.
constexpr size_t kMaxAppResults = 3;
// This dialog follows the design that
// chrome/browser/ui/views/intent_picker_bubble_view.cc created and the
// main component sizes were also mostly copied over to share the
// same layout.
// Main components sizes
constexpr int kIntentPickerCheckBoxColumnWidth = 288;
constexpr int kMaxIntentPickerWidth = 320;
constexpr int kRowHeight = 32;
constexpr int kTitlePadding = 16;
constexpr gfx::Insets kSeparatorPadding(0, 0, 16, 0);
constexpr SkColor kSeparatorColor = SkColorSetARGB(0x1F, 0x0, 0x0, 0x0);

std::unique_ptr<views::Separator> CreateHorizontalSeparator() {
  auto separator = std::make_unique<views::Separator>();
  separator->SetColor(kSeparatorColor);
  separator->SetBorder(views::CreateEmptyBorder(kSeparatorPadding));
  return separator;
}

void RecordDialogState(
    WebAppUrlHandlerIntentPickerView::DialogState dialog_state) {
  base::UmaHistogramEnumeration("WebApp.UrlHandling.DialogState", dialog_state);
}

}  // namespace

// static
base::flat_set<Profile*>
WebAppUrlHandlerIntentPickerView::GetUrlHandlingValidProfiles(
    std::vector<web_app::UrlHandlerLaunchParams>& launch_params_list) {
  ProfileManager* const profile_manager = g_browser_process->profile_manager();
  if (!profile_manager)
    return {};

  std::vector<Profile*> profiles;
  // A predicate function for base::EraseIf that returns true if `params`
  // references an invalid Profile. Otherwise, adds the corresponding profile to
  // `profiles` and returns false.
  // TODO(crbug.com/1217419): Verify if site permission is enabled.
  auto remove_pred = [profile_manager, &profiles](
                         const web_app::UrlHandlerLaunchParams& params) {
    if (!profile_manager->GetProfileAttributesStorage()
             .GetProfileAttributesWithPath(params.profile_path)) {
      return true;  // Profile deleted or path otherwise invalid.
    }

    Profile* const profile = profile_manager->GetProfile(params.profile_path);
    if (!profile)
      return true;  // Failed to load profile.

    profiles.push_back(profile);
    return false;
  };

  profiles.reserve(launch_params_list.size());
  base::ElapsedTimer timer;
  base::EraseIf(launch_params_list, std::move(remove_pred));
  base::UmaHistogramMicrosecondsTimes(
      "WebApp.UrlHandling.GetValidProfilesAtStartUp", timer.Elapsed());
  return std::move(profiles);
}

void WebAppUrlHandlerIntentPickerView::Show(
    const GURL& url,
    std::vector<web_app::UrlHandlerLaunchParams> launch_params_list,
    std::unique_ptr<ScopedKeepAlive> keep_alive,
    chrome::WebAppUrlHandlerAcceptanceCallback dialog_close_callback) {
  auto view = std::make_unique<WebAppUrlHandlerIntentPickerView>(
      url, std::move(launch_params_list), std::move(keep_alive),
      std::move(dialog_close_callback));

  views::DialogDelegate::CreateDialogWidget(std::move(view),
                                            /*context=*/nullptr,
                                            /*parent=*/nullptr)
      ->Show();
}

WebAppUrlHandlerIntentPickerView::WebAppUrlHandlerIntentPickerView(
    const GURL& url,
    std::vector<web_app::UrlHandlerLaunchParams> launch_params_list,
    std::unique_ptr<ScopedKeepAlive> keep_alive,
    chrome::WebAppUrlHandlerAcceptanceCallback dialog_close_callback)
    : url_(url),
      launch_params_list_(std::move(launch_params_list)),
      close_callback_(std::move(dialog_close_callback)),
      // Pass the ScopedKeepAlive into here ensures the process is alive until
      // the dialog is closed, and initiates the shutdown at closure if there
      // is nothing else keeping the browser alive.
      keep_alive_(std::move(keep_alive)) {
  SetDefaultButton(ui::DIALOG_BUTTON_OK);
  // Disable the open button by default and enable it when the user has
  // selected an option.
  SetButtonEnabled(ui::DIALOG_BUTTON_OK, false);
  SetModalType(ui::MODAL_TYPE_NONE);
  std::u16string title =
      l10n_util::GetStringUTF16(IDS_URL_HANDLER_INTENT_PICKER_TITLE);
  SetTitle(title);
  SetShowCloseButton(true);

  SetButtonLabel(
      ui::DIALOG_BUTTON_OK,
      l10n_util::GetStringUTF16(IDS_URL_HANDLER_INTENT_PICKER_OK_BUTTON_TEXT));
  SetButtonLabel(ui::DIALOG_BUTTON_CANCEL,
                 l10n_util::GetStringUTF16(
                     IDS_URL_HANDLER_INTENT_PICKER_CANCEL_BUTTON_TEXT));

  SetAcceptCallback(base::BindOnce(
      &WebAppUrlHandlerIntentPickerView::OnAccepted, base::Unretained(this)));

  SetCancelCallback(base::BindOnce(
      &WebAppUrlHandlerIntentPickerView::OnCanceled, base::Unretained(this)));

  SetCloseCallback(base::BindOnce(&WebAppUrlHandlerIntentPickerView::OnClosed,
                                  base::Unretained(this)));
  Initialize();
}

WebAppUrlHandlerIntentPickerView::~WebAppUrlHandlerIntentPickerView() = default;

gfx::Size WebAppUrlHandlerIntentPickerView::CalculatePreferredSize() const {
  return gfx::Size(kMaxIntentPickerWidth,
                   GetHeightForWidth(kMaxIntentPickerWidth));
}

absl::optional<web_app::UrlHandlerLaunchParams>
WebAppUrlHandlerIntentPickerView::GetSelectedLaunchParams() const {
  // User didn't make a choice, no launch params.
  if (!HasUserSelectedApp())
    return absl::nullopt;

  DCHECK(IsSelectedAppValid());

  if (hover_buttons_[selected_app_tag_.value()]->is_app()) {
    return hover_buttons_[selected_app_tag_.value()]
        ->url_handler_launch_params();
  }

  // User has selected the browser, no launch params.
  return absl::nullopt;
}

void WebAppUrlHandlerIntentPickerView::SetSelectedAppIndex(
    size_t index,
    const ui::Event& event) {
  DCHECK_GE(index, 0u);
  DCHECK_LT(index, hover_buttons_.size());
  if (!HasUserSelectedApp()) {
    // User made a choice for the first time, enable the open button.
    SetButtonEnabled(ui::DIALOG_BUTTON_OK, true);
  } else {
    // Unselect the previous user choice.
    hover_buttons_[selected_app_tag_.value()]->MarkAsUnselected(nullptr);
  }
  selected_app_tag_ = index;
  hover_buttons_[selected_app_tag_.value()]->MarkAsSelected(&event);
  views::View::RequestFocus();
}

void WebAppUrlHandlerIntentPickerView::OnAccepted() {
  RunCloseCallback(/*accepted=*/true);
}

void WebAppUrlHandlerIntentPickerView::OnCanceled() {
  RunCloseCallback(/*accepted=*/false);
}

void WebAppUrlHandlerIntentPickerView::OnClosed() {
  OnCanceled();
}

void WebAppUrlHandlerIntentPickerView::Initialize() {
  views::GridLayout* layout =
      SetLayoutManager(std::make_unique<views::GridLayout>());

  // Creates a view to hold the views for each app.
  auto scrollable_view = std::make_unique<views::View>();
  scrollable_view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  web_app::WebAppProvider* provider;
  // size+1 for the browser entry.
  size_t total_buttons = launch_params_list_.size() + 1;
  hover_buttons_.reserve(total_buttons);
  // Create a WebAppUrlHandlerHoverButton to open the link in browser and
  // list it as the first choice.
  auto app_button =
      std::make_unique<WebAppUrlHandlerHoverButton>(base::BindRepeating(
          &WebAppUrlHandlerIntentPickerView::SetSelectedAppIndex,
          base::Unretained(this), 0));
  app_button->set_tag(0);
  app_button->GetViewAccessibility().OverridePosInSet(1, total_buttons);
  hover_buttons_.push_back(app_button.get());
  scrollable_view->AddChildViewAt(std::move(app_button), 0);

  for (const auto& launch_params : launch_params_list_) {
    Profile* profile = g_browser_process->profile_manager()->GetProfileByPath(
        launch_params.profile_path);
    provider = web_app::WebAppProvider::Get(profile);
    web_app::WebAppRegistrar& registrar = provider->registrar();

    const std::u16string& profile_name =
        profiles::GetAvatarNameForProfile(launch_params.profile_path);
    const std::u16string& app_name = base::UTF8ToUTF16(
        base::StringPiece(registrar.GetAppShortName(launch_params.app_id)));
    const std::u16string& app_title =
        (profile_name ==
         l10n_util::GetStringUTF16(IDS_SINGLE_PROFILE_DISPLAY_NAME))
            ? app_name
            : l10n_util::GetStringFUTF16(
                  IDS_URL_HANDLER_INTENT_PICKER_APP_TITLE, app_name,
                  profile_name);

    const size_t button_index = hover_buttons_.size();
    // TODO(crbug.com/1072058): Make sure the UI is reasonable when
    // |app_title| is long.
    auto app_button = std::make_unique<WebAppUrlHandlerHoverButton>(
        base::BindRepeating(
            &WebAppUrlHandlerIntentPickerView::SetSelectedAppIndex,
            base::Unretained(this), button_index),
        launch_params, provider, app_title,
        registrar.GetAppStartUrl(launch_params.app_id));
    app_button->set_tag(button_index);
    app_button->GetViewAccessibility().OverridePosInSet(button_index + 1,
                                                        total_buttons);
    hover_buttons_.push_back(app_button.get());
    scrollable_view->AddChildViewAt(std::move(app_button), button_index);
  }

  auto scroll_view = std::make_unique<views::ScrollView>();
  scroll_view->SetBackgroundThemeColorId(
      ui::NativeTheme::kColorId_BubbleBackground);
  scroll_view->SetContents(std::move(scrollable_view));
  // This part gives the scroll a fixed width and height. The height depends on
  // how many app candidates we got and how many we actually want to show.
  // The added 0.5 on the else block allow us to let the user know there are
  // more than |kMaxAppResults| apps accessible by scrolling the list.
  scroll_view->ClipHeightTo(kRowHeight, (kMaxAppResults + 0.5) * kRowHeight);
  scroll_view->GetViewAccessibility().OverrideRole(
      ax::mojom::Role::kRadioGroup);

  constexpr int kColumnSetId = 0;
  views::ColumnSet* cs = layout->AddColumnSet(kColumnSetId);
  cs->AddColumn(views::GridLayout::FILL, views::GridLayout::CENTER,
                views::GridLayout::kFixedSize,
                views::GridLayout::ColumnSize::kFixed, kMaxIntentPickerWidth,
                0);

  layout->StartRowWithPadding(views::GridLayout::kFixedSize, kColumnSetId,
                              views::GridLayout::kFixedSize, kTitlePadding);
  scroll_view_ = layout->AddView(std::move(scroll_view));
  layout->StartRow(views::GridLayout::kFixedSize, kColumnSetId, 0);

  // The checkbox allows the user to opt-in to relaxed security
  // (i.e. skipping future prompts) for this url.
  layout->AddView(CreateHorizontalSeparator());
  // This second ColumnSet has a padding column in order to manipulate the
  // Checkbox positioning freely.
  constexpr int kColumnSetIdPadded = 2;
  views::ColumnSet* cs_padded = layout->AddColumnSet(kColumnSetIdPadded);
  cs_padded->AddPaddingColumn(views::GridLayout::kFixedSize, kTitlePadding);
  cs_padded->AddColumn(views::GridLayout::FILL, views::GridLayout::CENTER,
                       views::GridLayout::kFixedSize,
                       views::GridLayout::ColumnSize::kFixed,
                       kIntentPickerCheckBoxColumnWidth, 0);
  layout->StartRowWithPadding(views::GridLayout::kFixedSize, kColumnSetIdPadded,
                              views::GridLayout::kFixedSize, 0);

  enable_remember_checkbox_ =
      base::FeatureList::IsEnabled(blink::features::kWebAppEnableUrlHandlers);

  if (enable_remember_checkbox_) {
    remember_selection_checkbox_ = layout->AddView(
        std::make_unique<views::Checkbox>(l10n_util::GetStringUTF16(
            IDS_URL_HANDLER_INTENT_PICKER_REMEMBER_SELECTION)));
    layout->AddPaddingRow(views::GridLayout::kFixedSize, kRowHeight);
  }
}

void WebAppUrlHandlerIntentPickerView::RunCloseCallback(bool accepted) {
  if (!close_callback_)
    return;

  absl::optional<web_app::UrlHandlerLaunchParams> launch_params;
  bool accepted_override = false;
  switch (extensions::ScopedTestDialogAutoConfirm::GetAutoConfirmValue()) {
    case extensions::ScopedTestDialogAutoConfirm::NONE:
      accepted_override = accepted;
      launch_params = GetSelectedLaunchParams();
      break;
    case extensions::ScopedTestDialogAutoConfirm::ACCEPT_AND_REMEMBER_OPTION:
      remember_selection_checkbox_->SetChecked(/*checked=*/true);
      FALLTHROUGH;
    case extensions::ScopedTestDialogAutoConfirm::ACCEPT_AND_OPTION:
      accepted_override = true;
      selected_app_tag_ =
          extensions::ScopedTestDialogAutoConfirm::GetOptionSelected();
      launch_params = GetSelectedLaunchParams();
      break;
    case extensions::ScopedTestDialogAutoConfirm::ACCEPT:
      accepted_override = true;
      launch_params = GetSelectedLaunchParams();
      break;
    case extensions::ScopedTestDialogAutoConfirm::CANCEL:
      accepted_override = false;
      launch_params = absl::nullopt;
      break;
  }

  auto state = DialogState::kClosed;
  if (accepted_override) {
    const bool remember_choice_checked =
        enable_remember_checkbox_ && remember_selection_checkbox_->GetChecked();

    if (remember_choice_checked) {
      if (launch_params) {
        // An app is selected as the default choice.
        web_app::url_handler_prefs::SaveOpenInApp(
            g_browser_process->local_state(), launch_params->app_id,
            launch_params->profile_path, launch_params->url);
        state = DialogState::kAppAcceptedAndRememberChoice;
      } else {
        // The browser is the selected default choice.
        web_app::url_handler_prefs::SaveOpenInBrowser(
            g_browser_process->local_state(), url_);
        state = DialogState::kBrowserAcceptedAndRememberChoice;
      }
    } else {
      state = launch_params ? DialogState::kAppAcceptedNoRememberChoice
                            : DialogState::kBrowserAcceptedNoRememberChoice;
    }
  }
  RecordDialogState(state);

  std::move(close_callback_).Run(accepted_override, std::move(launch_params));
}

bool WebAppUrlHandlerIntentPickerView::IsSelectedAppValid() const {
  return selected_app_tag_.has_value() && selected_app_tag_.value() >= 0 &&
         selected_app_tag_.value() < static_cast<int>(hover_buttons_.size());
}

bool WebAppUrlHandlerIntentPickerView::HasUserSelectedApp() const {
  return selected_app_tag_.has_value();
}

BEGIN_METADATA(WebAppUrlHandlerIntentPickerView, views::DialogDelegateView)
END_METADATA

namespace chrome {

// static
void ShowWebAppUrlHandlerIntentPickerDialog(
    const GURL& url,
    std::vector<web_app::UrlHandlerLaunchParams> launch_params_list,
    WebAppUrlHandlerAcceptanceCallback dialog_close_callback) {
  DCHECK(dialog_close_callback);
  auto keep_alive = std::make_unique<ScopedKeepAlive>(
      KeepAliveOrigin::WEB_APP_INTENT_PICKER, KeepAliveRestartOption::DISABLED);

  base::flat_set<Profile*> profiles =
      WebAppUrlHandlerIntentPickerView::GetUrlHandlingValidProfiles(
          launch_params_list);

  auto show_dialog_callback = base::BindOnce(
      [](const GURL& url, std::unique_ptr<ScopedKeepAlive> keep_alive,
         base::ElapsedTimer timer,
         std::vector<web_app::UrlHandlerLaunchParams> launch_params_list,
         WebAppUrlHandlerAcceptanceCallback dialog_close_callback) {
        // Record registrar loading time before showing the dialog.
        base::UmaHistogramMicrosecondsTimes(
            "WebApp.UrlHandling.LoadWebAppRegistrarsAtStartUp",
            timer.Elapsed());

        // TODO(crbug.com/1217419): Check if site permission is enabled once
        // all profiles and registrars are loaded.

        WebAppUrlHandlerIntentPickerView::Show(
            url, std::move(launch_params_list), std::move(keep_alive),
            std::move(dialog_close_callback));
      },
      url, std::move(keep_alive), base::ElapsedTimer(),
      std::move(launch_params_list), std::move(dialog_close_callback));

  auto on_registrar_ready_callback =
      base::BarrierClosure(profiles.size(), std::move(show_dialog_callback));

  for (Profile* profile : profiles) {
    auto* provider = web_app::WebAppProvider::Get(profile);
    DCHECK(provider);

    provider->on_registry_ready().Post(FROM_HERE, on_registrar_ready_callback);
  }
}

}  // namespace chrome
