// Perimortem Engine
// Copyright © Matt Kaes

#include "puffer/lsp/rpc/executor.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/reader/binary.hpp"
#include "perimortem/core/thread/worker.hpp"
#include "perimortem/core/writer/binary.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/utility/table.hpp"

#include "puffer/lsp/methods.hpp"
#include "puffer/lsp/rpc/frame_reader.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Core;
using namespace Perimortem::Utility;
using namespace Puffer;

template <const auto& dispatch_table, Count worker_count>
auto Lsp::Rpc::Executor<dispatch_table, worker_count>::execute(
    View::Bytes pipe_name) -> void {
  Bool connected = create_connection(pipe_name);
  if (!connected) {
    Diagnostics::Log::error("RPC server is in a bad state. Exiting..."_view);
    return;
  }

  // Create a block to scope the lifetime of the workers.
  {
    Static::Vector<Thread::Worker, worker_count> workers;
    alignas(Unsigned_64) Static::Bytes<sizeof(Unsigned_64)> job_data;
    Writer::Binary<Data::ByteOrder::Native> job_writer(job_data.get_access());
    job_writer << reinterpret_cast<Unsigned_64>(this);
    for (Count i = 0; i < worker_count; i++) {
      Static::Bytes<16> name_buffer;
      Writer::Textual name_writer(name_buffer);
      name_writer << "ttx exec #"_view << i;
      workers[i] =
          Thread::Worker::start(name_writer, run_worker_job, job_writer);
    }

    Diagnostics::Log::info("TTX RPC Service now Running..."_view);
    process_events();

    Diagnostics::Log::info(
        "TTX RPC Service is shutting down, joining worker threads..."_view);
    close_connection();
  }

  Diagnostics::Log::info("Closing socket and any outstanding RPC jobs..."_view);
  close(socket_descriptor);
  socket_descriptor = -1;

  clean_up(pending_jobs_head);
  pending_jobs_head = nullptr;
  pending_jobs_tail = nullptr;
  clean_retired_jobs();
}

template <const auto& dispatch_table, Count worker_count>
auto Lsp::Rpc::Executor<dispatch_table, worker_count>::create_connection(
    View::Bytes pipe_name) -> Bool {
  socket_descriptor = socket(AF_FILE, SOCK_STREAM, 0);

  sockaddr_un address;
  address.sun_family = AF_UNIX;

  Count path_length =
      Math::min(pipe_name.get_size(), Count(sizeof(address.sun_path) - 1));
  Data::copy(
      Data::cast<Unsigned_8>(address.sun_path), pipe_name.get_data(),
      path_length);
  address.sun_path[path_length] = '\0';

  auto connect_result =
      connect(socket_descriptor, (sockaddr*)&address, sizeof(address));
  if (connect_result == -1) {
    Diagnostics::Log::Message<128> error_message(
        Diagnostics::Log::Level::Error);
    error_message << "Failed to connect to pipe at "_view << pipe_name;
    return False;
  }

  pthread_mutex_lock(&job_mutex);
  connection_open = True;
  pthread_mutex_unlock(&job_mutex);
  return True;
}

template <const auto& dispatch_table, Count worker_count>
auto Lsp::Rpc::Executor<dispatch_table, worker_count>::run_worker_job(
    View::Bytes job_data) -> void {
  Reader::Binary<Data::ByteOrder::Native> reader(job_data);
  const Unsigned_64 executor_address = reader.read_unsigned_64();
  if (reader.get_location() != reader.get_size() || executor_address == 0) {
    Diagnostics::Log::fatal("Invalid TTX RPC worker job payload."_view);
  }

  reinterpret_cast<Executor<dispatch_table, worker_count>*>(executor_address)
      ->run_worker();
}

template <const auto& dispatch_table, Count worker_count>
auto Lsp::Rpc::Executor<dispatch_table, worker_count>::write_jsonrpc_frame(
    View::Bytes view) -> void {
  if (!connection_is_open()) {
    return;
  }

  Signed_64 bytes_written = 0;
  while (static_cast<Count>(bytes_written) < view.get_size()) {
    Signed_64 bytes = write(
        socket_descriptor, view.get_data() + bytes_written,
        view.get_size() - bytes_written);
    if (bytes < 0) {
      Diagnostics::Log::error(
          "Writing RPC frame to socket failed, closing connection"_view);
      close_connection();
      return;
    }

    bytes_written += bytes;
  }
}

template <const auto& dispatch_table, Count worker_count>
auto Lsp::Rpc::Executor<dispatch_table, worker_count>::write_response(
    View::Bytes json_response) -> void {
  Static::Bytes<256> header_buffer;
  Writer::Textual content_length(header_buffer);
  content_length << "Content-Length: "_view << json_response.get_size()
                 << "\r\n\r\n"_view;

  pthread_mutex_lock(&write_mutex);
  write_jsonrpc_frame(content_length);
  write_jsonrpc_frame(json_response);
  pthread_mutex_unlock(&write_mutex);
}

template <const auto& dispatch_table, Count worker_count>
auto Lsp::Rpc::Executor<dispatch_table, worker_count>::create_job(
    View::Bytes data) -> void {
  auto allocation = Bibliotheca::check_out(sizeof(JobBlock) + data.get_size());
  Unsigned_8* job_data = allocation.ptr + sizeof(JobBlock);
  Data::copy(job_data, data.get_data(), data.get_size());
  JobBlock* job =
      new (allocation.ptr) JobBlock(View::Bytes(job_data, data.get_size()));

  pthread_mutex_lock(&job_mutex);
  if (!connection_open) {
    pthread_mutex_unlock(&job_mutex);
    destroy_job(job);
    return;
  }

  if (pending_jobs_tail) {
    pending_jobs_tail->next = job;
  } else {
    pending_jobs_head = job;
  }

  pending_jobs_tail = job;

  pthread_cond_signal(&job_signal);
  pthread_mutex_unlock(&job_mutex);
}

template <const auto& dispatch_table, Count worker_count>
auto Lsp::Rpc::Executor<dispatch_table, worker_count>::take_job() -> JobBlock* {
  pthread_mutex_lock(&job_mutex);
  while (connection_open && pending_jobs_head == nullptr) {
    pthread_cond_wait(&job_signal, &job_mutex);
  }

  if (!connection_open || pending_jobs_head == nullptr) {
    pthread_mutex_unlock(&job_mutex);
    return nullptr;
  }

  JobBlock* job = pending_jobs_head;
  pending_jobs_head = job->next;
  if (pending_jobs_head == nullptr) {
    pending_jobs_tail = nullptr;
  }

  job->next = nullptr;

  pthread_mutex_unlock(&job_mutex);
  return job;
}

template <const auto& dispatch_table, Count worker_count>
auto Lsp::Rpc::Executor<dispatch_table, worker_count>::retire_job(JobBlock* job)
    -> void {
  if (job == nullptr) {
    return;
  }

  pthread_mutex_lock(&retire_mutex);
  job->next = retired_jobs;
  retired_jobs = job;
  pthread_mutex_unlock(&retire_mutex);
}

template <const auto& dispatch_table, Count worker_count>
auto Lsp::Rpc::Executor<dispatch_table, worker_count>::clean_up(
    JobBlock* job_queue) -> void {
  while (job_queue) {
    JobBlock* job = job_queue;
    job_queue = job->next;
    destroy_job(job);
  }
}

template <const auto& dispatch_table, Count worker_count>
auto Lsp::Rpc::Executor<dispatch_table, worker_count>::clean_retired_jobs()
    -> void {
  JobBlock* jobs = nullptr;
  pthread_mutex_lock(&retire_mutex);
  jobs = retired_jobs;
  retired_jobs = nullptr;
  pthread_mutex_unlock(&retire_mutex);
  clean_up(jobs);
}

template <const auto& dispatch_table, Count worker_count>
auto Lsp::Rpc::Executor<dispatch_table, worker_count>::destroy_job(
    JobBlock* job) -> void {
  if (job == nullptr) {
    return;
  }

  job->~JobBlock();
  Bibliotheca::remit(Data::cast<Unsigned_8>(job));
}

template <const auto& dispatch_table, Count worker_count>
auto Lsp::Rpc::Executor<dispatch_table, worker_count>::process_job(
    Allocator::Arena& arena,
    JobBlock* job) -> void {
  Message message(arena, job->get_frame());
  if (!message.is_valid()) {
    Diagnostics::Log::warning("RPC message could not be decoded."_view);
    return;
  }

  auto method_name = message.get_method();
  using DispatchTable = Table<Lsp::Rpc::DispatchFunc, dispatch_table>;
  auto job_function = DispatchTable::find_or_default(method_name, nullptr);
  if (job_function == nullptr) {
    // JSON-RPC notifications never receive a response, and the LSP reserves
    // `$/*` methods for protocol notifications that a server may ignore.
    if (!message.expects_response()) {
      return;
    }

    Diagnostics::Log::Message<128> warning_message(
        Diagnostics::Log::Level::Warning);
    warning_message << "Unregistered RPC method `"_view << method_name
                    << "`"_view;

    auto error_response = message.report_error(warning_message.get_message());
    auto json_response = error_response.format(message.get_arena());
    write_response(json_response);
    return;
  }

  {
    Diagnostics::Log::Message<128> accepted_message(
        Diagnostics::Log::Level::Info);
    accepted_message << "Job Accepted: RPC method `"_view << method_name
                     << "`"_view;
  }

  auto response = job_function(documents, message);
  if (!message.expects_response()) {
    return;
  }

  write_response(response.format(message.get_arena()));
}

template <const auto& dispatch_table, Count worker_count>
auto Lsp::Rpc::Executor<dispatch_table, worker_count>::run_worker() -> void {
  Allocator::Arena arena;
  while (connection_is_open()) {
    JobBlock* job = take_job();
    if (job == nullptr) {
      return;
    }

    process_job(arena, job);
    arena.reset();
    retire_job(job);
  }
}

template <const auto& dispatch_table, Count worker_count>
auto Lsp::Rpc::Executor<dispatch_table, worker_count>::process_events()
    -> void {
  constexpr Count chunk_size = 1 << 16;
  Static::Bytes<chunk_size> chunk;
  FrameReader reader;
  while (connection_is_open()) {
    const auto bytes_read =
        read(socket_descriptor, chunk.get_data(), chunk.get_size());
    if (bytes_read < 0) {
      Diagnostics::Log::error("Error while reading from pipe"_view);
      close_connection();
      return;
    }

    if (bytes_read == 0) {
      Diagnostics::Log::info("LSP pipe was closed by the client"_view);
      close_connection();
      return;
    }

    reader.receive(chunk.slice(0, Count(bytes_read)));

    View::Bytes message = reader.next_message();
    while (!message.is_empty()) {
      clean_retired_jobs();
      create_job(message);
      reader.consume_message();
      message = reader.next_message();
    }

    clean_retired_jobs();
  }
}

template <const auto& dispatch_table, Count worker_count>
auto Lsp::Rpc::Executor<dispatch_table, worker_count>::close_connection()
    -> void {
  pthread_mutex_lock(&job_mutex);
  connection_open = False;
  pthread_cond_broadcast(&job_signal);
  pthread_mutex_unlock(&job_mutex);
}

template <const auto& dispatch_table, Count worker_count>
auto Lsp::Rpc::Executor<dispatch_table, worker_count>::connection_is_open()
    -> Bool {
  pthread_mutex_lock(&job_mutex);
  const Bool result = connection_open;
  pthread_mutex_unlock(&job_mutex);
  return result;
}

// The executor is templated over the method table so dispatch stays a static
// lookup. Puffer owns the single concrete instantiation here instead of
// carrying a runtime registration path through every server startup.
template class Lsp::Rpc::
    Executor<Lsp::method_table, Lsp::default_executor_count>;
