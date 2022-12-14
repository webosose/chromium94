// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/webui/diagnostics_ui/backend/session_log_handler.h"

#include "ash/constants/ash_features.h"
#include "ash/public/cpp/holding_space/holding_space_client.h"
#include "ash/webui/diagnostics_ui/backend/networking_log.h"
#include "ash/webui/diagnostics_ui/backend/routine_log.h"
#include "ash/webui/diagnostics_ui/backend/telemetry_log.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "chromeos/strings/grit/chromeos_strings.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/shell_dialogs/select_file_policy.h"

namespace ash {
namespace diagnostics {
namespace {

const char kRoutineLogSectionHeader[] = "=== Routine Log === \n";
const char kTelemetryLogSectionHeader[] = "=== Telemetry Log === \n";
const char kNetworkingLogSectionHeader[] = "=== Networking Log === \n";
const char kDefaultSessionLogFileName[] = "session_log.txt";
const char kRoutineLogPath[] = "/tmp/diagnostics/diagnostics_routine_log";

}  // namespace

SessionLogHandler::SessionLogHandler(
    const SelectFilePolicyCreator& select_file_policy_creator,
    ash::HoldingSpaceClient* holding_space_client)
    : SessionLogHandler(
          select_file_policy_creator,
          std::make_unique<TelemetryLog>(),
          std::make_unique<RoutineLog>(base::FilePath(kRoutineLogPath)),
          std::make_unique<NetworkingLog>(),
          holding_space_client) {}

SessionLogHandler::SessionLogHandler(
    const SelectFilePolicyCreator& select_file_policy_creator,
    std::unique_ptr<TelemetryLog> telemetry_log,
    std::unique_ptr<RoutineLog> routine_log,
    std::unique_ptr<NetworkingLog> networking_log,
    ash::HoldingSpaceClient* holding_space_client)
    : select_file_policy_creator_(select_file_policy_creator),
      telemetry_log_(std::move(telemetry_log)),
      routine_log_(std::move(routine_log)),
      networking_log_(std::move(networking_log)),
      holding_space_client_(holding_space_client) {
  DCHECK(holding_space_client_);
}

SessionLogHandler::~SessionLogHandler() = default;

void SessionLogHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "initialize", base::BindRepeating(&SessionLogHandler::HandleInitialize,
                                        base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "saveSessionLog",
      base::BindRepeating(&SessionLogHandler::HandleSaveSessionLogRequest,
                          base::Unretained(this)));
}

void SessionLogHandler::FileSelected(const base::FilePath& path,
                                     int index,
                                     void* params) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&SessionLogHandler::CreateSessionLog,
                     base::Unretained(this), path),
      base::BindOnce(&SessionLogHandler::OnSessionLogCreated,
                     weak_factory_.GetWeakPtr(), path));
}

void SessionLogHandler::OnSessionLogCreated(const base::FilePath& file_path,
                                            bool success) {
  if (success) {
    holding_space_client_->AddDiagnosticsLog(file_path);
  }

  ResolveJavascriptCallback(base::Value(save_session_log_callback_id_),
                            base::Value(success));
  save_session_log_callback_id_ = "";

  if (log_created_closure_)
    std::move(log_created_closure_).Run();
}

void SessionLogHandler::FileSelectionCanceled(void* params) {
  RejectJavascriptCallback(base::Value(save_session_log_callback_id_),
                           /*success=*/base::Value(false));
  save_session_log_callback_id_ = "";
}

TelemetryLog* SessionLogHandler::GetTelemetryLog() const {
  return telemetry_log_.get();
}

RoutineLog* SessionLogHandler::GetRoutineLog() const {
  return routine_log_.get();
}

NetworkingLog* SessionLogHandler::GetNetworkingLog() const {
  return networking_log_.get();
}

void SessionLogHandler::SetWebUIForTest(content::WebUI* web_ui) {
  set_web_ui(web_ui);
}

void SessionLogHandler::SetLogCreatedClosureForTest(base::OnceClosure closure) {
  log_created_closure_ = std::move(closure);
}

bool SessionLogHandler::CreateSessionLog(const base::FilePath& file_path) {
  // Fetch RoutineLog
  const std::string routine_log_contents = routine_log_->GetContents();

  // Fetch TelemetryLog
  const std::string telemetry_log_contents = telemetry_log_->GetContents();

  std::vector<std::string> pieces = {
      kTelemetryLogSectionHeader, telemetry_log_contents,
      kRoutineLogSectionHeader, routine_log_contents};

  if (features::IsNetworkingInDiagnosticsAppEnabled()) {
    // Fetch NetworkingLog
    pieces.push_back(kNetworkingLogSectionHeader);
    pieces.push_back(networking_log_->GetContents());
  }

  return base::WriteFile(file_path, base::JoinString(pieces, "\n"));
}

void SessionLogHandler::HandleSaveSessionLogRequest(
    const base::ListValue* args) {
  CHECK_EQ(1U, args->GetSize());
  DCHECK(save_session_log_callback_id_.empty());
  save_session_log_callback_id_ = args->GetList()[0].GetString();

  content::WebContents* web_contents = web_ui()->GetWebContents();
  gfx::NativeWindow owning_window =
      web_contents ? web_contents->GetTopLevelNativeWindow()
                   : gfx::kNullNativeWindow;

  select_file_dialog_ = ui::SelectFileDialog::Create(
      this, select_file_policy_creator_.Run(web_contents));
  select_file_dialog_->SelectFile(
      ui::SelectFileDialog::SELECT_SAVEAS_FILE,
      /*title=*/l10n_util::GetStringUTF16(IDS_DIAGNOSTICS_SELECT_DIALOG_TITLE),
      /*default_path=*/base::FilePath(kDefaultSessionLogFileName),
      /*file_types=*/nullptr,
      /*file_type_index=*/0,
      /*default_extension=*/base::FilePath::StringType(), owning_window,
      /*params=*/nullptr);
}

void SessionLogHandler::HandleInitialize(const base::ListValue* args) {
  DCHECK(args && args->GetList().empty());
  AllowJavascript();
}

}  // namespace diagnostics
}  // namespace ash
