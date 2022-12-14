// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_LEARNING_IMPL_LOOKUP_TABLE_TRAINER_H_
#define MEDIA_LEARNING_IMPL_LOOKUP_TABLE_TRAINER_H_

#include "base/component_export.h"
#include "base/macros.h"
#include "media/learning/common/learning_task.h"
#include "media/learning/impl/training_algorithm.h"

namespace media {
namespace learning {

// Trains a lookup table model.
class COMPONENT_EXPORT(LEARNING_IMPL) LookupTableTrainer
    : public TrainingAlgorithm {
 public:
  LookupTableTrainer();
  ~LookupTableTrainer() override;

  void Train(const LearningTask& task,
             const TrainingData& training_data,
             TrainedModelCB model_cb) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LookupTableTrainer);
};

}  // namespace learning
}  // namespace media

#endif  // MEDIA_LEARNING_IMPL_LOOKUP_TABLE_TRAINER_H_
