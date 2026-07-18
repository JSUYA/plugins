// Copyright 2024 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "task_runner_tizen.h"

TaskRunnerTizen::TaskRunnerTizen() : state_(std::make_shared<State>()) {}

TaskRunnerTizen::~TaskRunnerTizen() {
  std::shared_ptr<State> state = std::move(state_);
  guint source_id = 0;
  {
    std::lock_guard<std::mutex> lock(state->tasks_mutex);
    state->shutting_down = true;
    source_id = state->source_id;
    state->source_id = 0;
    state->tasks = {};
  }
  if (source_id != 0) {
    g_source_remove(source_id);
  }
}

void TaskRunnerTizen::EnqueueTask(TaskClosure task) {
  std::shared_ptr<State> state = state_;
  std::lock_guard<std::mutex> lock(state->tasks_mutex);
  if (state->shutting_down) {
    return;
  }
  state->tasks.push(std::move(task));
  if (state->source_id == 0) {
    auto* source_data = new std::shared_ptr<State>(state);
    state->source_id = g_idle_add_full(G_PRIORITY_DEFAULT, RunTask, source_data,
                                       DestroySourceData);
    if (state->source_id == 0) {
      delete source_data;
    }
  }
}

gboolean TaskRunnerTizen::RunTask(gpointer data) {
  std::shared_ptr<State> state = *static_cast<std::shared_ptr<State>*>(data);
  while (true) {
    TaskClosure task;
    {
      std::lock_guard<std::mutex> lock(state->tasks_mutex);
      if (state->shutting_down || state->tasks.empty()) {
        state->source_id = 0;
        return G_SOURCE_REMOVE;
      }
      task = std::move(state->tasks.front());
      state->tasks.pop();
    }
    task();
  }
}

void TaskRunnerTizen::DestroySourceData(gpointer data) {
  delete static_cast<std::shared_ptr<State>*>(data);
}
