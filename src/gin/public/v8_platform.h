// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GIN_PUBLIC_V8_PLATFORM_H_
#define GIN_PUBLIC_V8_PLATFORM_H_

#include "base/allocator/buildflags.h"
#include "base/compiler_specific.h"
#include "base/lazy_instance.h"
#include "gin/gin_export.h"
#include "v8/include/v8-platform.h"

namespace gin {

// A v8::Platform implementation to use with gin.
class GIN_EXPORT V8Platform : public v8::Platform {
 public:
  V8Platform(const V8Platform&) = delete;
  V8Platform& operator=(const V8Platform&) = delete;

  static V8Platform* Get();

// v8::Platform implementation.
// Some configurations do not use page_allocator.
#if BUILDFLAG(USE_PARTITION_ALLOC)
  v8::PageAllocator* GetPageAllocator() override;
  void OnCriticalMemoryPressure() override;
#endif
  std::shared_ptr<v8::TaskRunner> GetForegroundTaskRunner(
      v8::Isolate*) override;
  int NumberOfWorkerThreads() override;
  void CallOnWorkerThread(std::unique_ptr<v8::Task> task) override;
  void CallBlockingTaskOnWorkerThread(std::unique_ptr<v8::Task> task) override;
  void CallLowPriorityTaskOnWorkerThread(
      std::unique_ptr<v8::Task> task) override;
  void CallDelayedOnWorkerThread(std::unique_ptr<v8::Task> task,
                                 double delay_in_seconds) override;
  std::unique_ptr<v8::JobHandle> PostJob(
      v8::TaskPriority priority,
      std::unique_ptr<v8::JobTask> job_task) override;
  bool IdleTasksEnabled(v8::Isolate* isolate) override;
  double MonotonicallyIncreasingTime() override;
  double CurrentClockTimeMillis() override;
  StackTracePrinter GetStackTracePrinter() override;
  v8::TracingController* GetTracingController() override;

 private:
  friend struct base::LazyInstanceTraitsBase<V8Platform>;

  V8Platform();
  ~V8Platform() override;

  class TracingControllerImpl;
  std::unique_ptr<TracingControllerImpl> tracing_controller_;
};

}  // namespace gin

#endif  // GIN_PUBLIC_V8_PLATFORM_H_
