// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/shared_image_video.h"

#include <utility>

#include "base/android/scoped_hardware_buffer_fence_sync.h"
#include "base/android/scoped_hardware_buffer_handle.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/viz/common/gpu/vulkan_context_provider.h"
#include "components/viz/common/resources/resource_format_utils.h"
#include "components/viz/common/resources/resource_sizes.h"
#include "gpu/command_buffer/common/shared_image_usage.h"
#include "gpu/command_buffer/service/abstract_texture.h"
#include "gpu/command_buffer/service/abstract_texture_impl.h"
#include "gpu/command_buffer/service/ahardwarebuffer_utils.h"
#include "gpu/command_buffer/service/mailbox_manager.h"
#include "gpu/command_buffer/service/memory_tracking.h"
#include "gpu/command_buffer/service/shared_context_state.h"
#include "gpu/command_buffer/service/shared_image_representation.h"
#include "gpu/command_buffer/service/shared_image_representation_skia_gl.h"
#include "gpu/command_buffer/service/shared_image_representation_skia_vk_android.h"
#include "gpu/command_buffer/service/skia_utils.h"
#include "gpu/command_buffer/service/texture_manager.h"
#include "gpu/command_buffer/service/texture_owner.h"
#include "gpu/vulkan/vulkan_device_queue.h"
#include "gpu/vulkan/vulkan_fence_helper.h"
#include "gpu/vulkan/vulkan_function_pointers.h"
#include "gpu/vulkan/vulkan_image.h"
#include "gpu/vulkan/vulkan_implementation.h"
#include "gpu/vulkan/vulkan_util.h"
#include "third_party/skia/include/core/SkPromiseImageTexture.h"
#include "third_party/skia/include/gpu/GrBackendSemaphore.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "ui/gl/gl_utils.h"

namespace gpu {

namespace {
class VideoImage : public gl::GLImage {
 public:
  VideoImage() = default;

  VideoImage(AHardwareBuffer* buffer, base::ScopedFD begin_read_fence)
      : handle_(base::android::ScopedHardwareBufferHandle::Create(buffer)),
        begin_read_fence_(std::move(begin_read_fence)) {}

  // gl::GLImage:
  std::unique_ptr<base::android::ScopedHardwareBufferFenceSync>
  GetAHardwareBuffer() override {
    if (!handle_.is_valid())
      return nullptr;

    return std::make_unique<ScopedHardwareBufferFenceSyncImpl>(
        this, base::android::ScopedHardwareBufferHandle::Create(handle_.get()),
        std::move(begin_read_fence_));
  }

  base::ScopedFD TakeEndReadFence() { return std::move(end_read_fence_); }

 protected:
  ~VideoImage() override = default;

 private:
  class ScopedHardwareBufferFenceSyncImpl
      : public base::android::ScopedHardwareBufferFenceSync {
   public:
    ScopedHardwareBufferFenceSyncImpl(
        scoped_refptr<VideoImage> image,
        base::android::ScopedHardwareBufferHandle handle,
        base::ScopedFD fence_fd)
        : ScopedHardwareBufferFenceSync(std::move(handle),
                                        std::move(fence_fd),
                                        base::ScopedFD(),
                                        /*is_video=*/true),
          image_(std::move(image)) {}
    ~ScopedHardwareBufferFenceSyncImpl() override = default;

    void SetReadFence(base::ScopedFD fence_fd, bool has_context) override {
      image_->end_read_fence_ =
          gl::MergeFDs(std::move(image_->end_read_fence_), std::move(fence_fd));
    }

   private:
    scoped_refptr<VideoImage> image_;
  };

  base::android::ScopedHardwareBufferHandle handle_;

  // This fence should be waited upon before reading from the buffer.
  base::ScopedFD begin_read_fence_;

  // This fence should be waited upon to ensure that the reader is finished
  // reading from the buffer.
  base::ScopedFD end_read_fence_;
};

}  // namespace

SharedImageVideo::SharedImageVideo(
    const Mailbox& mailbox,
    const gfx::Size& size,
    const gfx::ColorSpace color_space,
    GrSurfaceOrigin surface_origin,
    SkAlphaType alpha_type,
    scoped_refptr<StreamTextureSharedImageInterface> stream_texture_sii,
    scoped_refptr<SharedContextState> context_state,
    bool is_thread_safe,
    scoped_refptr<RefCountedLock> drdc_lock)
    : SharedImageBackingAndroid(
          mailbox,
          viz::RGBA_8888,
          size,
          color_space,
          surface_origin,
          alpha_type,
          (SHARED_IMAGE_USAGE_DISPLAY | SHARED_IMAGE_USAGE_GLES2),
          viz::ResourceSizes::UncheckedSizeInBytes<size_t>(size,
                                                           viz::RGBA_8888),
          is_thread_safe,
          base::ScopedFD()),
      RefCountedLockHelperDrDc(std::move(drdc_lock)),
      stream_texture_sii_(std::move(stream_texture_sii)),
      context_state_(std::move(context_state)),
      task_runner_(base::ThreadTaskRunnerHandle::Get()) {
  DCHECK(stream_texture_sii_);
  DCHECK(context_state_);

  context_state_->AddContextLostObserver(this);
}

SharedImageVideo::~SharedImageVideo() {
  if (task_runner_->RunsTasksInCurrentSequence()) {
    CleanupOnCorrectThread(std::move(stream_texture_sii_),
                           std::move(context_state_), this, /*event=*/nullptr,
                           GetDrDcLock());
  } else {
    base::WaitableEvent event;
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&SharedImageVideo::CleanupOnCorrectThread,
                       std::move(stream_texture_sii_),
                       std::move(context_state_), base::Unretained(this),
                       &event, GetDrDcLock()));
    event.Wait();
  }
}

void SharedImageVideo::CleanupOnCorrectThread(
    scoped_refptr<StreamTextureSharedImageInterface> stream_texture_sii,
    scoped_refptr<SharedContextState> context_state,
    SharedImageVideo* backing,
    base::WaitableEvent* event,
    scoped_refptr<RefCountedLock> drdc_lock) {
  if (context_state)
    context_state->RemoveContextLostObserver(backing);
  context_state.reset();

  {
    base::AutoLockMaybe auto_lock(drdc_lock ? drdc_lock->GetDrDcLockPtr()
                                            : nullptr);
    stream_texture_sii->ReleaseResources();
    stream_texture_sii.reset();
  }
  if (event)
    event->Signal();
}

gfx::Rect SharedImageVideo::ClearedRect() const {
  // SharedImageVideo objects are always created from pre-initialized textures
  // provided by the media decoder. Always treat these as cleared (return the
  // full rectangle).
  return gfx::Rect(size());
}

void SharedImageVideo::SetClearedRect(const gfx::Rect& cleared_rect) {}

void SharedImageVideo::Update(std::unique_ptr<gfx::GpuFence> in_fence) {
  DCHECK(!in_fence);
}

bool SharedImageVideo::ProduceLegacyMailbox(MailboxManager* mailbox_manager) {
  // Android does not use legacy mailbox anymore. Hence marking this as
  // NOTREACHED() now. Once all platform stops using legacy mailbox, this
  // method can be removed.
  NOTREACHED();
  return false;
}

size_t SharedImageVideo::EstimatedSizeForMemTracking() const {
  base::AutoLockMaybe auto_lock(GetDrDcLockPtr());

  // This backing contributes to gpu memory only if its bound to the texture
  // and not when the backing is created.
  return stream_texture_sii_->IsUsingGpuMemory() ? estimated_size() : 0;
}

void SharedImageVideo::OnContextLost() {
  base::AutoLockMaybe auto_lock(GetDrDcLockPtr());

  // We release codec buffers when shared image context is lost. This is
  // because texture owner's texture was created on shared context. Once
  // shared context is lost, no one should try to use that texture.
  stream_texture_sii_->ReleaseResources();
  context_state_->RemoveContextLostObserver(this);
  context_state_ = nullptr;
}

absl::optional<VulkanYCbCrInfo> SharedImageVideo::GetYcbcrInfo(
    TextureOwner* texture_owner,
    scoped_refptr<SharedContextState> context_state) {
  // For non-vulkan context, return null.
  if (!context_state->GrContextIsVulkan())
    return absl::nullopt;

  // Get AHardwareBuffer from the latest frame.
  auto scoped_hardware_buffer = texture_owner->GetAHardwareBuffer();
  if (!scoped_hardware_buffer) {
    return absl::nullopt;
  }

  DCHECK(scoped_hardware_buffer->buffer());
  auto* context_provider = context_state->vk_context_provider();
  VulkanImplementation* vk_implementation =
      context_provider->GetVulkanImplementation();
  VkDevice vk_device = context_provider->GetDeviceQueue()->GetVulkanDevice();

  VulkanYCbCrInfo ycbcr_info;
  if (!vk_implementation->GetSamplerYcbcrConversionInfo(
          vk_device, scoped_hardware_buffer->TakeBuffer(), &ycbcr_info)) {
    LOG(ERROR) << "Failed to get the ycbcr info.";
    return absl::nullopt;
  }
  return absl::optional<VulkanYCbCrInfo>(ycbcr_info);
}

std::unique_ptr<base::android::ScopedHardwareBufferFenceSync>
SharedImageVideo::GetAHardwareBuffer() {
  base::AutoLockMaybe auto_lock(GetDrDcLockPtr());

  DCHECK(stream_texture_sii_);
  return stream_texture_sii_->GetAHardwareBuffer();
}

// Representation of SharedImageVideo as a GL Texture.
class SharedImageRepresentationGLTextureVideo
    : public SharedImageRepresentationGLTexture,
      public RefCountedLockHelperDrDc {
 public:
  SharedImageRepresentationGLTextureVideo(
      SharedImageManager* manager,
      SharedImageVideo* backing,
      MemoryTypeTracker* tracker,
      std::unique_ptr<gles2::AbstractTexture> texture,
      scoped_refptr<RefCountedLock> drdc_lock)
      : SharedImageRepresentationGLTexture(manager, backing, tracker),
        RefCountedLockHelperDrDc(std::move(drdc_lock)),
        texture_(std::move(texture)) {}

  gles2::Texture* GetTexture() override {
    auto* texture = gles2::Texture::CheckedCast(texture_->GetTextureBase());
    DCHECK(texture);

    return texture;
  }

  bool BeginAccess(GLenum mode) override {
    base::AutoLockMaybe auto_lock(GetDrDcLockPtr());

    // This representation should only be called for read or overlay.
    DCHECK(mode == GL_SHARED_IMAGE_ACCESS_MODE_READ_CHROMIUM ||
           mode == GL_SHARED_IMAGE_ACCESS_MODE_OVERLAY_CHROMIUM);

    auto* video_backing = static_cast<SharedImageVideo*>(backing());
    video_backing->BeginGLReadAccess(texture_->service_id());
    return true;
  }

  void EndAccess() override {}

 private:
  std::unique_ptr<gles2::AbstractTexture> texture_;

  DISALLOW_COPY_AND_ASSIGN(SharedImageRepresentationGLTextureVideo);
};

// Representation of SharedImageVideo as a GL Texture.
class SharedImageRepresentationGLTexturePassthroughVideo
    : public SharedImageRepresentationGLTexturePassthrough,
      public RefCountedLockHelperDrDc {
 public:
  SharedImageRepresentationGLTexturePassthroughVideo(
      SharedImageManager* manager,
      SharedImageVideo* backing,
      MemoryTypeTracker* tracker,
      std::unique_ptr<gles2::AbstractTexture> abstract_texture,
      scoped_refptr<RefCountedLock> drdc_lock)
      : SharedImageRepresentationGLTexturePassthrough(manager,
                                                      backing,
                                                      tracker),
        RefCountedLockHelperDrDc(std::move(drdc_lock)),
        abstract_texture_(std::move(abstract_texture)),
        passthrough_texture_(gles2::TexturePassthrough::CheckedCast(
            abstract_texture_->GetTextureBase())) {
    // TODO(https://crbug.com/1172769): Remove this CHECK.
    CHECK(passthrough_texture_);
  }

  const scoped_refptr<gles2::TexturePassthrough>& GetTexturePassthrough()
      override {
    return passthrough_texture_;
  }

  bool BeginAccess(GLenum mode) override {
    base::AutoLockMaybe auto_lock(GetDrDcLockPtr());

    // This representation should only be called for read or overlay.
    DCHECK(mode == GL_SHARED_IMAGE_ACCESS_MODE_READ_CHROMIUM ||
           mode == GL_SHARED_IMAGE_ACCESS_MODE_OVERLAY_CHROMIUM);

    auto* video_backing = static_cast<SharedImageVideo*>(backing());
    video_backing->BeginGLReadAccess(passthrough_texture_->service_id());
    return true;
  }

  void EndAccess() override {}

 private:
  std::unique_ptr<gles2::AbstractTexture> abstract_texture_;
  scoped_refptr<gles2::TexturePassthrough> passthrough_texture_;

  DISALLOW_COPY_AND_ASSIGN(SharedImageRepresentationGLTexturePassthroughVideo);
};

class SharedImageRepresentationVideoSkiaVk
    : public SharedImageRepresentationSkiaVkAndroid,
      public RefCountedLockHelperDrDc {
 public:
  SharedImageRepresentationVideoSkiaVk(
      SharedImageManager* manager,
      SharedImageBackingAndroid* backing,
      scoped_refptr<SharedContextState> context_state,
      MemoryTypeTracker* tracker,
      scoped_refptr<RefCountedLock> drdc_lock)
      : SharedImageRepresentationSkiaVkAndroid(manager,
                                               backing,
                                               std::move(context_state),
                                               tracker),
        RefCountedLockHelperDrDc(std::move(drdc_lock)) {}

  sk_sp<SkSurface> BeginWriteAccess(
      int final_msaa_count,
      const SkSurfaceProps& surface_props,
      std::vector<GrBackendSemaphore>* begin_semaphores,
      std::vector<GrBackendSemaphore>* end_semaphores,
      std::unique_ptr<GrBackendSurfaceMutableState>* end_state) override {
    // Writes are not intended to used for video backed representations.
    NOTIMPLEMENTED();
    return nullptr;
  }

  void EndWriteAccess(sk_sp<SkSurface> surface) override { NOTIMPLEMENTED(); }

  sk_sp<SkPromiseImageTexture> BeginReadAccess(
      std::vector<GrBackendSemaphore>* begin_semaphores,
      std::vector<GrBackendSemaphore>* end_semaphores,
      std::unique_ptr<GrBackendSurfaceMutableState>* end_state) override {
    base::AutoLockMaybe auto_lock(GetDrDcLockPtr());

    DCHECK(!scoped_hardware_buffer_);
    auto* video_backing = static_cast<SharedImageVideo*>(backing());
    DCHECK(video_backing);
    auto* stream_texture_sii = video_backing->stream_texture_sii_.get();

    // GetAHardwareBuffer() renders the latest image and gets AHardwareBuffer
    // from it.
    scoped_hardware_buffer_ = stream_texture_sii->GetAHardwareBuffer();
    if (!scoped_hardware_buffer_) {
      LOG(ERROR) << "Failed to get the hardware buffer.";
      return nullptr;
    }
    DCHECK(scoped_hardware_buffer_->buffer());

    // Wait on the sync fd attached to the buffer to make sure buffer is
    // ready before the read. This is done by inserting the sync fd semaphore
    // into begin_semaphore vector which client will wait on.
    init_read_fence_ = scoped_hardware_buffer_->TakeFence();

    if (!vulkan_image_) {
      DCHECK(!promise_texture_);

      vulkan_image_ = CreateVkImageFromAhbHandle(
          scoped_hardware_buffer_->TakeBuffer(), context_state(), size(),
          format(), VK_QUEUE_FAMILY_FOREIGN_EXT);
      if (!vulkan_image_)
        return nullptr;

      // We always use VK_IMAGE_TILING_OPTIMAL while creating the vk image in
      // VulkanImplementationAndroid::CreateVkImageAndImportAHB. Hence pass
      // the tiling parameter as VK_IMAGE_TILING_OPTIMAL to below call rather
      // than passing |vk_image_info.tiling|. This is also to ensure that the
      // promise image created here at [1] as well the fulfill image created
      // via the current function call are consistent and both are using
      // VK_IMAGE_TILING_OPTIMAL. [1] -
      // https://cs.chromium.org/chromium/src/components/viz/service/display_embedder/skia_output_surface_impl.cc?rcl=db5ffd448ba5d66d9d3c5c099754e5067c752465&l=789.
      DCHECK_EQ(static_cast<int32_t>(vulkan_image_->image_tiling()),
                static_cast<int32_t>(VK_IMAGE_TILING_OPTIMAL));

      // TODO(bsalomon): Determine whether it makes sense to attempt to reuse
      // this if the vk_info stays the same on subsequent calls.
      promise_texture_ = SkPromiseImageTexture::Make(
          GrBackendTexture(size().width(), size().height(),
                           CreateGrVkImageInfo(vulkan_image_.get())));
      DCHECK(promise_texture_);
    }

    return SharedImageRepresentationSkiaVkAndroid::BeginReadAccess(
        begin_semaphores, end_semaphores, end_state);
  }

  void EndReadAccess() override {
    base::AutoLockMaybe auto_lock(GetDrDcLockPtr());
    DCHECK(scoped_hardware_buffer_);

    SharedImageRepresentationSkiaVkAndroid::EndReadAccess();

    // Pass the end read access sync fd to the scoped hardware buffer. This
    // will make sure that the AImage associated with the hardware buffer will
    // be deleted only when the read access is ending.
    scoped_hardware_buffer_->SetReadFence(android_backing()->TakeReadFence(),
                                          true);
    scoped_hardware_buffer_ = nullptr;
  }

 private:
  std::unique_ptr<base::android::ScopedHardwareBufferFenceSync>
      scoped_hardware_buffer_;
};

// TODO(vikassoni): Currently GLRenderer doesn't support overlays with shared
// image. Add support for overlays in GLRenderer as well as overlay
// representations of shared image.
std::unique_ptr<SharedImageRepresentationGLTexture>
SharedImageVideo::ProduceGLTexture(SharedImageManager* manager,
                                   MemoryTypeTracker* tracker) {
  base::AutoLockMaybe auto_lock(GetDrDcLockPtr());

  // For (old) overlays, we don't have a texture owner, but overlay promotion
  // might not happen for some reasons. In that case, it will try to draw
  // which should result in no image.
  if (!stream_texture_sii_->HasTextureOwner())
    return nullptr;

  // Generate an abstract texture.
  auto texture = GenAbstractTexture(context_state_, /*passthrough=*/false);
  if (!texture)
    return nullptr;

  return std::make_unique<SharedImageRepresentationGLTextureVideo>(
      manager, this, tracker, std::move(texture), GetDrDcLock());
}

// TODO(vikassoni): Currently GLRenderer doesn't support overlays with shared
// image. Add support for overlays in GLRenderer as well as overlay
// representations of shared image.
std::unique_ptr<SharedImageRepresentationGLTexturePassthrough>
SharedImageVideo::ProduceGLTexturePassthrough(SharedImageManager* manager,
                                              MemoryTypeTracker* tracker) {
  base::AutoLockMaybe auto_lock(GetDrDcLockPtr());

  // For (old) overlays, we don't have a texture owner, but overlay promotion
  // might not happen for some reasons. In that case, it will try to draw
  // which should result in no image.
  if (!stream_texture_sii_->HasTextureOwner())
    return nullptr;

  // Generate an abstract texture.
  auto texture = GenAbstractTexture(context_state_, /*passthrough=*/true);
  if (!texture)
    return nullptr;

  return std::make_unique<SharedImageRepresentationGLTexturePassthroughVideo>(
      manager, this, tracker, std::move(texture), GetDrDcLock());
}

// Currently SkiaRenderer doesn't support overlays.
std::unique_ptr<SharedImageRepresentationSkia> SharedImageVideo::ProduceSkia(
    SharedImageManager* manager,
    MemoryTypeTracker* tracker,
    scoped_refptr<SharedContextState> context_state) {
  base::AutoLockMaybe auto_lock(GetDrDcLockPtr());

  DCHECK(context_state);

  // For (old) overlays, we don't have a texture owner, but overlay promotion
  // might not happen for some reasons. In that case, it will try to draw
  // which should result in no image.
  if (!stream_texture_sii_->HasTextureOwner())
    return nullptr;

  if (context_state->GrContextIsVulkan()) {
    return std::make_unique<SharedImageRepresentationVideoSkiaVk>(
        manager, this, std::move(context_state), tracker, GetDrDcLock());
  }

  DCHECK(context_state->GrContextIsGL());
  auto* texture_base = stream_texture_sii_->GetTextureBase();
  DCHECK(texture_base);
  const bool passthrough =
      (texture_base->GetType() == gpu::TextureBase::Type::kPassthrough);

  auto texture = GenAbstractTexture(context_state, passthrough);
  if (!texture)
    return nullptr;

  std::unique_ptr<gpu::SharedImageRepresentationGLTextureBase>
      gl_representation;
  if (passthrough) {
    gl_representation =
        std::make_unique<SharedImageRepresentationGLTexturePassthroughVideo>(
            manager, this, tracker, std::move(texture), GetDrDcLock());
  } else {
    gl_representation =
        std::make_unique<SharedImageRepresentationGLTextureVideo>(
            manager, this, tracker, std::move(texture), GetDrDcLock());
  }
  return SharedImageRepresentationSkiaGL::Create(std::move(gl_representation),
                                                 std::move(context_state),
                                                 manager, this, tracker);
}

std::unique_ptr<gles2::AbstractTexture> SharedImageVideo::GenAbstractTexture(
    scoped_refptr<SharedContextState> context_state,
    const bool passthrough) {
  AssertAcquiredDrDcLock();

  std::unique_ptr<gles2::AbstractTexture> texture;
  if (passthrough) {
    texture = std::make_unique<gles2::AbstractTextureImplPassthrough>(
        GL_TEXTURE_EXTERNAL_OES, GL_RGBA, size().width(), size().height(), 1, 0,
        GL_RGBA, GL_UNSIGNED_BYTE);
  } else {
    texture = std::make_unique<gles2::AbstractTextureImpl>(
        GL_TEXTURE_EXTERNAL_OES, GL_RGBA, size().width(), size().height(), 1, 0,
        GL_RGBA, GL_UNSIGNED_BYTE);
  }

  // If TextureOwner binds texture implicitly on update, that means it will
  // use TextureOwner texture_id to update and bind. Hence use TextureOwner
  // texture_id in abstract texture via BindStreamTextureImage().
  if (stream_texture_sii_->TextureOwnerBindsTextureOnUpdate()) {
    texture->BindStreamTextureImage(
        stream_texture_sii_.get(),
        stream_texture_sii_->GetTextureBase()->service_id());
  }
  return texture;
}

void SharedImageVideo::BeginGLReadAccess(const GLuint service_id) {
  AssertAcquiredDrDcLock();
  stream_texture_sii_->UpdateAndBindTexImage(service_id);
}

// Representation of SharedImageVideo as an overlay plane.
class SharedImageRepresentationOverlayVideo
    : public gpu::SharedImageRepresentationOverlay,
      public RefCountedLockHelperDrDc {
 public:
  SharedImageRepresentationOverlayVideo(gpu::SharedImageManager* manager,
                                        SharedImageVideo* backing,
                                        gpu::MemoryTypeTracker* tracker,
                                        scoped_refptr<RefCountedLock> drdc_lock)
      : gpu::SharedImageRepresentationOverlay(manager, backing, tracker),
        RefCountedLockHelperDrDc(std::move(drdc_lock)) {}

 protected:
  bool BeginReadAccess(std::vector<gfx::GpuFence>* acquire_fences) override {
    base::AutoLockMaybe auto_lock(GetDrDcLockPtr());
    // A |CodecImage| is already in a SurfaceView, render content to the
    // overlay.
    if (!stream_image()->HasTextureOwner()) {
      TRACE_EVENT0("media",
                   "SharedImageRepresentationOverlayVideo::BeginReadAccess");
      stream_image()->RenderToOverlay();
    }
    return true;
  }

  void EndReadAccess(gfx::GpuFenceHandle release_fence) override {
    base::AutoLockMaybe auto_lock(GetDrDcLockPtr());
    DCHECK(release_fence.is_null());
    if (gl_image_) {
      if (scoped_hardware_buffer_) {
        scoped_hardware_buffer_->SetReadFence(gl_image_->TakeEndReadFence(),
                                              true);
      }
      gl_image_.reset();
      scoped_hardware_buffer_.reset();
    }
  }

  gl::GLImage* GetGLImage() override {
    base::AutoLockMaybe auto_lock(GetDrDcLockPtr());
    DCHECK(stream_image()->HasTextureOwner())
        << "The backing is already in a SurfaceView!";

    // Note that we have SurfaceView overlay as well as SurfaceControl.
    // SurfaceView may/may not have TextureOwner whereas SurfaceControl always
    // have TextureOwner. It is not possible to know whether we are in
    // SurfaceView or SurfaceControl mode in Begin/EndReadAccess. Hence
    // |scoped_hardware_buffer_| and |gl_image_| needs to be created here since
    // GetGLImage will only be called for SurfaceControl.
    if (!gl_image_) {
      scoped_hardware_buffer_ = stream_image()->GetAHardwareBuffer();

      // |scoped_hardware_buffer_| could be null for cases when a buffer is
      // not acquired in ImageReader for some reasons and there is no previously
      // acquired image left.
      if (scoped_hardware_buffer_) {
        gl_image_ = base::MakeRefCounted<VideoImage>(
            scoped_hardware_buffer_->buffer(),
            scoped_hardware_buffer_->TakeFence());
      } else {
        // Caller of GetGLImage currently do not expect a null |gl_image_|.
        // Hence creating a valid object with null buffer which results in a
        // blank video frame and is expected. TODO(vikassoni) : Explore option
        // of returning a null GLImage here.
        gl_image_ = base::MakeRefCounted<VideoImage>();
      }
    }
    return gl_image_.get();
  }

  void NotifyOverlayPromotion(bool promotion,
                              const gfx::Rect& bounds) override {
    base::AutoLockMaybe auto_lock(GetDrDcLockPtr());
    stream_image()->NotifyOverlayPromotion(promotion, bounds);
  }

 private:
  std::unique_ptr<base::android::ScopedHardwareBufferFenceSync>
      scoped_hardware_buffer_;
  scoped_refptr<VideoImage> gl_image_;

  StreamTextureSharedImageInterface* stream_image() {
    auto* video_backing = static_cast<SharedImageVideo*>(backing());
    DCHECK(video_backing);
    return video_backing->stream_texture_sii_.get();
  }

  DISALLOW_COPY_AND_ASSIGN(SharedImageRepresentationOverlayVideo);
};

std::unique_ptr<gpu::SharedImageRepresentationOverlay>
SharedImageVideo::ProduceOverlay(gpu::SharedImageManager* manager,
                                 gpu::MemoryTypeTracker* tracker) {
  return std::make_unique<SharedImageRepresentationOverlayVideo>(
      manager, this, tracker, GetDrDcLock());
}

}  // namespace gpu
