// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <pthread.h>

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/puffer/lsp/documents.hpp"
#include "tetrodotoxin/puffer/lsp/rpc/message.hpp"

namespace Tetrodotoxin::Puffer::Lsp::Rpc {

using DispatchFunc = Response (*)(Documents&, const Message&);

template <const auto& dispatch_table, Count worker_count>
class Executor {
 public:
  auto execute(Perimortem::Core::View::Bytes pipe_name) -> void;

 private:
  class JobBlock {
   public:
    explicit JobBlock(Perimortem::Core::View::Bytes frame) : frame(frame) {}

    auto get_frame() const -> Perimortem::Core::View::Bytes { return frame; }

    JobBlock* next = nullptr;

   private:
    Perimortem::Core::View::Bytes frame;
  };

  auto create_connection(Perimortem::Core::View::Bytes pipe_name) -> Bool;
  static auto run_worker_job(Perimortem::Core::View::Bytes job_data) -> void;
  auto write_jsonrpc_frame(Perimortem::Core::View::Bytes view) -> void;
  auto write_response(Perimortem::Core::View::Bytes json_response) -> void;
  auto create_job(Perimortem::Core::View::Bytes data) -> void;
  auto take_job() -> JobBlock*;
  auto retire_job(JobBlock* job) -> void;
  auto clean_up(JobBlock* job_queue) -> void;
  auto clean_retired_jobs() -> void;
  auto destroy_job(JobBlock* job) -> void;
  auto process_job(
      Perimortem::Memory::Allocator::Arena& arena,
      JobBlock* job) -> void;
  auto run_worker() -> void;
  auto process_events() -> void;
  auto close_connection() -> void;
  auto connection_is_open() -> Bool;

  Documents documents;
  Signed_32 socket_descriptor = -1;

  pthread_mutex_t job_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t retire_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t job_signal = PTHREAD_COND_INITIALIZER;

  JobBlock* pending_jobs_head = nullptr;
  JobBlock* pending_jobs_tail = nullptr;
  JobBlock* retired_jobs = nullptr;
  Bool connection_open = False;
};

}  // namespace Tetrodotoxin::Puffer::Lsp::Rpc
