// Copyright 2024 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "task_runner_tizen.h"

TaskRunnerTizen::TaskRunnerTizen() = default;

TaskRunnerTizen::~TaskRunnerTizen() = default;

void TaskRunnerTizen::EnqueueTask(TaskClosure task) {
  std::lock_guard<std::mutex> lock(tasks_mutex_);
  tasks_.push(std::move(task));
  g_main_context_invoke(nullptr, RunTask, this);
}

gboolean TaskRunnerTizen::RunTask(gpointer data) {
  TaskRunnerTizen* runner = static_cast<TaskRunnerTizen*>(data);
  std::lock_guard<std::mutex> lock(runner->tasks_mutex_);
  while (!runner->tasks_.empty()) {
    TaskClosure task = std::move(runner->tasks_.front());
    runner->tasks_.pop();
    task();
  }
  return G_SOURCE_REMOVE;
}
