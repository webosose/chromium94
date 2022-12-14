// Copyright 2020 The Dawn Authors
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

#include "dawn_native/QuerySet.h"

#include "dawn_native/Device.h"
#include "dawn_native/Extensions.h"
#include "dawn_native/ValidationUtils_autogen.h"

#include <set>

namespace dawn_native {

    namespace {

        class ErrorQuerySet final : public QuerySetBase {
          public:
            ErrorQuerySet(DeviceBase* device) : QuerySetBase(device, ObjectBase::kError) {
            }

          private:
            void DestroyImpl() override {
                UNREACHABLE();
            }
        };

    }  // anonymous namespace

    MaybeError ValidateQuerySetDescriptor(DeviceBase* device,
                                          const QuerySetDescriptor* descriptor) {
        if (descriptor->nextInChain != nullptr) {
            return DAWN_VALIDATION_ERROR("nextInChain must be nullptr");
        }

        if (descriptor->count > kMaxQueryCount) {
            return DAWN_VALIDATION_ERROR("Max query count exceeded");
        }

        DAWN_TRY(ValidateQueryType(descriptor->type));

        switch (descriptor->type) {
            case wgpu::QueryType::Occlusion:
                if (descriptor->pipelineStatisticsCount != 0) {
                    return DAWN_VALIDATION_ERROR(
                        "The pipeline statistics should not be set if query type is Occlusion");
                }
                break;

            case wgpu::QueryType::PipelineStatistics: {
                // TODO(crbug.com/1177506): Pipeline statistics query is not fully implemented.
                // Disallow it as unsafe until the implementaion is completed.
                if (device->IsToggleEnabled(Toggle::DisallowUnsafeAPIs)) {
                    return DAWN_VALIDATION_ERROR(
                        "Pipeline statistics query is disallowed because it's not fully "
                        "implemented");
                }

                if (!device->IsExtensionEnabled(Extension::PipelineStatisticsQuery)) {
                    return DAWN_VALIDATION_ERROR(
                        "The pipeline statistics query feature is not supported");
                }

                if (descriptor->pipelineStatisticsCount == 0) {
                    return DAWN_VALIDATION_ERROR(
                        "At least one pipeline statistics is set if query type is "
                        "PipelineStatistics");
                }

                std::set<wgpu::PipelineStatisticName> pipelineStatisticsSet;
                for (uint32_t i = 0; i < descriptor->pipelineStatisticsCount; i++) {
                    DAWN_TRY(ValidatePipelineStatisticName(descriptor->pipelineStatistics[i]));

                    std::pair<std::set<wgpu::PipelineStatisticName>::iterator, bool> res =
                        pipelineStatisticsSet.insert((descriptor->pipelineStatistics[i]));
                    if (!res.second) {
                        return DAWN_VALIDATION_ERROR("Duplicate pipeline statistics found");
                    }
                }
            } break;

            case wgpu::QueryType::Timestamp:
                if (!device->IsExtensionEnabled(Extension::TimestampQuery)) {
                    return DAWN_VALIDATION_ERROR("The timestamp query feature is not supported");
                }

                if (device->IsToggleEnabled(Toggle::DisallowUnsafeAPIs)) {
                    return DAWN_VALIDATION_ERROR(
                        "The timestamp query is disallowed because it may expose precise timing "
                        "information");
                }

                if (descriptor->pipelineStatisticsCount != 0) {
                    return DAWN_VALIDATION_ERROR(
                        "The pipeline statistics should not be set if query type is Timestamp");
                }
                break;

            default:
                break;
        }

        return {};
    }

    QuerySetBase::QuerySetBase(DeviceBase* device, const QuerySetDescriptor* descriptor)
        : ObjectBase(device),
          mQueryType(descriptor->type),
          mQueryCount(descriptor->count),
          mState(QuerySetState::Available) {
        for (uint32_t i = 0; i < descriptor->pipelineStatisticsCount; i++) {
            mPipelineStatistics.push_back(descriptor->pipelineStatistics[i]);
        }

        mQueryAvailability.resize(descriptor->count);
    }

    QuerySetBase::QuerySetBase(DeviceBase* device, ObjectBase::ErrorTag tag)
        : ObjectBase(device, tag) {
    }

    QuerySetBase::~QuerySetBase() {
        // Uninitialized or already destroyed
        ASSERT(mState == QuerySetState::Unavailable || mState == QuerySetState::Destroyed);
    }

    // static
    QuerySetBase* QuerySetBase::MakeError(DeviceBase* device) {
        return new ErrorQuerySet(device);
    }

    wgpu::QueryType QuerySetBase::GetQueryType() const {
        return mQueryType;
    }

    uint32_t QuerySetBase::GetQueryCount() const {
        return mQueryCount;
    }

    const std::vector<wgpu::PipelineStatisticName>& QuerySetBase::GetPipelineStatistics() const {
        return mPipelineStatistics;
    }

    const std::vector<bool>& QuerySetBase::GetQueryAvailability() const {
        return mQueryAvailability;
    }

    void QuerySetBase::SetQueryAvailability(uint32_t index, bool available) {
        mQueryAvailability[index] = available;
    }

    MaybeError QuerySetBase::ValidateCanUseInSubmitNow() const {
        ASSERT(!IsError());
        if (mState == QuerySetState::Destroyed) {
            return DAWN_VALIDATION_ERROR("Destroyed query set used in a submit");
        }
        return {};
    }

    void QuerySetBase::APIDestroy() {
        if (GetDevice()->ConsumedError(ValidateDestroy())) {
            return;
        }
        DestroyInternal();
    }

    MaybeError QuerySetBase::ValidateDestroy() const {
        DAWN_TRY(GetDevice()->ValidateObject(this));
        return {};
    }

    void QuerySetBase::DestroyInternal() {
        if (mState != QuerySetState::Destroyed) {
            DestroyImpl();
        }
        mState = QuerySetState::Destroyed;
    }

}  // namespace dawn_native
