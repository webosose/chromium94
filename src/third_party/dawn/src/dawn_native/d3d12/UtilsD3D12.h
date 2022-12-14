// Copyright 2019 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef DAWNNATIVE_D3D12_UTILSD3D12_H_
#define DAWNNATIVE_D3D12_UTILSD3D12_H_

#include "dawn_native/Commands.h"
#include "dawn_native/d3d12/BufferD3D12.h"
#include "dawn_native/d3d12/TextureCopySplitter.h"
#include "dawn_native/d3d12/TextureD3D12.h"
#include "dawn_native/d3d12/d3d12_platform.h"
#include "dawn_native/dawn_platform.h"

namespace dawn_native { namespace d3d12 {

    ResultOrError<std::wstring> ConvertStringToWstring(const char* str);

    D3D12_COMPARISON_FUNC ToD3D12ComparisonFunc(wgpu::CompareFunction func);

    D3D12_TEXTURE_COPY_LOCATION ComputeTextureCopyLocationForTexture(const Texture* texture,
                                                                     uint32_t level,
                                                                     uint32_t layer,
                                                                     Aspect aspect);

    D3D12_TEXTURE_COPY_LOCATION ComputeBufferLocationForCopyTextureRegion(
        const Texture* texture,
        ID3D12Resource* bufferResource,
        const Extent3D& bufferSize,
        const uint64_t offset,
        const uint32_t rowPitch,
        Aspect aspect);
    D3D12_BOX ComputeD3D12BoxFromOffsetAndSize(const Origin3D& offset, const Extent3D& copySize);

    bool IsTypeless(DXGI_FORMAT format);

    void RecordCopyBufferToTextureFromTextureCopySplit(ID3D12GraphicsCommandList* commandList,
                                                       const TextureCopySubresource& baseCopySplit,
                                                       ID3D12Resource* bufferResource,
                                                       uint64_t baseOffset,
                                                       uint64_t bufferBytesPerRow,
                                                       Texture* texture,
                                                       uint32_t textureMiplevel,
                                                       uint32_t textureLayer,
                                                       Aspect aspect);

    void RecordCopyBufferToTexture(CommandRecordingContext* commandContext,
                                   const TextureCopy& textureCopy,
                                   ID3D12Resource* bufferResource,
                                   const uint64_t offset,
                                   const uint32_t bytesPerRow,
                                   const uint32_t rowsPerImage,
                                   const Extent3D& copySize,
                                   Texture* texture,
                                   Aspect aspect);

    void RecordCopyTextureToBufferFromTextureCopySplit(ID3D12GraphicsCommandList* commandList,
                                                       const TextureCopySubresource& baseCopySplit,
                                                       Buffer* buffer,
                                                       uint64_t baseOffset,
                                                       uint64_t bufferBytesPerRow,
                                                       Texture* texture,
                                                       uint32_t textureMiplevel,
                                                       uint32_t textureLayer,
                                                       Aspect aspect);

    void RecordCopyTextureToBuffer(ID3D12GraphicsCommandList* commandList,
                                   const TextureCopy& textureCopy,
                                   const BufferCopy& bufferCopy,
                                   Texture* texture,
                                   Buffer* buffer,
                                   const Extent3D& copySize);

}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_UTILSD3D12_H_
