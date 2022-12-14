// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/metrics/login_event_recorder.h"

#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/system/sys_info.h"
#include "base/task/current_thread.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "base/values.h"

namespace chromeos {

namespace {

constexpr char kUptime[] = "uptime";
constexpr char kDisk[] = "disk";

// The pointer to this object is used as a perfetto async event id.
constexpr char kBootTimes[] = "BootTimes";

#define FPL(value) FILE_PATH_LITERAL(value)

// Dir uptime & disk logs are located in.
constexpr const base::FilePath::CharType kLogPath[] = FPL("/tmp");

// Dir log{in,out} logs are located in.
constexpr base::FilePath::CharType kLoginLogPath[] = FPL("/home/chronos/user");

// Prefix for the time measurement files.
constexpr base::FilePath::CharType kUptimePrefix[] = FPL("uptime-");

// Prefix for the disk usage files.
constexpr base::FilePath::CharType kDiskPrefix[] = FPL("disk-");

// Names of login stats files.
constexpr base::FilePath::CharType kLoginSuccess[] = FPL("login-success");

// Delay in milliseconds before writing the login times to disk.
constexpr int64_t kLoginTimeWriteDelayMs = 3000;

// Appends the given buffer into the file. Returns the number of bytes
// written, or -1 on error.<
// TODO(satorux): Move this to file_util.
int AppendFile(const base::FilePath& file_path, const char* data, int size) {
  // Appending boot times to (probably) a symlink in /tmp is a security risk for
  // developers with chromeos=1 builds.
  if (!base::SysInfo::IsRunningOnChromeOS())
    return -1;

  FILE* file = base::OpenFile(file_path, "a");
  if (!file)
    return -1;

  const int num_bytes_written = fwrite(data, 1, size, file);
  base::CloseFile(file);
  return num_bytes_written;
}

void WriteTimes(const std::string base_name,
                const std::string uma_name,
                const std::string uma_prefix,
                std::vector<LoginEventRecorder::TimeMarker> times) {
  DCHECK(times.size());
  const int kMinTimeMillis = 1;
  const int kMaxTimeMillis = 30000;
  const int kNumBuckets = 100;
  const base::FilePath log_path(kLoginLogPath);

  // Need to sort by time since the entries may have been pushed onto the
  // vector (on the UI thread) in a different order from which they were
  // created (potentially on other threads).
  std::sort(times.begin(), times.end());

  base::Time first = times.front().time();
  base::Time last = times.back().time();
  base::TimeDelta total = last - first;
  base::HistogramBase* total_hist = base::Histogram::FactoryTimeGet(
      uma_name, base::TimeDelta::FromMilliseconds(kMinTimeMillis),
      base::TimeDelta::FromMilliseconds(kMaxTimeMillis), kNumBuckets,
      base::HistogramBase::kUmaTargetedHistogramFlag);
  total_hist->AddTime(total);
  std::string output =
      base::StringPrintf("%s: %.2f", uma_name.c_str(), total.InSecondsF());
  base::Time prev = first;
  // Convert base::Time to base::TimeTicks for tracing.
  auto time2timeticks = [](const base::Time& ts) {
    return base::TimeTicks::Now() - (base::Time::Now() - ts);
  };
  // Send first event to name the track:
  // "In Chrome, we usually don't bother setting explicit track names. If none
  // is provided, the track is named after the first event on the track."
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN_WITH_TIMESTAMP0(
      "startup", kBootTimes, TRACE_ID_LOCAL(kBootTimes), time2timeticks(prev));

  for (unsigned int i = 0; i < times.size(); ++i) {
    const LoginEventRecorder::TimeMarker& tm = times[i];

    if (tm.url().has_value()) {
      TRACE_EVENT_NESTABLE_ASYNC_BEGIN_WITH_TIMESTAMP1(
          "startup", tm.name(), TRACE_ID_LOCAL(kBootTimes),
          time2timeticks(prev), "url", *tm.url());
      TRACE_EVENT_NESTABLE_ASYNC_END_WITH_TIMESTAMP1(
          "startup", tm.name(), TRACE_ID_LOCAL(kBootTimes),
          time2timeticks(tm.time()), "url", *tm.url());
    } else {
      TRACE_EVENT_NESTABLE_ASYNC_BEGIN_WITH_TIMESTAMP0(
          "startup", tm.name(), TRACE_ID_LOCAL(kBootTimes),
          time2timeticks(prev));
      TRACE_EVENT_NESTABLE_ASYNC_END_WITH_TIMESTAMP0("startup", tm.name(),
                                                     TRACE_ID_LOCAL(kBootTimes),
                                                     time2timeticks(tm.time()));
    }

    base::TimeDelta since_first = tm.time() - first;
    base::TimeDelta since_prev = tm.time() - prev;
    std::string name;

    if (tm.send_to_uma()) {
      name = uma_prefix + tm.name();
      base::HistogramBase* prev_hist = base::Histogram::FactoryTimeGet(
          name, base::TimeDelta::FromMilliseconds(kMinTimeMillis),
          base::TimeDelta::FromMilliseconds(kMaxTimeMillis), kNumBuckets,
          base::HistogramBase::kUmaTargetedHistogramFlag);
      prev_hist->AddTime(since_prev);
    } else {
      name = tm.name();
    }
    output += base::StringPrintf("\n%.2f +%.4f %s", since_first.InSecondsF(),
                                 since_prev.InSecondsF(), name.data());
    if (tm.url().has_value()) {
      output += ": ";
      output += *tm.url();
    }
    prev = tm.time();
  }
  output += '\n';
  TRACE_EVENT_NESTABLE_ASYNC_END_WITH_TIMESTAMP0(
      "startup", kBootTimes, TRACE_ID_LOCAL(kBootTimes), time2timeticks(prev));

  base::WriteFile(log_path.Append(base_name), output.data(), output.size());
}

}  // namespace

LoginEventRecorder::TimeMarker::TimeMarker(const char* name,
                                           absl::optional<std::string> url,
                                           bool send_to_uma)
    : name_(name), url_(url), send_to_uma_(send_to_uma) {}

LoginEventRecorder::TimeMarker::TimeMarker(const TimeMarker& other) = default;

LoginEventRecorder::TimeMarker::~TimeMarker() = default;

// static
LoginEventRecorder::Stats LoginEventRecorder::Stats::GetCurrentStats() {
  const base::FilePath kProcUptime(FPL("/proc/uptime"));
  const base::FilePath kDiskStat(FPL("/sys/block/sda/stat"));
  Stats stats;
  // Callers of this method expect synchronous behavior.
  // It's safe to allow IO here, because only virtual FS are accessed.
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  base::ReadFileToString(kProcUptime, &stats.uptime_);
  base::ReadFileToString(kDiskStat, &stats.disk_);
  return stats;
}

std::string LoginEventRecorder::Stats::SerializeToString() const {
  if (uptime_.empty() && disk_.empty())
    return std::string();
  base::DictionaryValue dictionary;
  dictionary.SetString(kUptime, uptime_);
  dictionary.SetString(kDisk, disk_);

  std::string result;
  if (!base::JSONWriter::Write(dictionary, &result)) {
    LOG(WARNING) << "LoginEventRecorder::Stats::SerializeToString(): failed.";
    return std::string();
  }

  return result;
}

// static
LoginEventRecorder::Stats LoginEventRecorder::Stats::DeserializeFromString(
    const std::string& source) {
  if (source.empty())
    return Stats();

  std::unique_ptr<base::Value> value = base::JSONReader::ReadDeprecated(source);
  base::DictionaryValue* dictionary;
  if (!value || !value->GetAsDictionary(&dictionary)) {
    LOG(ERROR) << "LoginEventRecorder::Stats::DeserializeFromString(): not a "
                  "dictionary: '"
               << source << "'";
    return Stats();
  }

  Stats result;
  if (!dictionary->GetString(kUptime, &result.uptime_) ||
      !dictionary->GetString(kDisk, &result.disk_)) {
    LOG(ERROR)
        << "LoginEventRecorder::Stats::DeserializeFromString(): format error: '"
        << source << "'";
    return Stats();
  }

  return result;
}

bool LoginEventRecorder::Stats::UptimeDouble(double* result) const {
  std::string uptime = uptime_;
  const size_t space_at = uptime.find_first_of(' ');
  if (space_at == std::string::npos)
    return false;

  uptime.resize(space_at);

  if (base::StringToDouble(uptime, result))
    return true;

  return false;
}

void LoginEventRecorder::Stats::RecordStats(const std::string& name) const {
  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BEST_EFFORT},
      base::BindOnce(&LoginEventRecorder::Stats::RecordStatsAsync,
                     base::Owned(new Stats(*this)), name));
}

void LoginEventRecorder::Stats::RecordStatsWithCallback(
    const std::string& name,
    base::OnceClosure callback) const {
  base::ThreadPool::PostTaskAndReply(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BEST_EFFORT},
      base::BindOnce(&LoginEventRecorder::Stats::RecordStatsAsync,
                     base::Owned(new Stats(*this)), name),
      std::move(callback));
}

void LoginEventRecorder::Stats::RecordStatsAsync(
    const base::FilePath::StringType& name) const {
  const base::FilePath log_path(kLogPath);
  const base::FilePath uptime_output =
      log_path.Append(base::FilePath(kUptimePrefix + name));
  const base::FilePath disk_output =
      log_path.Append(base::FilePath(kDiskPrefix + name));

  // Append numbers to the files.
  AppendFile(uptime_output, uptime_.data(), uptime_.size());
  AppendFile(disk_output, disk_.data(), disk_.size());
}

static base::LazyInstance<LoginEventRecorder>::DestructorAtExit
    g_login_event_recorder = LAZY_INSTANCE_INITIALIZER;

LoginEventRecorder::LoginEventRecorder()
    : task_runner_(base::SequencedTaskRunnerHandle::Get()) {
  DCHECK(base::CurrentUIThread::IsSet());
  login_time_markers_.reserve(30);
  logout_time_markers_.reserve(30);
}

LoginEventRecorder::~LoginEventRecorder() = default;

// static
LoginEventRecorder* LoginEventRecorder::Get() {
  return g_login_event_recorder.Pointer();
}

void LoginEventRecorder::AddLoginTimeMarker(const char* marker_name,
                                            bool send_to_uma) {
  AddLoginTimeMarkerWithURL(marker_name, absl::optional<std::string>(),
                            send_to_uma);
}

void LoginEventRecorder::AddLoginTimeMarkerWithURL(
    const char* marker_name,
    absl::optional<std::string> url,
    bool send_to_uma) {
  // Do not contaminate the UMA in the field.
  if (write_login_requested_ && send_to_uma) {
    LOG(WARNING) << "LoginTime Marker was requested after LoginDone()";
    return;
  }

  AddMarker(&login_time_markers_, TimeMarker(marker_name, url, send_to_uma));
}

void LoginEventRecorder::AddLogoutTimeMarker(const char* marker_name,
                                             bool send_to_uma) {
  AddMarker(
      &logout_time_markers_,
      TimeMarker(marker_name, absl::optional<std::string>(), send_to_uma));
}

void LoginEventRecorder::RecordAuthenticationSuccess() {
  AddLoginTimeMarker("Authenticate", true);
  RecordCurrentStats(kLoginSuccess);
}

void LoginEventRecorder::RecordAuthenticationFailure() {
  // no nothing for now.
}

void LoginEventRecorder::RecordCurrentStats(const std::string& name) {
  Stats::GetCurrentStats().RecordStats(name);
}

void LoginEventRecorder::ClearLoginTimeMarkers() {
  login_time_markers_.clear();
}

void LoginEventRecorder::WriteLoginTimes(const std::string base_name,
                                         const std::string uma_name,
                                         const std::string uma_prefix) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  write_login_requested_ = true;
  base::ThreadPool::PostDelayedTask(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BEST_EFFORT},
      base::BindOnce(&LoginEventRecorder::WriteLoginTimesDelayed,
                     weak_ptr_factory_.GetWeakPtr(), base_name, uma_name,
                     uma_prefix),
      base::TimeDelta::FromMilliseconds(kLoginTimeWriteDelayMs));
}

void LoginEventRecorder::WriteLogoutTimes(const std::string base_name,
                                          const std::string uma_name,
                                          const std::string uma_prefix) {
  WriteTimes(base_name, uma_name, uma_prefix, std::move(logout_time_markers_));
}

void LoginEventRecorder::AddMarker(std::vector<TimeMarker>* vector,
                                   TimeMarker&& marker) {
  // The marker vectors can only be safely manipulated on the main thread.
  // If we're late in the process of shutting down (eg. as can be the case at
  // logout), then we have to assume we're on the main thread already.
  if (task_runner_->RunsTasksInCurrentSequence()) {
    vector->push_back(marker);
  } else {
    // Add the marker on the UI thread.
    task_runner_->PostTask(FROM_HERE,
                           base::BindOnce(&LoginEventRecorder::AddMarker,
                                          weak_ptr_factory_.GetWeakPtr(),
                                          base::Unretained(vector), marker));
  }
}

void LoginEventRecorder::WriteLoginTimesDelayed(const std::string base_name,
                                                const std::string uma_name,
                                                const std::string uma_prefix) {
  WriteTimes(base_name, uma_name, uma_prefix, std::move(login_time_markers_));
}

}  // namespace chromeos
