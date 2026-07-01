// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <pthread.h>

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/thread/worker.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/lsp/server/documents.hpp"
#include "tetrodotoxin/lsp/server/rpc/frame_reader.hpp"
#include "tetrodotoxin/lsp/server/rpc/request.hpp"

namespace Tetrodotoxin::Lsp::Server::Rpc {

class Executor {
 public:
  using DispatchFunc = Rpc::Response (*)(Executor&, const Rpc::Request&);

  auto register_method(
      Perimortem::Core::View::Bytes name,
      DispatchFunc resolver) -> void;
  auto execute(Perimortem::Core::View::Bytes pipe_name) -> void;
  auto get_documents() -> Documents& { return documents; }
  auto is_cancelled(Signed_64 id) -> Bool;

 private:
  class Dispatch {
   public:
    Perimortem::Core::View::Bytes name;
    DispatchFunc func;
  };

  class CancelRecord {
   public:
    Signed_64 id = 0;
    Bool active = False;
  };

  class JobBlock {
   public:
    auto get_data() const -> Perimortem::Core::View::Bytes {
      return Perimortem::Core::View::Bytes(
          Perimortem::Core::Data::cast<const Bits_8>(this + 1), size);
    }
    auto get_access() -> Bits_8* {
      return Perimortem::Core::Data::cast<Bits_8>(this + 1);
    }

    JobBlock* next = nullptr;
    Count size = 0;
    Signed_64 id = 0;
    Bool has_id = False;
    Bool cancelled = False;
  };

  auto create_connection(Perimortem::Core::View::Bytes pipe_name) -> Bool;
  static auto run_worker_job(Perimortem::Core::View::Bytes job_data) -> void;
  auto lookup_dispatch(Perimortem::Core::View::Bytes name) -> DispatchFunc;
  auto write_jsonrpc_frame(Perimortem::Core::View::Bytes view) -> void;
  auto write_response(Perimortem::Core::View::Bytes json_response) -> void;
  auto create_job(Perimortem::Core::View::Bytes data) -> void;
  auto take_job() -> JobBlock*;
  auto retire_job(JobBlock* job) -> void;
  auto clean_up(JobBlock*& job_queue) -> void;
  auto clean_retired_jobs() -> void;
  auto read_job_id(Perimortem::Core::View::Bytes data, Signed_64& id) -> Bool;
  auto handle_control_message(Perimortem::Core::View::Bytes data) -> Bool;
  auto cancel_request(Signed_64 id) -> void;
  auto process_job(Perimortem::Memory::Allocator::Arena& arena, JobBlock* job)
      -> void;
  auto run_worker() -> void;
  auto process_events() -> void;
  auto close_connection() -> void;
  auto connection_is_open() -> Bool;

  static constexpr Count executor_count = 4;

  Documents documents;
  Perimortem::Core::Static::Vector<Dispatch, 32> dispatch_resolver;
  Perimortem::Core::Static::Vector<CancelRecord, 64> cancellations;
  Count dispatch_count = 0;
  Signed_32 socket_descriptor = -1;

  pthread_mutex_t job_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t retire_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t cancel_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t job_signal = PTHREAD_COND_INITIALIZER;

  JobBlock* pending_jobs_head = nullptr;
  JobBlock* pending_jobs_tail = nullptr;
  JobBlock* retired_jobs = nullptr;
  Bool connection_open = False;
};

}  // namespace Tetrodotoxin::Lsp::Server::Rpc
