// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/holding_space/holding_space_downloads_delegate.h"

#include "ash/constants/ash_features.h"
#include "ash/public/cpp/holding_space/holding_space_constants.h"
#include "ash/public/cpp/holding_space/holding_space_progress.h"
#include "ash/public/cpp/image_util.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/style/ash_color_provider.h"
#include "base/containers/contains.h"
#include "base/containers/cxx20_erase.h"
#include "chrome/browser/ash/crosapi/crosapi_ash.h"
#include "chrome/browser/ash/crosapi/crosapi_manager.h"
#include "chrome/browser/ash/file_manager/path_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/ash/holding_space/holding_space_util.h"
#include "components/download/public/common/download_item.h"
#include "components/download/public/common/simple_download_manager.h"
#include "components/vector_icons/vector_icons.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/download_item_utils.h"
#include "content/public/browser/download_manager.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/text/bytes_formatting.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/paint_vector_icon.h"

namespace ash {

namespace {

// Helpers ---------------------------------------------------------------------

// Returns a mojo download state converted from the specified item `state`.
crosapi::mojom::DownloadState ConvertToMojoDownloadState(
    download::DownloadItem::DownloadState state) {
  switch (state) {
    case download::DownloadItem::IN_PROGRESS:
      return crosapi::mojom::DownloadState::kInProgress;
    case download::DownloadItem::COMPLETE:
      return crosapi::mojom::DownloadState::kComplete;
    case download::DownloadItem::CANCELLED:
      return crosapi::mojom::DownloadState::kCancelled;
    case download::DownloadItem::INTERRUPTED:
      return crosapi::mojom::DownloadState::kInterrupted;
    case download::DownloadItem::MAX_DOWNLOAD_STATE:
      return crosapi::mojom::DownloadState::kUnknown;
  }
}

// Returns a mojo download item converted from the specified `item`.
crosapi::mojom::DownloadItemPtr ConvertToMojoDownloadItem(
    const download::DownloadItem* item) {
  auto mojo_download_item = crosapi::mojom::DownloadItem::New();
  mojo_download_item->guid = item->GetGuid();
  mojo_download_item->state = ConvertToMojoDownloadState(item->GetState());
  mojo_download_item->target_file_path = item->GetTargetFilePath();
  mojo_download_item->full_path = item->GetFullPath();
  mojo_download_item->is_paused = item->IsPaused();
  mojo_download_item->has_is_paused = true;
  mojo_download_item->open_when_complete = item->GetOpenWhenComplete();
  mojo_download_item->has_open_when_complete = true;
  mojo_download_item->received_bytes = item->GetReceivedBytes();
  mojo_download_item->has_received_bytes = true;
  mojo_download_item->total_bytes = item->GetTotalBytes();
  mojo_download_item->has_total_bytes = true;
  mojo_download_item->start_time = item->GetStartTime();
  mojo_download_item->is_dangerous = item->IsDangerous();
  mojo_download_item->has_is_dangerous = true;
  mojo_download_item->is_mixed_content = item->IsMixedContent();
  mojo_download_item->has_is_mixed_content = true;

  // NOTE: `browser_context` may be `nullptr` in tests. We assume in this case
  // that the item is not from an incognito profile; tests exercising incognito
  // behavior should make sure `browser_context` is not `nullptr`.
  auto* browser_context = content::DownloadItemUtils::GetBrowserContext(item);
  mojo_download_item->is_from_incognito_profile =
      browser_context
          ? Profile::FromBrowserContext(browser_context)->IsIncognitoProfile()
          : false;

  return mojo_download_item;
}

// Returns a `gfx::ImageSkia` to show as a placeholder in a holding space image
// to indicate error.
gfx::ImageSkia CreateErrorPlaceholderImageSkia(
    const gfx::Size& size,
    const absl::optional<bool>& dark_background) {
  DCHECK_GE(size.width(), kHoldingSpaceIconSize);
  DCHECK_GE(size.height(), kHoldingSpaceIconSize);
  return gfx::ImageSkiaOperations::CreateSuperimposedImage(
      image_util::CreateEmptyImage(size),
      gfx::CreateVectorIcon(
          vector_icons::kErrorOutlineIcon, kHoldingSpaceIconSize,
          dark_background.value_or(AshColorProvider::Get()->IsDarkModeEnabled())
              ? gfx::kGoogleRed300
              : gfx::kGoogleRed600));
}

// Returns the singleton `crosapi::DownloadControllerAsh` if it exists.
crosapi::DownloadControllerAsh* GetDownloadControllerAsh() {
  return crosapi::CrosapiManager::IsInitialized()
             ? crosapi::CrosapiManager::Get()
                   ->crosapi_ash()
                   ->download_controller_ash()
             : nullptr;
}

// Returns whether the specified `mojo_download_item` is complete.
bool IsComplete(const crosapi::mojom::DownloadItem* mojo_download_item) {
  return mojo_download_item->state == crosapi::mojom::DownloadState::kComplete;
}

// Returns whether or not the specified `mojo_download_item` is eligible for
// in-progress downloads integration.
bool IsEligibleForInProgressIntegration(
    const crosapi::mojom::DownloadItem* mojo_download_item) {
  // The `has_is_mixed_content` field was the last field to be implemented in
  // Lacros. Its presence indicates that other required metadata and APIs (e.g.
  // pause, resume, cancel, etc.) are also implemented and is therefore used to
  // gate eligibility.
  return mojo_download_item->has_is_mixed_content;
}

// Returns whether the specified `mojo_download_item` is in progress.
bool IsInProgress(const crosapi::mojom::DownloadItem* mojo_download_item) {
  return mojo_download_item->state ==
         crosapi::mojom::DownloadState::kInProgress;
}

// Returns whether the specified `download_item` is in progress.
bool IsInProgress(const download::DownloadItem* download_item) {
  return download_item->GetState() == download::DownloadItem::IN_PROGRESS;
}

}  // namespace

// HoldingSpaceDownloadsDelegate::InProgressDownload ---------------------------

// A wrapper around an in-progress `crosapi::mojom::DownloadItem` which notifies
// its associated `delegate_` of changes in download state. Each in-progress
// download is associated with a single in-progress holding space item once the
// target file path for the in-progress download has been set. NOTE: Instances
// of this class are immediately destroyed when the underlying download is no
// longer in-progress or when the associated in-progress holding space item is
// removed from the model.
class HoldingSpaceDownloadsDelegate::InProgressDownload {
 public:
  enum class Type {
    kAsh,     // See `InProgressAshDownload`.
    kLacros,  // See `InProgressLacrosDownload`.
  };

  InProgressDownload(Type type,
                     HoldingSpaceDownloadsDelegate* delegate,
                     crosapi::mojom::DownloadItemPtr mojo_download_item)
      : type_(type),
        delegate_(delegate),
        mojo_download_item_(std::move(mojo_download_item)) {
    DCHECK(IsInProgress(mojo_download_item_.get()));
  }

  InProgressDownload(const InProgressDownload&) = delete;
  InProgressDownload& operator=(const InProgressDownload&) = delete;
  virtual ~InProgressDownload() = default;

  // Returns the specific type of `InProgressDownload` that this is.
  Type GetType() const { return type_; }

  // Cancels the underlying download. NOTE: This is expected to be invoked in
  // direct response to an explicit user action and will result in the
  // destruction of `this`.
  virtual void Cancel() = 0;

  // Pauses the underlying download.
  virtual void Pause() = 0;

  // Resumes the underlying download. NOTE: This is expected to be invoked in
  // direct response to an explicit user action.
  virtual void Resume() = 0;

  // Marks the underlying download to be opened when complete.
  virtual void OpenWhenComplete() = 0;

  // Returns the file path associated with the underlying download.
  // NOTE: The file path may be empty before a target file path has been picked.
  base::FilePath GetFilePath() const {
    return mojo_download_item_->full_path.value_or(base::FilePath());
  }

  // Returns the GUID which uniquely identifies the underlying download.
  // NOTE: The existence of `this` implies that GUID is present.
  const std::string& GetGuid() const {
    return mojo_download_item_->guid.value();
  }

  // Returns the target file path associated with the underlying download.
  // NOTE: Returned path may be empty before a target file path has been picked.
  const base::FilePath& GetTargetFilePath() const {
    return mojo_download_item_->target_file_path;
  }

  // Returns the current progress of the underlying download.
  // NOTE: Progress is hidden if the download is dangerous or mixed content.
  HoldingSpaceProgress GetProgress() const {
    if (IsComplete(mojo_download_item_.get()))
      return HoldingSpaceProgress();
    return HoldingSpaceProgress(GetReceivedBytes(), GetTotalBytes(),
                                /*complete=*/false,
                                /*hidden=*/IsDangerous() || IsMixedContent());
  }

  // Returns the number of bytes received for the underlying download.
  int64_t GetReceivedBytes() const {
    return mojo_download_item_->received_bytes;
  }

  // Returns the number of total bytes for the underlying download.
  // NOTE: The total number of bytes will be absent if unknown or indeterminate.
  absl::optional<int64_t> GetTotalBytes() const {
    const int64_t total_bytes = mojo_download_item_->total_bytes;
    return total_bytes > 0 ? absl::make_optional(total_bytes) : absl::nullopt;
  }

  // Returns whether the underlying download is dangerous.
  bool IsDangerous() const { return mojo_download_item_->is_dangerous; }

  // Returns whether the underlying download is mixed content.
  bool IsMixedContent() const { return mojo_download_item_->is_mixed_content; }

  // Returns whether the underlying download is paused.
  bool IsPaused() const { return mojo_download_item_->is_paused; }

  // Associates this in-progress download with the specified in-progress
  // `holding_space_item`. NOTE: This association may be performed only once.
  void SetHoldingSpaceItem(const HoldingSpaceItem* holding_space_item) {
    DCHECK(!holding_space_item_);
    holding_space_item_ = holding_space_item;
  }

  // Returns the in-progress `holding_space_item_` associated with this
  // in-progress download. NOTE: This may be `nullptr` if no holding space item
  // has yet been associated with the in-progress download.
  const HoldingSpaceItem* GetHoldingSpaceItem() const {
    return holding_space_item_;
  }

  // Returns a resolver which creates a `gfx::ImageSkia` placeholder
  // corresponding to the file type of the associated *target* file path, rather
  // than the *backing* file path, when a thumbnail cannot be generated. Note
  // that if the download is dangerous or is mixed content, a placeholder
  // indicating error will be returned.
  HoldingSpaceImage::PlaceholderImageSkiaResolver
  GetPlaceholderImageSkiaResolver() const {
    return base::BindRepeating(
        [](const base::WeakPtr<InProgressDownload>& in_progress_download,
           const base::FilePath& file_path, const gfx::Size& size,
           const absl::optional<bool>& dark_background,
           const absl::optional<bool>& is_folder) {
          if (in_progress_download &&
              (in_progress_download->IsDangerous() ||
               in_progress_download->IsMixedContent())) {
            return CreateErrorPlaceholderImageSkia(size, dark_background);
          }

          base::FilePath rewritten_file_path(file_path);
          if (in_progress_download &&
              IsInProgress(in_progress_download->mojo_download_item_.get())) {
            rewritten_file_path = in_progress_download->GetTargetFilePath();
          }

          return HoldingSpaceImage::CreateDefaultPlaceholderImageSkiaResolver()
              .Run(rewritten_file_path, size, dark_background, is_folder);
        },
        weak_factory_.GetWeakPtr());
  }

  // Returns the text to display for the underlying download.
  absl::optional<std::u16string> GetText() const {
    // Only in-progress download items override primary text. In other cases,
    // the primary text will fall back to the lossy display name of the backing
    // file and be automatically updated in response to file system changes.
    if (!IsInProgress(mojo_download_item_.get()))
      return absl::nullopt;
    return mojo_download_item_->target_file_path.BaseName().LossyDisplayName();
  }

  // Returns the secondary text to display for the underlying download.
  absl::optional<std::u16string> GetSecondaryText() const {
    // Only in-progress download items have secondary text.
    if (!IsInProgress(mojo_download_item_.get()))
      return absl::nullopt;

    // In-progress download items which are dangerous or mixed content have a
    // special secondary text treatment.
    if (IsDangerous() || IsMixedContent()) {
      return l10n_util::GetStringUTF16(
          IDS_ASH_HOLDING_SPACE_IN_PROGRESS_DOWNLOAD_DANGEROUS_FILE);
    }

    // In-progress download items which are marked to be opened when complete
    // and are not paused have a special secondary text treatment.
    if (mojo_download_item_->open_when_complete && !IsPaused()) {
      return l10n_util::GetStringUTF16(
          IDS_ASH_HOLDING_SPACE_IN_PROGRESS_DOWNLOAD_OPEN_WHEN_COMPLETE);
    }

    const int64_t received_bytes = GetReceivedBytes();
    const absl::optional<int64_t> total_bytes = GetTotalBytes();

    std::u16string secondary_text;
    if (total_bytes.has_value()) {
      // If `total_bytes` is known, `secondary_text` will be something of the
      // form "10/100 MB", where the first number is the number of received
      // bytes and the second number is the total number of bytes expected.
      const ui::DataUnits units = ui::GetByteDisplayUnits(total_bytes.value());
      secondary_text = l10n_util::GetStringFUTF16(
          IDS_ASH_HOLDING_SPACE_IN_PROGRESS_DOWNLOAD_SIZE_INFO,
          ui::FormatBytesWithUnits(received_bytes, units, /*show_units=*/false),
          ui::FormatBytesWithUnits(total_bytes.value(), units,
                                   /*show_units=*/true));
    } else {
      // If `total_bytes` is not known, `secondary_text` will be something of
      // the form "10 MB", indicating only the number of received bytes.
      secondary_text = ui::FormatBytes(received_bytes);
    }

    if (IsPaused()) {
      // If the `item` is paused, prepend "Paused, " to the `secondary_text`
      // such that the string is of the form "Paused, 10/100 MB" or "Paused, 10
      // MB", depending on whether or not `total_bytes` is known.
      secondary_text = l10n_util::GetStringFUTF16(
          IDS_ASH_HOLDING_SPACE_IN_PROGRESS_DOWNLOAD_PAUSED_WITH_SIZE_INFO,
          secondary_text);
    }

    return secondary_text;
  }

 protected:
  // Updates the `mojo_download_item_` associated with this in-progress
  // download, notifying `delegate_` of the change in state. Note that invoking
  // this method may result in the destruction of `this`.
  void UpdateMojoDownloadItem(
      crosapi::mojom::DownloadItemPtr mojo_download_item) {
    const bool was_dangerous_or_mixed_content =
        IsDangerous() || IsMixedContent();

    mojo_download_item_ = std::move(mojo_download_item);

    if (!mojo_download_item_) {
      delegate_->OnDownloadFailed(this);  // NOTE: Destroys `this`.
      return;
    }

    // Explicitly invalidate the image of the associated holding space item if
    // the download is transitioning to/from a dangerous or mixed content state.
    const bool invalidate_image =
        was_dangerous_or_mixed_content != (IsDangerous() || IsMixedContent());

    switch (mojo_download_item_->state) {
      case crosapi::mojom::DownloadState::kInProgress:
        delegate_->OnDownloadUpdated(this, invalidate_image);
        break;
      case crosapi::mojom::DownloadState::kComplete:
        delegate_->OnDownloadCompleted(this);  // NOTE: Destroys `this`.
        break;
      case crosapi::mojom::DownloadState::kCancelled:
      case crosapi::mojom::DownloadState::kInterrupted:
        delegate_->OnDownloadFailed(this);  // NOTE: Destroys `this`.
        break;
      case crosapi::mojom::DownloadState::kUnknown:
        NOTREACHED();
        break;
    }
  }

 private:
  const Type type_;
  HoldingSpaceDownloadsDelegate* const delegate_;  // NOTE: Owns `this`.
  crosapi::mojom::DownloadItemPtr mojo_download_item_;

  // The in-progress holding space item associated with this in-progress
  // download. NOTE: This may be `nullptr` until the target file path for the
  // in-progress download has been set and a holding space item has been created
  // and associated.
  const HoldingSpaceItem* holding_space_item_ = nullptr;

  base::WeakPtrFactory<InProgressDownload> weak_factory_{this};
};

// HoldingSpaceDownloadsDelegate::InProgressAshDownload ------------------------

// A wrapper around an in-progress `download::DownloadItem` originating from the
// Ash Chrome browser. NOTE: Instances of this class are immediately destroyed
// when the underlying download is no longer in-progress or when the associated
// in-progress holding space item is removed from the model.
class HoldingSpaceDownloadsDelegate::InProgressAshDownload
    : public HoldingSpaceDownloadsDelegate::InProgressDownload,
      public download::DownloadItem::Observer {
 public:
  InProgressAshDownload(HoldingSpaceDownloadsDelegate* delegate,
                        content::DownloadManager* manager,
                        download::DownloadItem* download_item)
      : InProgressDownload(Type::kAsh,
                           delegate,
                           ConvertToMojoDownloadItem(download_item)),
        manager_(manager),
        download_item_(download_item) {
    download_item_observation_.Observe(download_item);
  }

  InProgressAshDownload(const InProgressAshDownload&) = delete;
  InProgressAshDownload& operator=(const InProgressAshDownload&) = delete;
  ~InProgressAshDownload() override = default;

  // Returns the download manager that originated this download.
  const content::DownloadManager* manager() const { return manager_; }

  // Returns the download item that this in-progress download wraps.
  const download::DownloadItem* download_item() const { return download_item_; }

 private:
  // InProgressDownload:
  void Cancel() override { download_item_->Cancel(/*from_user=*/true); }
  void Pause() override { download_item_->Pause(); }
  void Resume() override { download_item_->Resume(/*from_user=*/true); }

  void OpenWhenComplete() override {
    download_item_->SetOpenWhenComplete(true);
  }

  // download::DownloadItem::Observer:
  void OnDownloadUpdated(download::DownloadItem* download_item) override {
    // NOTE: This method invocation may result in destruction of `this`,
    // depending on the state of the underlying download.
    UpdateMojoDownloadItem(ConvertToMojoDownloadItem(download_item));
  }

  void OnDownloadDestroyed(download::DownloadItem* download_item) override {
    UpdateMojoDownloadItem(nullptr);  // NOTE: Destroys `this`.
  }

  content::DownloadManager* const manager_;
  download::DownloadItem* const download_item_;

  base::ScopedObservation<download::DownloadItem,
                          download::DownloadItem::Observer>
      download_item_observation_{this};
};

// HoldingSpaceDownloadsDelegate::InProgressLacrosDownload ---------------------

// A wrapper around an in-progress `crosapi::mojom::DownloadItem` originating
// from the Lacros Chrome browser. NOTE: Instances of this class are immediately
// destroyed when the underlying download is no longer in-progress or when the
// associated in-progress holding space item is removed from the model.
class HoldingSpaceDownloadsDelegate::InProgressLacrosDownload
    : public HoldingSpaceDownloadsDelegate::InProgressDownload,
      public crosapi::DownloadControllerAsh::DownloadControllerObserver {
 public:
  InProgressLacrosDownload(HoldingSpaceDownloadsDelegate* delegate,
                           crosapi::mojom::DownloadItemPtr mojo_download_item)
      : InProgressDownload(Type::kLacros,
                           delegate,
                           std::move(mojo_download_item)) {
    auto* const download_controller_ash = GetDownloadControllerAsh();
    if (download_controller_ash)
      download_controller_ash->AddObserver(this);
  }

  InProgressLacrosDownload(const InProgressLacrosDownload&) = delete;
  InProgressLacrosDownload& operator=(const InProgressLacrosDownload&) = delete;

  ~InProgressLacrosDownload() override {
    auto* const download_controller_ash = GetDownloadControllerAsh();
    if (download_controller_ash)
      download_controller_ash->RemoveObserver(this);
  }

 private:
  // InProgressDownload:
  void Cancel() override {
    auto* const download_controller_ash = GetDownloadControllerAsh();
    if (download_controller_ash)
      download_controller_ash->Cancel(GetGuid(), /*user_cancel=*/true);
  }

  void Pause() override {
    auto* const download_controller_ash = GetDownloadControllerAsh();
    if (download_controller_ash)
      download_controller_ash->Pause(GetGuid());
  }

  void Resume() override {
    auto* const download_controller_ash = GetDownloadControllerAsh();
    if (download_controller_ash)
      download_controller_ash->Resume(GetGuid(), /*user_resume=*/true);
  }

  void OpenWhenComplete() override {
    auto* const download_controller_ash = GetDownloadControllerAsh();
    if (download_controller_ash)
      download_controller_ash->SetOpenWhenComplete(GetGuid(), true);
  }

  // crosapi::DownloadControllerAsh::DownloadControllerObserver:
  void OnLacrosDownloadUpdated(
      const crosapi::mojom::DownloadItem& mojo_download_item) override {
    if (mojo_download_item.guid != GetGuid())
      return;
    // NOTE: This method invocation may result in destruction of `this`,
    // depending on the state of the underlying download.
    UpdateMojoDownloadItem(mojo_download_item.Clone());
  }

  void OnLacrosDownloadDestroyed(
      const crosapi::mojom::DownloadItem& mojo_download_item) override {
    if (mojo_download_item.guid == GetGuid())
      UpdateMojoDownloadItem(nullptr);  // NOTE: Destroys `this`.
  }
};

// HoldingSpaceDownloadsDelegate -----------------------------------------------

HoldingSpaceDownloadsDelegate::HoldingSpaceDownloadsDelegate(
    HoldingSpaceKeyedService* service,
    HoldingSpaceModel* model)
    : HoldingSpaceKeyedServiceDelegate(service, model) {}

HoldingSpaceDownloadsDelegate::~HoldingSpaceDownloadsDelegate() {
  // Lacros Chrome downloads.
  auto* const download_controller_ash = GetDownloadControllerAsh();
  if (download_controller_ash)
    download_controller_ash->RemoveObserver(this);
}

void HoldingSpaceDownloadsDelegate::Cancel(const HoldingSpaceItem* item) {
  DCHECK(HoldingSpaceItem::IsDownload(item->type()));
  for (const auto& in_progress_download : in_progress_downloads_) {
    if (in_progress_download->GetHoldingSpaceItem() == item) {
      in_progress_download->Cancel();
      return;
    }
  }
}

void HoldingSpaceDownloadsDelegate::Pause(const HoldingSpaceItem* item) {
  DCHECK(HoldingSpaceItem::IsDownload(item->type()));
  for (const auto& in_progress_download : in_progress_downloads_) {
    if (in_progress_download->GetHoldingSpaceItem() == item) {
      in_progress_download->Pause();
      return;
    }
  }
}

void HoldingSpaceDownloadsDelegate::Resume(const HoldingSpaceItem* item) {
  DCHECK(HoldingSpaceItem::IsDownload(item->type()));
  for (const auto& in_progress_download : in_progress_downloads_) {
    if (in_progress_download->GetHoldingSpaceItem() == item) {
      in_progress_download->Resume();
      return;
    }
  }
}

bool HoldingSpaceDownloadsDelegate::OpenWhenComplete(
    const HoldingSpaceItem* item) {
  DCHECK(HoldingSpaceItem::IsDownload(item->type()));
  for (const auto& in_progress_download : in_progress_downloads_) {
    if (in_progress_download->GetHoldingSpaceItem() == item) {
      in_progress_download->OpenWhenComplete();
      return true;
    }
  }
  return false;
}

void HoldingSpaceDownloadsDelegate::OnPersistenceRestored() {
  // ARC downloads.
  if (features::IsHoldingSpaceArcIntegrationEnabled()) {
    // NOTE: The `arc_intent_helper_bridge` may be `nullptr` if the `profile()`
    // is not allowed to use ARC, e.g. if the `profile()` is OTR.
    auto* const arc_intent_helper_bridge =
        arc::ArcIntentHelperBridge::GetForBrowserContext(profile());
    if (arc_intent_helper_bridge)
      arc_intent_helper_observation_.Observe(arc_intent_helper_bridge);
  }

  // Ash Chrome downloads.
  download_notifier_.AddProfile(profile());

  // Lacros Chrome downloads.
  auto* const download_controller_ash = GetDownloadControllerAsh();
  if (download_controller_ash) {
    download_controller_ash->GetAllDownloads(
        base::BindOnce(&HoldingSpaceDownloadsDelegate::OnLacrosDownloadsSynced,
                       weak_factory_.GetWeakPtr()));
  }
}

void HoldingSpaceDownloadsDelegate::OnHoldingSpaceItemsRemoved(
    const std::vector<const HoldingSpaceItem*>& items) {
  // If the user removes a holding space item associated with an in-progress
  // download, that in-progress download can be destroyed. The download will
  // continue, but it will no longer be associated with a holding space item.
  base::EraseIf(in_progress_downloads_, [&](const auto& in_progress_download) {
    return base::Contains(items, in_progress_download->GetHoldingSpaceItem());
  });
}

void HoldingSpaceDownloadsDelegate::OnManagerInitialized(
    content::DownloadManager* manager) {
  DCHECK(!is_restoring_persistence());
  download::SimpleDownloadManager::DownloadVector downloads;
  manager->GetAllDownloads(&downloads);
  for (auto* download : downloads)
    OnDownloadCreated(manager, download);
}

void HoldingSpaceDownloadsDelegate::OnManagerGoingDown(
    content::DownloadManager* manager) {
  // Collect all downloads associated with `manager`. These downloads will be
  // removed from `in_progress_downloads_` on failure, so they cannot be failed
  // in a loop that iterates over `in_progress_downloads_`.
  std::set<InProgressDownload*> downloads_to_remove;
  for (const auto& in_progress_download : in_progress_downloads_) {
    if (in_progress_download->GetType() != InProgressDownload::Type::kAsh)
      continue;

    if (static_cast<InProgressAshDownload*>(in_progress_download.get())
            ->manager() == manager) {
      downloads_to_remove.insert(in_progress_download.get());
    }
  }

  // Fail all of `manager`'s downloads.
  for (InProgressDownload* in_progress_download : downloads_to_remove)
    OnDownloadFailed(in_progress_download);
}

void HoldingSpaceDownloadsDelegate::OnDownloadCreated(
    content::DownloadManager* manager,
    download::DownloadItem* download_item) {
  DCHECK(!is_restoring_persistence());
  if (IsInProgress(download_item)) {
    in_progress_downloads_.emplace(
        std::make_unique<InProgressAshDownload>(this, manager, download_item));
  }
}

void HoldingSpaceDownloadsDelegate::OnDownloadUpdated(
    content::DownloadManager* manager,
    download::DownloadItem* download_item) {
  if (!IsInProgress(download_item))
    return;

  // In the common case, we already created an `InProgressDownload` for
  // `download_item` in `OnDownloadCreated()`.
  for (const auto& in_progress_download : in_progress_downloads_) {
    if (in_progress_download->GetType() == InProgressDownload::Type::kAsh &&
        static_cast<InProgressAshDownload*>(in_progress_download.get())
                ->download_item() == download_item) {
      return;
    }
  }

  // If `download_item` does not have an existing `InProgressDownload` (e.g.,
  // because it was interrupted and then resumed), create one.
  in_progress_downloads_.emplace(
      std::make_unique<InProgressAshDownload>(this, manager, download_item));
}

bool HoldingSpaceDownloadsDelegate::ShouldObserveProfile(Profile* profile) {
  return !profile->IsIncognitoProfile() ||
         features::IsHoldingSpaceIncognitoProfileIntegrationEnabled();
}

void HoldingSpaceDownloadsDelegate::OnArcDownloadAdded(
    const base::FilePath& relative_path,
    const std::string& owner_package_name) {
  DCHECK(features::IsHoldingSpaceArcIntegrationEnabled());
  if (is_restoring_persistence())
    return;

  // It is expected that `owner_package_name` be non-empty. Media files from
  // Chrome are synced to ARC via media scan and have `NULL` owning packages but
  // are expected *not* to have generated `OnArcDownloadAdded()` events.
  if (owner_package_name.empty()) {
    NOTREACHED();
    return;
  }

  // It is expected that `relative_path` always be contained within `Download/`
  // which refers to the public downloads folder for the current `profile()`.
  base::FilePath path(
      file_manager::util::GetDownloadsFolderForProfile(profile()));
  if (!base::FilePath("Download/").AppendRelativePath(relative_path, &path)) {
    NOTREACHED();
    return;
  }

  service()->AddDownload(HoldingSpaceItem::Type::kArcDownload, path);
}

void HoldingSpaceDownloadsDelegate::OnLacrosDownloadCreated(
    const crosapi::mojom::DownloadItem& mojo_download_item) {
  if (mojo_download_item.is_from_incognito_profile &&
      !features::IsHoldingSpaceIncognitoProfileIntegrationEnabled()) {
    return;
  }
  // NOTE: If ineligible for in-progress download handling, the download will
  // still be added to holding space on completion.
  if (IsInProgress(&mojo_download_item) &&
      IsEligibleForInProgressIntegration(&mojo_download_item)) {
    in_progress_downloads_.emplace(std::make_unique<InProgressLacrosDownload>(
        this, mojo_download_item.Clone()));
  }
}

void HoldingSpaceDownloadsDelegate::OnLacrosDownloadUpdated(
    const crosapi::mojom::DownloadItem& mojo_download_item) {
  if (mojo_download_item.is_from_incognito_profile &&
      !features::IsHoldingSpaceIncognitoProfileIntegrationEnabled()) {
    return;
  }
  // NOTE: It is only necessary to add a holding space item on completion here
  // if the download was ineligible for in-progress download handling.
  if (IsComplete(&mojo_download_item) &&
      !IsEligibleForInProgressIntegration(&mojo_download_item)) {
    service()->AddDownload(HoldingSpaceItem::Type::kLacrosDownload,
                           mojo_download_item.target_file_path);
  }
}

void HoldingSpaceDownloadsDelegate::OnLacrosDownloadsSynced(
    std::vector<crosapi::mojom::DownloadItemPtr> mojo_download_items) {
  // After the initial sync, observe updates to Lacros downloads.
  auto* const download_controller_ash = GetDownloadControllerAsh();
  if (download_controller_ash)
    download_controller_ash->AddObserver(this);

  // Sync `in_progress_downloads_` with `mojo_download_items` state.
  for (const auto& mojo_download_item : mojo_download_items)
    OnLacrosDownloadCreated(*mojo_download_item);
}

void HoldingSpaceDownloadsDelegate::OnDownloadUpdated(
    InProgressDownload* in_progress_download,
    bool invalidate_image) {
  // If in-progress downloads integration is disabled, a holding space item will
  // be associated with a download only upon download completion.
  if (!features::IsHoldingSpaceInProgressDownloadsIntegrationEnabled())
    return;

  // Downloads will not have an associated file path until the target file path
  // is set. In some cases, this requires the user to actively select the target
  // file path from a selection dialog. Only once file path information is set
  // should a holding space item be associated with the in-progress download.
  if (!in_progress_download->GetFilePath().empty())
    CreateOrUpdateHoldingSpaceItem(in_progress_download, invalidate_image);
}

void HoldingSpaceDownloadsDelegate::OnDownloadCompleted(
    InProgressDownload* in_progress_download) {
  // If in-progress downloads integration is enabled, a holding space item will
  // have already been associated with the download while it was in-progress.
  CreateOrUpdateHoldingSpaceItem(in_progress_download, true);
  EraseDownload(in_progress_download);
}

void HoldingSpaceDownloadsDelegate::OnDownloadFailed(
    const InProgressDownload* in_progress_download) {
  // If the `in_progress_download` resulted in the creation of a holding space
  // `item`, that `item` should be removed when the underlying download fails.
  const HoldingSpaceItem* item = in_progress_download->GetHoldingSpaceItem();
  if (item) {
    // NOTE: Removing `item` from the `model()` will result in the
    // `in_progress_download` being erased.
    model()->RemoveItem(item->id());
    DCHECK(!base::Contains(in_progress_downloads_, in_progress_download));
    return;
  }
  EraseDownload(in_progress_download);
}

void HoldingSpaceDownloadsDelegate::EraseDownload(
    const InProgressDownload* in_progress_download) {
  auto it = in_progress_downloads_.find(in_progress_download);
  DCHECK(it != in_progress_downloads_.end());
  in_progress_downloads_.erase(it);
}

void HoldingSpaceDownloadsDelegate::CreateOrUpdateHoldingSpaceItem(
    InProgressDownload* in_progress_download,
    bool invalidate_image) {
  const HoldingSpaceItem* item = in_progress_download->GetHoldingSpaceItem();

  // Create.
  if (!item) {
    HoldingSpaceItem::Type type;
    switch (in_progress_download->GetType()) {
      case InProgressDownload::Type::kAsh:
        type = HoldingSpaceItem::Type::kDownload;
        break;
      case InProgressDownload::Type::kLacros:
        type = HoldingSpaceItem::Type::kLacrosDownload;
        break;
    }
    service()->AddDownload(
        type, in_progress_download->GetFilePath(),
        in_progress_download->GetProgress(),
        in_progress_download->GetPlaceholderImageSkiaResolver());
    in_progress_download->SetHoldingSpaceItem(
        item = model()->GetItem(type, in_progress_download->GetFilePath()));
    // NOTE: This code intentionally falls through so as to update metadata for
    // the newly created holding space item.
  }

  // May be `nullptr` in tests.
  if (!item)
    return;

  // Update.
  model()
      ->UpdateItem(item->id())
      ->SetBackingFile(in_progress_download->GetFilePath(),
                       holding_space_util::ResolveFileSystemUrl(
                           profile(), in_progress_download->GetFilePath()))
      .SetText(in_progress_download->GetText())
      .SetSecondaryText(in_progress_download->GetSecondaryText())
      .SetPaused(in_progress_download->IsPaused())
      .SetProgress(in_progress_download->GetProgress());

  if (!invalidate_image)
    return;

  model()->InvalidateItemImageIf(base::BindRepeating(
      [](const std::string& item_id, const HoldingSpaceItem* item) {
        return item->id() == item_id;
      },
      std::cref(item->id())));
}

}  // namespace ash
