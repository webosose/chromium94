// Copyright 2021 The Tint Authors.
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

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "fuzzers/tint_common_fuzzer.h"
#include "fuzzers/tint_regex_fuzzer/cli.h"
#include "fuzzers/tint_regex_fuzzer/util.h"
#include "fuzzers/tint_regex_fuzzer/wgsl_mutator.h"

#include "src/reader/wgsl/parser.h"
#include "src/writer/wgsl/generator.h"

namespace tint {
namespace fuzzers {
namespace regex_fuzzer {
namespace {

CliParams cli_params{};

enum class MutationKind {
  kSwapIntervals,
  kDeleteInterval,
  kDuplicateInterval,
  kReplaceIdentifier,
  kReplaceLiteral,
  kNumMutationKinds
};

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
  // Parse CLI parameters. `ParseCliParams` will call `exit` if some parameter
  // is invalid.
  cli_params = ParseCliParams(argc, *argv);
  return 0;
}

extern "C" size_t LLVMFuzzerCustomMutator(uint8_t* data,
                                          size_t size,
                                          size_t max_size,
                                          unsigned seed) {
  std::string wgsl_code(data, data + size);
  const std::vector<std::string> delimiters{";"};
  std::mt19937 generator(seed);

  std::string delimiter =
      delimiters[GetRandomIntFromRange(0, delimiters.size() - 1, generator)];

  MutationKind mutation_kind = static_cast<MutationKind>(GetRandomIntFromRange(
      0, static_cast<size_t>(MutationKind::kNumMutationKinds) - 1, generator));

  switch (mutation_kind) {
    case MutationKind::kSwapIntervals:
      if (!SwapRandomIntervals(delimiter, wgsl_code, generator)) {
        return 0;
      }
      break;

    case MutationKind::kDeleteInterval:
      if (!DeleteRandomInterval(delimiter, wgsl_code, generator)) {
        return 0;
      }
      break;

    case MutationKind::kDuplicateInterval:
      if (!DuplicateRandomInterval(delimiter, wgsl_code, generator)) {
        return 0;
      }
      break;

    case MutationKind::kReplaceIdentifier:
      if (!ReplaceRandomIdentifier(wgsl_code, generator)) {
        return 0;
      }
      break;

    case MutationKind::kReplaceLiteral:
      if (!ReplaceRandomIntLiteral(wgsl_code, generator)) {
        return 0;
      }
      break;

    default:
      assert(false && "Unreachable");
      return 0;
  }

  if (wgsl_code.size() > max_size) {
    return 0;
  }

  memcpy(data, wgsl_code.c_str(), wgsl_code.size());
  return wgsl_code.size();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size == 0) {
    return 0;
  }

  struct Target {
    FuzzingTarget fuzzing_target;
    OutputFormat output_format;
    const char* name;
  };

  Target targets[] = {{FuzzingTarget::kWgsl, OutputFormat::kWGSL, "WGSL"},
                      {FuzzingTarget::kHlsl, OutputFormat::kHLSL, "HLSL"},
                      {FuzzingTarget::kMsl, OutputFormat::kMSL, "MSL"},
                      {FuzzingTarget::kSpv, OutputFormat::kSpv, "SPV"}};

  for (auto target : targets) {
    if ((target.fuzzing_target & cli_params.fuzzing_target) !=
        target.fuzzing_target) {
      continue;
    }

    transform::Manager transform_manager;
    transform::DataMap transform_inputs;
    transform_manager.Add<transform::Robustness>();

    CommonFuzzer fuzzer(InputFormat::kWGSL, target.output_format);
    fuzzer.EnableInspector();
    fuzzer.SetTransformManager(&transform_manager, std::move(transform_inputs));

    fuzzer.Run(data, size);
  }

  return 0;
}

}  // namespace
}  // namespace regex_fuzzer
}  // namespace fuzzers
}  // namespace tint
