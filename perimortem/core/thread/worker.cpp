// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/thread/worker.hpp"

#include <pthread.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

static_assert(
    sizeof(pthread_t) == sizeof(Bits_64) &&
        alignof(pthread_t) == sizeof(Bits_64),
    "pthread_t layout differs from Bits_64 — update Thread::handle");

using namespace Perimortem::Core;

// Stores the logical thread id.
// Any thread not spawned by Perimortem uses id -1 and is marked as "main".
static thread_local Bits_64 this_thread_id = Count(-1);

struct alignas(64) ThreadInfo {
  Thread::Worker::JobFunc func;
  Diagnostics::Log::Sink sink;
  Count name_size;
  Static::Bytes<36> name;
};

static_assert(sizeof(ThreadInfo) == 64);

static pthread_mutex_t worker_mutex = PTHREAD_MUTEX_INITIALIZER;
static Static::Vector<Bits_8, Thread::Worker::max_workers()> thread_occupancy;
static Static::Vector<ThreadInfo, Thread::Worker::max_workers()> thread_info;

static auto thread_entry(void* logical_thread_address) -> void* {
  // Set the active thread id to whatever slot was setup for this worker.
  // Bit janky but passing the "logical" address as a number avoids having to
  // memory manage any data across boundaries.
  this_thread_id = Count(logical_thread_address);
  const auto& this_thread_info = thread_info[this_thread_id];

  // Name includes null terminal
  const auto& thread_name = this_thread_info.name;
  pthread_setname_np(pthread_self(), Data::cast<char>(thread_name.get_data()));
  Diagnostics::Log::set_thread_name(
      thread_name.slice(0, this_thread_info.name_size));

  // By default inherit the thread's sink.
  Diagnostics::Log::set_sink(this_thread_info.sink);

  // Actually perform the work.
  this_thread_info.func();

  // On exit mark the worker as free.
  // We don't use any of the worker data after exit so if we get intercepted
  // during clean up that's okay after this stage.
  pthread_mutex_lock(&worker_mutex);
  thread_occupancy[this_thread_id] = 0;
  pthread_mutex_unlock(&worker_mutex);

  return nullptr;
}

Thread::Worker::~Worker() {
  // Avoid C++'s mistakes and make sure Workers actually `join` on exit.
  join();
}

Thread::Worker::Worker(Worker&& rhs) : handle(rhs.handle) {
  rhs.handle = 0;
}

auto Thread::Worker::operator=(Worker&& rhs) -> Worker& {
  handle = rhs.handle;
  rhs.handle = 0;

  return *this;
}

auto Thread::Worker::join() -> void {
  if (handle) {
    pthread_join(*Data::cast<pthread_t>(&handle), nullptr);
    handle = 0;
  }
}

auto Thread::Worker::start(Core::View::Bytes name, JobFunc func)
    -> Thread::Worker {
  // Starting workers requires a thread lock.
  pthread_mutex_lock(&worker_mutex);

  auto candidate = Algorithm::min_element<Bits_8>(thread_occupancy);
  if (thread_occupancy[candidate] != 0) {
    // If the candidate slot isn't zero then all slots must be in use.
    Diagnostics::Log::fatal(
        "Program requested to create a Thread::Worker but all 32 Workers "
        "were "
        "already occupied."_view);
  }

  // Set thread to occupied and fill out it's thread information.
  thread_occupancy[candidate] = 1;
  auto& target_info = thread_info[candidate];
  target_info.func = func;
  target_info.sink = Diagnostics::Log::get_sink();

  target_info.name_size = name.get_size();
  if (target_info.name_size > target_info.name.get_capacity() - 1) {
    Static::Bytes<256> buffer;
    Writer::Textual warning_message(buffer);
    warning_message << "Requested thread name `"_view << name
                    << "` is too long and will be shortened to `"_view
                    << name.slice(0, target_info.name.get_capacity() - 1)
                    << "`"_view;
    Diagnostics::Log::warning(warning_message);
    target_info.name_size = target_info.name.get_capacity() - 1;
  }

  target_info.name = name.slice(0, target_info.name_size);
  target_info.name[target_info.name_size] = '\0';

  // Start the worker, passing in it's thread_id.
  Thread::Worker new_worker;
  pthread_create(
      Data::cast<pthread_t>(&new_worker.handle), nullptr, thread_entry,
      (void*)candidate);

  // Make sure we release the thread lock.
  pthread_mutex_unlock(&worker_mutex);

  return new_worker;
}

auto Thread::Worker::on_main_thread() -> Bool {
  return thread_id() >= Thread::Worker::max_workers();
}

auto Thread::Worker::thread_id() -> Count {
  return Count(this_thread_id);
}

auto Thread::Worker::thread_name() -> View::Bytes {
  // If we are on the main thread then we use the
  if (on_main_thread()) {
    return "main"_view;
  }

  // If we are on a spawned thread then get the worker's registered name.
  const auto& this_thread_info = thread_info[this_thread_id];
  return this_thread_info.name.slice(0, this_thread_info.name_size);
}
