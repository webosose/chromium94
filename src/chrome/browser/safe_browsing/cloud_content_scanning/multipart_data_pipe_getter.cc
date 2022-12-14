// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/cloud_content_scanning/multipart_data_pipe_getter.h"

#include <algorithm>

#include "base/files/memory_mapped_file.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/strcat.h"
#include "base/time/time.h"
#include "mojo/public/c/system/data_pipe.h"
#include "net/base/net_errors.h"

namespace safe_browsing {

namespace {
const char kDataContentType[] = "Content-Type: application/octet-stream";

// Write the data from |file_| by chunks of 32 kbs.
constexpr int64_t kMaxSize = 32 * 1024;

}  // namespace

// static
std::unique_ptr<MultipartDataPipeGetter> MultipartDataPipeGetter::Create(
    const std::string& boundary,
    const std::string& metadata,
    base::File file) {
  if (!file.IsValid())
    return nullptr;

  auto mm_file = std::make_unique<base::MemoryMappedFile>();
  if (!mm_file->Initialize(std::move(file)))
    return nullptr;

  return std::make_unique<MultipartDataPipeGetter>(boundary, metadata,
                                                   std::move(mm_file));
}

MultipartDataPipeGetter::MultipartDataPipeGetter(
    const std::string& boundary,
    const std::string& metadata,
    std::unique_ptr<base::MemoryMappedFile> file)
    : file_(std::move(file)) {
  metadata_ = base::StrCat({"--", boundary, "\r\n", kDataContentType,
                            "\r\n\r\n", metadata, "\r\n--", boundary, "\r\n",
                            kDataContentType, "\r\n\r\n"});
  last_boundary_ = base::StrCat({"\r\n--", boundary, "--\r\n"});
  CHECK(file_->IsValid());
}

MultipartDataPipeGetter::~MultipartDataPipeGetter() = default;

void MultipartDataPipeGetter::Read(mojo::ScopedDataPipeProducerHandle pipe,
                                   ReadCallback callback) {
  Reset();

  std::move(callback).Run(net::OK, FullSize());

  pipe_ = std::move(pipe);
  watcher_ = std::make_unique<mojo::SimpleWatcher>(
      FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL);
  watcher_->Watch(
      pipe_.get(), MOJO_HANDLE_SIGNAL_WRITABLE, MOJO_WATCH_CONDITION_SATISFIED,
      base::BindRepeating(&MultipartDataPipeGetter::MojoReadyCallback,
                          base::Unretained(this)));

  Write();
}

void MultipartDataPipeGetter::Clone(
    mojo::PendingReceiver<network::mojom::DataPipeGetter> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void MultipartDataPipeGetter::Reset() {
  watcher_.reset();
  pipe_.reset();
  write_position_ = 0;
}

std::unique_ptr<base::MemoryMappedFile> MultipartDataPipeGetter::ReleaseFile() {
  return std::move(file_);
}

void MultipartDataPipeGetter::MojoReadyCallback(
    MojoResult result,
    const mojo::HandleSignalsState& state) {
  Write();
}

void MultipartDataPipeGetter::Write() {
  if (write_position_ == 0)
    write_start_time_ = base::TimeTicks::Now();

  int64_t metadata_end = metadata_.size();
  if (0 <= write_position_ && write_position_ < metadata_end) {
    if (!WriteString(metadata_, write_position_))
      return;
  }

  int64_t data_end = metadata_end + file_->length();
  if (metadata_end <= write_position_ && write_position_ < data_end) {
    if (!WriteFileData())
      return;
  }

  int64_t last_boundary_end = data_end + last_boundary_.size();
  DCHECK_EQ(last_boundary_end, FullSize());
  if (data_end <= write_position_ && write_position_ < last_boundary_end) {
    int64_t offset = write_position_ - data_end;
    if (!WriteString(last_boundary_, offset))
      return;
  }

  if (write_position_ == FullSize()) {
    base::TimeDelta write_duration = base::TimeTicks::Now() - write_start_time_;
    base::UmaHistogramCustomTimes("Enterprise.MultipartDataPipe.WriteDuration",
                                  write_duration,
                                  base::TimeDelta::FromMilliseconds(1),
                                  base::TimeDelta::FromMinutes(5), 50);
    int64_t bps =
        (1000 * FullSize()) /
        std::max(1, static_cast<int>(write_duration.InMilliseconds()));
    int64_t kbps = bps / 1024;
    base::UmaHistogramCustomCounts("Enterprise.MultipartDataPipe.WriteRate",
                                   kbps,
                                   /*min*/ 0,
                                   /*max*/ 100 * 1024, /*buckets*/ 50);
    Reset();
  }
}

bool MultipartDataPipeGetter::WriteString(const std::string& str,
                                          int64_t offset) {
  CHECK_GE(offset, 0);
  CHECK_LT(offset, static_cast<int64_t>(str.size()));

  return Write(str.data(), str.size(), offset);
}

bool MultipartDataPipeGetter::WriteFileData() {
  int64_t file_offset = write_position_ - metadata_.size();
  CHECK_GE(file_offset, 0);
  CHECK_LT(file_offset, static_cast<int64_t>(file_->length()));

  return Write(reinterpret_cast<const char*>(file_->data()), file_->length(),
               file_offset);
}

bool MultipartDataPipeGetter::Write(const char* data,
                                    int64_t full_size,
                                    int64_t offset) {
  while (true) {
    int64_t remaining_bytes = full_size - offset;
    uint32_t write_size =
        static_cast<uint32_t>(std::min(kMaxSize, remaining_bytes));
    if (write_size == 0) {
      // The data is fully read, so allow the next Write.
      return true;
    }

    int result =
        pipe_->WriteData(data + offset, &write_size, MOJO_WRITE_DATA_FLAG_NONE);
    if (result == MOJO_RESULT_SHOULD_WAIT) {
      watcher_->ArmOrNotify();
      return false;
    } else if (result != MOJO_RESULT_OK) {
      Reset();
      return false;
    }

    offset += write_size;
    write_position_ += write_size;
    DCHECK_LE(offset, full_size);
  }
}

int64_t MultipartDataPipeGetter::FullSize() {
  return metadata_.size() + file_->length() + last_boundary_.size();
}

}  // namespace safe_browsing
