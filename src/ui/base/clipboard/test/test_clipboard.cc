// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/clipboard/test/test_clipboard.h"

#include <stddef.h>
#include <memory>
#include <utility>
#include "base/containers/contains.h"
#include "base/memory/ptr_util.h"
#include "base/notreached.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "build/buildflag.h"
#include "build/chromecast_buildflags.h"
#include "build/chromeos_buildflags.h"
#include "skia/ext/skia_utils_base.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/clipboard_constants.h"
#include "ui/base/clipboard/clipboard_monitor.h"
#include "ui/base/clipboard/custom_data_helper.h"
#include "ui/base/data_transfer_policy/data_transfer_endpoint.h"
#include "ui/base/data_transfer_policy/data_transfer_policy_controller.h"
#include "ui/base/ui_base_features.h"
#include "ui/gfx/codec/png_codec.h"

namespace ui {

namespace {
bool IsReadAllowed(const DataTransferEndpoint* src,
                   const DataTransferEndpoint* dst) {
  auto* policy_controller = DataTransferPolicyController::Get();
  if (!policy_controller)
    return true;
  return policy_controller->IsClipboardReadAllowed(src, dst, absl::nullopt);
}
}  // namespace

TestClipboard::TestClipboard()
    : default_store_buffer_(ClipboardBuffer::kCopyPaste) {}

TestClipboard::~TestClipboard() = default;

TestClipboard* TestClipboard::CreateForCurrentThread() {
  base::AutoLock lock(Clipboard::ClipboardMapLock());
  auto* clipboard = new TestClipboard;
  (*Clipboard::ClipboardMapPtr())[base::PlatformThread::CurrentId()] =
      base::WrapUnique(clipboard);
  return clipboard;
}

void TestClipboard::SetLastModifiedTime(const base::Time& time) {
  last_modified_time_ = time;
}

void TestClipboard::OnPreShutdown() {}

DataTransferEndpoint* TestClipboard::GetSource(ClipboardBuffer buffer) const {
  return GetStore(buffer).GetDataSource();
}

const ClipboardSequenceNumberToken& TestClipboard::GetSequenceNumber(
    ClipboardBuffer buffer) const {
  return GetStore(buffer).sequence_number;
}

bool TestClipboard::IsFormatAvailable(
    const ClipboardFormatType& format,
    ClipboardBuffer buffer,
    const ui::DataTransferEndpoint* data_dst) const {
  if (!IsReadAllowed(GetStore(buffer).data_src.get(), data_dst))
    return false;
#if defined(OS_LINUX) || defined(OS_CHROMEOS)
  // The linux clipboard treats the presence of text on the clipboard
  // as the url format being available.
  if (format == ClipboardFormatType::UrlType())
    return IsFormatAvailable(ClipboardFormatType::PlainTextType(), buffer,
                             data_dst);
#endif  // defined(OS_LINUX) || defined(OS_CHROMEOS)
  const DataStore& store = GetStore(buffer);
  if (format == ClipboardFormatType::FilenamesType())
    return !store.filenames.empty();
  // Chrome can retrieve an image from the clipboard as either a bitmap or PNG.
  if (format == ClipboardFormatType::PngType() ||
      format == ClipboardFormatType::BitmapType()) {
    return base::Contains(store.data, ClipboardFormatType::PngType()) ||
           base::Contains(store.data, ClipboardFormatType::BitmapType());
  }
  return base::Contains(store.data, format);
}

void TestClipboard::Clear(ClipboardBuffer buffer) {
  GetStore(buffer).Clear();
}

void TestClipboard::ReadAvailableTypes(
    ClipboardBuffer buffer,
    const DataTransferEndpoint* data_dst,
    std::vector<std::u16string>* types) const {
  DCHECK(types);
  types->clear();
  const DataStore& store = GetStore(buffer);
  if (!IsReadAllowed(store.data_src.get(), data_dst))
    return;

  if (IsFormatAvailable(ClipboardFormatType::PlainTextType(), buffer,
                        data_dst)) {
    types->push_back(base::UTF8ToUTF16(kMimeTypeText));
#if defined(OS_LINUX) && !BUILDFLAG(IS_CHROMEOS_ASH) && \
    !BUILDFLAG(IS_CHROMECAST) && !BUILDFLAG(IS_CHROMEOS_LACROS)
    // This additional mime type is required as both Ozone/X11 and Ozone/Wayland
    // clipboards convert text/plain[;charset=utf-8] <=> [UTF8_]STRING to allow
    // interoperability with other applications which do not use the same mime
    // types as chrome.
    // TODO(https://crbug.com/1096425): remove this if condition once Ozone is
    // the only path in Linux builds.
    if (features::IsUsingOzonePlatform())
      types->push_back(base::UTF8ToUTF16(kMimeTypeTextUtf8));
#endif
  }
  if (IsFormatAvailable(ClipboardFormatType::HtmlType(), buffer, data_dst))
    types->push_back(base::UTF8ToUTF16(kMimeTypeHTML));

  if (IsFormatAvailable(ClipboardFormatType::RtfType(), buffer, data_dst))
    types->push_back(base::UTF8ToUTF16(kMimeTypeRTF));
  if (IsFormatAvailable(ClipboardFormatType::PngType(), buffer, data_dst) ||
      IsFormatAvailable(ClipboardFormatType::BitmapType(), buffer, data_dst))
    types->push_back(base::UTF8ToUTF16(kMimeTypePNG));
  if (IsFormatAvailable(ClipboardFormatType::FilenamesType(), buffer, data_dst))
    types->push_back(base::UTF8ToUTF16(kMimeTypeURIList));

  auto it = store.data.find(ClipboardFormatType::WebCustomDataType());
  if (it != store.data.end())
    ReadCustomDataTypes(it->second.c_str(), it->second.size(), types);
}

std::vector<std::u16string>
TestClipboard::ReadAvailablePlatformSpecificFormatNames(
    ClipboardBuffer buffer,
    const ui::DataTransferEndpoint* data_dst) const {
  const DataStore& store = GetStore(buffer);
  if (!IsReadAllowed(store.data_src.get(), data_dst))
    return {};

  const auto& data = store.data;
  std::vector<std::u16string> types;
  types.reserve(data.size());
  std::map<std::string, std::string> custom_format_names =
      ExtractCustomPlatformNames(buffer, data_dst);
  for (const auto& item : custom_format_names)
    types.push_back(base::UTF8ToUTF16(item.first));
  for (const auto& it : data) {
    std::string format_type = it.first.GetName();
    if (!format_type.empty()) {
      types.push_back(base::UTF8ToUTF16(format_type));
    }
  }

  return types;
}

void TestClipboard::ReadText(ClipboardBuffer buffer,
                             const DataTransferEndpoint* data_dst,
                             std::u16string* result) const {
  if (!IsReadAllowed(GetStore(buffer).data_src.get(), data_dst))
    return;

  std::string result8;
  ReadAsciiText(buffer, data_dst, &result8);
  *result = base::UTF8ToUTF16(result8);
}

// TODO(crbug.com/1103215): |data_dst| should be supported.
void TestClipboard::ReadAsciiText(ClipboardBuffer buffer,
                                  const DataTransferEndpoint* data_dst,
                                  std::string* result) const {
  const DataStore& store = GetStore(buffer);
  if (!IsReadAllowed(store.data_src.get(), data_dst))
    return;

  result->clear();
  auto it = store.data.find(ClipboardFormatType::PlainTextType());
  if (it != store.data.end())
    *result = it->second;
}

void TestClipboard::ReadHTML(ClipboardBuffer buffer,
                             const DataTransferEndpoint* data_dst,
                             std::u16string* markup,
                             std::string* src_url,
                             uint32_t* fragment_start,
                             uint32_t* fragment_end) const {
  const DataStore& store = GetStore(buffer);
  if (!IsReadAllowed(store.data_src.get(), data_dst))
    return;

  markup->clear();
  src_url->clear();
  auto it = store.data.find(ClipboardFormatType::HtmlType());
  if (it != store.data.end())
    *markup = base::UTF8ToUTF16(it->second);
  *src_url = store.html_src_url;
  *fragment_start = 0;
  *fragment_end = base::checked_cast<uint32_t>(markup->size());
}

void TestClipboard::ReadSvg(ClipboardBuffer buffer,
                            const DataTransferEndpoint* data_dst,
                            std::u16string* result) const {
  const DataStore& store = GetStore(buffer);
  if (!IsReadAllowed(store.data_src.get(), data_dst))
    return;

  result->clear();
  auto it = store.data.find(ClipboardFormatType::SvgType());
  if (it != store.data.end())
    *result = base::UTF8ToUTF16(it->second);
}

void TestClipboard::ReadRTF(ClipboardBuffer buffer,
                            const DataTransferEndpoint* data_dst,
                            std::string* result) const {
  const DataStore& store = GetStore(buffer);
  if (!IsReadAllowed(store.data_src.get(), data_dst))
    return;

  result->clear();
  auto it = store.data.find(ClipboardFormatType::RtfType());
  if (it != store.data.end())
    *result = it->second;
}

void TestClipboard::ReadPng(ClipboardBuffer buffer,
                            const DataTransferEndpoint* data_dst,
                            ReadPngCallback callback) const {
  const DataStore& store = GetStore(buffer);
  if (!IsReadAllowed(store.data_src.get(), data_dst)) {
    std::move(callback).Run(std::vector<uint8_t>());
    return;
  }
  std::move(callback).Run(store.png);
}

void TestClipboard::ReadImage(ClipboardBuffer buffer,
                              const DataTransferEndpoint* data_dst,
                              ReadImageCallback callback) const {
  const DataStore& store = GetStore(buffer);
  if (!IsReadAllowed(store.data_src.get(), data_dst)) {
    std::move(callback).Run(SkBitmap());
    return;
  }
  SkBitmap bitmap;
  gfx::PNGCodec::Decode(store.png.data(), store.png.size(), &bitmap);
  std::move(callback).Run(bitmap);
}

// TODO(crbug.com/1103215): |data_dst| should be supported.
void TestClipboard::ReadCustomData(ClipboardBuffer buffer,
                                   const std::u16string& type,
                                   const DataTransferEndpoint* data_dst,
                                   std::u16string* result) const {}

void TestClipboard::ReadFilenames(ClipboardBuffer buffer,
                                  const DataTransferEndpoint* data_dst,
                                  std::vector<ui::FileInfo>* result) const {
  const DataStore& store = GetStore(buffer);
  if (!IsReadAllowed(store.data_src.get(), data_dst))
    return;

  *result = store.filenames;
}

// TODO(crbug.com/1103215): |data_dst| should be supported.
void TestClipboard::ReadBookmark(const DataTransferEndpoint* data_dst,
                                 std::u16string* title,
                                 std::string* url) const {
  const DataStore& store = GetDefaultStore();
  if (!IsReadAllowed(store.data_src.get(), data_dst))
    return;

  if (url) {
    auto it = store.data.find(ClipboardFormatType::UrlType());
    if (it != store.data.end())
      *url = it->second;
  }
  if (title)
    *title = base::UTF8ToUTF16(store.url_title);
}

void TestClipboard::ReadData(const ClipboardFormatType& format,
                             const DataTransferEndpoint* data_dst,
                             std::string* result) const {
  const DataStore& store = GetDefaultStore();
  if (!IsReadAllowed(store.data_src.get(), data_dst))
    return;

  result->clear();
  auto it = store.data.find(format);
  if (it != store.data.end())
    *result = it->second;
}

base::Time TestClipboard::GetLastModifiedTime() const {
  return last_modified_time_;
}

void TestClipboard::ClearLastModifiedTime() {
  last_modified_time_ = base::Time();
}

#if defined(USE_OZONE)
bool TestClipboard::IsSelectionBufferAvailable() const {
  return true;
}
#endif  // defined(USE_OZONE)

void TestClipboard::WritePortableAndPlatformRepresentations(
    ClipboardBuffer buffer,
    const ObjectMap& objects,
    std::vector<Clipboard::PlatformRepresentation> platform_representations,
    std::unique_ptr<DataTransferEndpoint> data_src) {
  Clear(buffer);
  default_store_buffer_ = buffer;

  DispatchPlatformRepresentations(std::move(platform_representations));
  for (const auto& kv : objects)
    DispatchPortableRepresentation(kv.first, kv.second);
  default_store_buffer_ = ClipboardBuffer::kCopyPaste;
  GetStore(buffer).SetDataSource(std::move(data_src));
}

void TestClipboard::WriteText(const char* text_data, size_t text_len) {
  std::string text(text_data, text_len);
  GetDefaultStore().data[ClipboardFormatType::PlainTextType()] = text;
#if defined(OS_WIN)
  // Create a dummy entry.
  GetDefaultStore().data[ClipboardFormatType::PlainTextAType()];
#endif
  if (IsSupportedClipboardBuffer(ClipboardBuffer::kSelection))
    GetStore(ClipboardBuffer::kSelection)
        .data[ClipboardFormatType::PlainTextType()] = text;
  ClipboardMonitor::GetInstance()->NotifyClipboardDataChanged();
}

void TestClipboard::WriteHTML(const char* markup_data,
                              size_t markup_len,
                              const char* url_data,
                              size_t url_len) {
  std::u16string markup;
  base::UTF8ToUTF16(markup_data, markup_len, &markup);
  GetDefaultStore().data[ClipboardFormatType::HtmlType()] =
      base::UTF16ToUTF8(markup);
  GetDefaultStore().html_src_url = std::string(url_data, url_len);
}

void TestClipboard::WriteSvg(const char* markup_data, size_t markup_len) {
  std::u16string markup;
  base::UTF8ToUTF16(markup_data, markup_len, &markup);
  GetDefaultStore().data[ClipboardFormatType::SvgType()] =
      base::UTF16ToUTF8(markup);
}

void TestClipboard::WriteRTF(const char* rtf_data, size_t data_len) {
  GetDefaultStore().data[ClipboardFormatType::RtfType()] =
      std::string(rtf_data, data_len);
}

void TestClipboard::WriteFilenames(std::vector<ui::FileInfo> filenames) {
  GetDefaultStore().filenames = std::move(filenames);
}

void TestClipboard::WriteBookmark(const char* title_data,
                                  size_t title_len,
                                  const char* url_data,
                                  size_t url_len) {
  GetDefaultStore().data[ClipboardFormatType::UrlType()] =
      std::string(url_data, url_len);
  GetDefaultStore().url_title = std::string(title_data, title_len);
}

void TestClipboard::WriteWebSmartPaste() {
  // Create a dummy entry.
  GetDefaultStore().data[ClipboardFormatType::WebKitSmartPasteType()];
}

void TestClipboard::WriteBitmap(const SkBitmap& bitmap) {
  // We expect callers to sanitize `bitmap` to be N32 color type, to avoid
  // out-of-bounds issues due to unexpected bits-per-pixel while copying the
  // bitmap's pixel buffer. This DCHECK is to help alert us if we've missed
  // something.
  DCHECK_EQ(bitmap.colorType(), kN32_SkColorType);

  // Create a dummy entry.
  GetDefaultStore().data[ClipboardFormatType::BitmapType()];
  gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, false, &GetDefaultStore().png);
  ClipboardMonitor::GetInstance()->NotifyClipboardDataChanged();
}

void TestClipboard::WriteData(const ClipboardFormatType& format,
                              const char* data_data,
                              size_t data_len) {
  GetDefaultStore().data[format] = std::string(data_data, data_len);
}

TestClipboard::DataStore::DataStore() = default;

TestClipboard::DataStore::DataStore(const DataStore& other) {
  sequence_number = other.sequence_number;
  data = other.data;
  url_title = other.url_title;
  html_src_url = other.html_src_url;
  png = other.png;
  data_src = other.data_src ? std::make_unique<DataTransferEndpoint>(
                                  DataTransferEndpoint(*(other.data_src)))
                            : nullptr;
}

TestClipboard::DataStore& TestClipboard::DataStore::operator=(
    const DataStore& other) {
  sequence_number = other.sequence_number;
  data = other.data;
  url_title = other.url_title;
  html_src_url = other.html_src_url;
  png = other.png;
  data_src = other.data_src ? std::make_unique<DataTransferEndpoint>(
                                  DataTransferEndpoint(*(other.data_src)))
                            : nullptr;
  return *this;
}

TestClipboard::DataStore::~DataStore() = default;

void TestClipboard::DataStore::Clear() {
  data.clear();
  url_title.clear();
  html_src_url.clear();
  png.clear();
  filenames.clear();
  data_src.reset();
}

void TestClipboard::DataStore::SetDataSource(
    std::unique_ptr<DataTransferEndpoint> data_src) {
  this->data_src = std::move(data_src);
}

DataTransferEndpoint* TestClipboard::DataStore::GetDataSource() const {
  return this->data_src.get();
}

const TestClipboard::DataStore& TestClipboard::GetStore(
    ClipboardBuffer buffer) const {
  CHECK(IsSupportedClipboardBuffer(buffer));
  return stores_[buffer];
}

TestClipboard::DataStore& TestClipboard::GetStore(ClipboardBuffer buffer) {
  CHECK(IsSupportedClipboardBuffer(buffer));
  DataStore& store = stores_[buffer];
  store.sequence_number = ClipboardSequenceNumberToken();
  return store;
}

const TestClipboard::DataStore& TestClipboard::GetDefaultStore() const {
  return GetStore(default_store_buffer_);
}

TestClipboard::DataStore& TestClipboard::GetDefaultStore() {
  return GetStore(default_store_buffer_);
}

}  // namespace ui
