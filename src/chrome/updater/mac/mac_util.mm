// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/updater/mac/mac_util.h"

#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/process/launch.h"
#include "base/strings/strcat.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/version.h"
#include "chrome/common/mac/launchd.h"
#include "chrome/updater/constants.h"
#include "chrome/updater/updater_branding.h"
#include "chrome/updater/updater_scope.h"
#include "chrome/updater/updater_version.h"
#include "chrome/updater/util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace updater {
namespace {

constexpr int kLaunchctlExitCodeNoSuchProcess = 3;

base::FilePath GetUpdateFolderName() {
  return base::FilePath(COMPANY_SHORTNAME_STRING)
      .AppendASCII(PRODUCT_FULLNAME_STRING);
}

base::FilePath ExecutableFolderPath() {
  return base::FilePath(
             base::StrCat({PRODUCT_FULLNAME_STRING, kExecutableSuffix, ".app"}))
      .Append(FILE_PATH_LITERAL("Contents"))
      .Append(FILE_PATH_LITERAL("MacOS"));
}

}  // namespace

base::FilePath GetExecutableRelativePath() {
  return ExecutableFolderPath().Append(
      base::StrCat({PRODUCT_FULLNAME_STRING, kExecutableSuffix}));
}

absl::optional<base::FilePath> GetLibraryFolderPath(UpdaterScope scope) {
  switch (scope) {
    case UpdaterScope::kUser:
      return base::mac::GetUserLibraryPath();
    case UpdaterScope::kSystem: {
      base::FilePath local_library_path;
      if (!base::mac::GetLocalDirectory(NSLibraryDirectory,
                                        &local_library_path)) {
        VLOG(1) << "Could not get local library path";
        return absl::nullopt;
      }
      return local_library_path;
    }
  }
}

absl::optional<base::FilePath> GetApplicationSupportDirectory(
    UpdaterScope scope) {
  base::FilePath path;
  switch (scope) {
    case UpdaterScope::kUser:
      if (base::mac::GetUserDirectory(NSApplicationSupportDirectory, &path))
        return path;
      break;
    case UpdaterScope::kSystem:
      if (base::mac::GetLocalDirectory(NSApplicationSupportDirectory, &path))
        return path;
      break;
  }

  VLOG(1) << "Could not get applications support path";
  return absl::nullopt;
}

absl::optional<base::FilePath> GetUpdaterFolderPath(UpdaterScope scope) {
  absl::optional<base::FilePath> path = GetLibraryFolderPath(scope);
  if (!path)
    return absl::nullopt;
  return path->Append(GetUpdateFolderName());
}

absl::optional<base::FilePath> GetVersionedUpdaterFolderPathForVersion(
    UpdaterScope scope,
    const base::Version& version) {
  absl::optional<base::FilePath> path = GetUpdaterFolderPath(scope);
  if (!path)
    return absl::nullopt;
  return path->AppendASCII(version.GetString());
}

absl::optional<base::FilePath> GetVersionedUpdaterFolderPath(
    UpdaterScope scope) {
  absl::optional<base::FilePath> path = GetUpdaterFolderPath(scope);
  if (!path)
    return absl::nullopt;
  return path->AppendASCII(kUpdaterVersion);
}

absl::optional<base::FilePath> GetExecutableFolderPathForVersion(
    UpdaterScope scope,
    const base::Version& version) {
  absl::optional<base::FilePath> path =
      GetVersionedUpdaterFolderPathForVersion(scope, version);
  if (!path)
    return absl::nullopt;
  return path->Append(ExecutableFolderPath());
}

absl::optional<base::FilePath> GetUpdaterExecutablePath(UpdaterScope scope) {
  absl::optional<base::FilePath> path = GetVersionedUpdaterFolderPath(scope);
  if (!path)
    return absl::nullopt;
  return path->Append(ExecutableFolderPath())
      .AppendASCII(base::StrCat({PRODUCT_FULLNAME_STRING, kExecutableSuffix}));
}

bool PathOwnedByUser(const base::FilePath& path) {
  struct passwd* result = nullptr;
  struct passwd user_info = {};
  char pwbuf[2048] = {};
  const uid_t user_uid = geteuid();

  const int error =
      getpwuid_r(user_uid, &user_info, pwbuf, sizeof(pwbuf), &result);

  if (error) {
    VLOG(1) << "Failed to get user info.";
    return true;
  }

  if (result == nullptr) {
    VLOG(1) << "No entry for user.";
    return true;
  }

  base::stat_wrapper_t stat_info = {};
  if (base::File::Lstat(path.value().c_str(), &stat_info) != 0) {
    DPLOG(ERROR) << "Failed to get information on path " << path.value();
    return false;
  }

  if (S_ISLNK(stat_info.st_mode)) {
    DLOG(ERROR) << "Path " << path.value() << " is a symbolic link.";
    return false;
  }

  if (stat_info.st_uid != user_uid) {
    DLOG(ERROR) << "Path " << path.value() << " is owned by the wrong user.";
    return false;
  }

  return true;
}

bool RemoveJobFromLaunchd(UpdaterScope scope,
                          Launchd::Domain domain,
                          Launchd::Type type,
                          base::ScopedCFTypeRef<CFStringRef> name) {
  // This may block while deleting the launchd plist file.
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);

  if (Launchd::GetInstance()->PlistExists(domain, type, name)) {
    if (!Launchd::GetInstance()->DeletePlist(domain, type, name))
      return false;
  }

  base::CommandLine command_line(base::FilePath("/bin/launchctl"));
  command_line.AppendArg("remove");
  command_line.AppendArg(base::SysCFStringRefToUTF8(name));
  if (scope == UpdaterScope::kSystem)
    command_line = MakeElevated(command_line);

  int exit_code = -1;
  std::string output;
  base::GetAppOutputWithExitCode(command_line, &output, &exit_code);
  return exit_code == 0 || exit_code == kLaunchctlExitCodeNoSuchProcess;
}

}  // namespace updater
