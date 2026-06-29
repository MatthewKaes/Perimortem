// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Perimortem::Core::Thread {

class Worker {
 public:
  Worker() = default;
  ~Worker();
  Worker(Worker&&);
  auto operator=(Worker&&) -> Worker&;

  // Delete copy constructors
  Worker(const Worker&) = delete;
  auto operator=(const Worker&) -> Worker& = delete;

  auto join() -> void;

  template <typename Job>
  static auto start(Core::View::Bytes name, Job& job) -> Worker {
    static_assert(sizeof(Bits_64) >= sizeof(Job*));
    return start(name, run_job<Job>, reinterpret_cast<Bits_64>(&job));
  }
  static auto on_main_thread() -> Bool;
  static auto thread_id() -> Count;
  static auto thread_name() -> View::Bytes;
  static constexpr auto max_workers() -> Count { return 32; };

 private:
  using JobFunc = void (*)(Bits_64);

  template <typename Job>
  static auto run_job(Bits_64 context) -> void {
    (*reinterpret_cast<Job*>(context))();
  }

  static auto start(Core::View::Bytes name, JobFunc func, Bits_64 context)
      -> Worker;

  // pthread_t is an unsigned long on Linux x86_64 (8 bytes, 8-byte aligned).
  // Validated by static_assert in thread.cpp.
  Bits_64 handle = 0;
};

}  // namespace Perimortem::Core::Thread
