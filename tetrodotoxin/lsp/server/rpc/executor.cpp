// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/lsp/server/rpc/executor.hpp"

#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/reader/binary.hpp"
#include "perimortem/core/writer/binary.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/allocator/arena.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Core;
using namespace Tetrodotoxin::Lsp;

auto Server::Rpc::Executor::register_method(
    View::Bytes name,
    DispatchFunc resolver) -> void {
  if (dispatch_count >= dispatch_resolver.get_size()) {
    Diagnostics::Log::fatal("Too many LSP methods registered."_view);
  }

  dispatch_resolver[dispatch_count++] = {name, resolver};
}

auto Server::Rpc::Executor::execute(View::Bytes pipe_name) -> void {
  if (!create_connection(pipe_name)) {
    Diagnostics::Log::error("RPC server is in a bad state. Exiting..."_view);
    return;
  }

  {
    Static::Vector<Thread::Worker, executor_count> workers;
    alignas(Bits_64) Static::Bytes<sizeof(Bits_64)> job_data;
    Writer::Binary<Data::ByteOrder::Native> job_writer(job_data.get_access());
    job_writer << reinterpret_cast<Bits_64>(this);

    for (Count i = 0; i < executor_count; i++) {
      Static::Bytes<16> name_buffer;
      Writer::Textual name_writer(name_buffer);
      name_writer << "ttx exec #"_view << i;
      workers[i] = Thread::Worker::start(
          name_writer, run_worker_job, job_writer);
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
  pending_jobs_tail = nullptr;
  clean_retired_jobs();
}

auto Server::Rpc::Executor::is_cancelled(Signed_64 id) -> Bool {
  pthread_mutex_lock(&cancel_mutex);
  for (Count i = 0; i < cancellations.get_size(); i++) {
    if (cancellations[i].active && cancellations[i].id == id) {
      pthread_mutex_unlock(&cancel_mutex);
      return True;
    }
  }
  pthread_mutex_unlock(&cancel_mutex);
  return False;
}

auto Server::Rpc::Executor::create_connection(View::Bytes pipe_name) -> Bool {
  socket_descriptor = socket(AF_FILE, SOCK_STREAM, 0);

  sockaddr_un address;
  address.sun_family = AF_UNIX;

  Count path_length =
      Math::min(pipe_name.get_size(), Count(sizeof(address.sun_path) - 1));
  Data::copy(
      Data::cast<Bits_8>(address.sun_path), pipe_name.get_data(), path_length);
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

auto Server::Rpc::Executor::run_worker_job(View::Bytes job_data) -> void {
  Reader::Binary<Data::ByteOrder::Native> reader(job_data);
  const Bits_64 executor_address = reader.read_bits_64();
  if (!reader.is_valid() || !reader.is_empty() || executor_address == 0) {
    Diagnostics::Log::fatal("Invalid TTX RPC worker job payload."_view);
  }

  reinterpret_cast<Executor*>(executor_address)->run_worker();
}

auto Server::Rpc::Executor::lookup_dispatch(View::Bytes name)
    -> DispatchFunc {
  for (Count i = 0; i < dispatch_count; i++) {
    if (dispatch_resolver[i].name == name) {
      return dispatch_resolver[i].func;
    }
  }

  return nullptr;
}

auto Server::Rpc::Executor::write_jsonrpc_frame(View::Bytes view) -> void {
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

auto Server::Rpc::Executor::write_response(View::Bytes json_response) -> void {
  Static::Bytes<256> header_buffer;
  Writer::Textual content_length(header_buffer);
  content_length << "Content-Length: "_view << json_response.get_size()
                 << "\r\n\r\n"_view;

  pthread_mutex_lock(&write_mutex);
  write_jsonrpc_frame(content_length);
  write_jsonrpc_frame(json_response);
  pthread_mutex_unlock(&write_mutex);
}

auto Server::Rpc::Executor::create_job(View::Bytes data) -> void {
  clean_retired_jobs();
  if (handle_control_message(data)) {
    return;
  }

  auto allocation = Bibliotheca::check_out(sizeof(JobBlock) + data.get_size());
  JobBlock* job = Data::cast<JobBlock>(allocation.ptr);
  job->next = nullptr;
  job->size = data.get_size();
  job->id = 0;
  job->has_id = read_job_id(data, job->id);
  job->cancelled = job->has_id && is_cancelled(job->id);
  Data::copy(job->get_access(), data.get_data(), data.get_size());

  pthread_mutex_lock(&job_mutex);
  if (!connection_open) {
    pthread_mutex_unlock(&job_mutex);
    Bibliotheca::remit(Data::cast<Bits_8>(job));
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

auto Server::Rpc::Executor::take_job() -> JobBlock* {
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

  pthread_cond_signal(&job_signal);
  pthread_mutex_unlock(&job_mutex);
  return job;
}

auto Server::Rpc::Executor::retire_job(JobBlock* job) -> void {
  if (job == nullptr) {
    return;
  }

  pthread_mutex_lock(&retire_mutex);
  job->next = retired_jobs;
  retired_jobs = job;
  pthread_mutex_unlock(&retire_mutex);
}

auto Server::Rpc::Executor::clean_up(JobBlock*& job_queue) -> void {
  while (job_queue) {
    JobBlock* job = job_queue;
    job_queue = job->next;
    Bibliotheca::remit(Data::cast<Bits_8>(job));
  }
}

auto Server::Rpc::Executor::clean_retired_jobs() -> void {
  JobBlock* jobs = nullptr;
  pthread_mutex_lock(&retire_mutex);
  jobs = retired_jobs;
  retired_jobs = nullptr;
  pthread_mutex_unlock(&retire_mutex);
  clean_up(jobs);
}

auto Server::Rpc::Executor::read_job_id(View::Bytes data, Signed_64& id)
    -> Bool {
  Allocator::Arena arena;
  Request request(arena, data);
  if (!request.is_valid() || !request.has_numeric_id()) {
    return False;
  }

  id = request.get_id();
  return True;
}

auto Server::Rpc::Executor::handle_control_message(View::Bytes data) -> Bool {
  Allocator::Arena arena;
  Request request(arena, data);
  if (!request.is_valid() || request.get_method() != "$/cancelRequest"_view) {
    return False;
  }

  const auto& id = request.get_params()["id"_view];
  if (id.is_number()) {
    cancel_request(id.get_number());
  }

  return True;
}

auto Server::Rpc::Executor::cancel_request(Signed_64 id) -> void {
  pthread_mutex_lock(&cancel_mutex);
  Count target = Count(-1);
  for (Count i = 0; i < cancellations.get_size(); i++) {
    if (cancellations[i].active && cancellations[i].id == id) {
      pthread_mutex_unlock(&cancel_mutex);
      return;
    }
    if (target == Count(-1) && !cancellations[i].active) {
      target = i;
    }
  }
  if (target != Count(-1)) {
    cancellations[target].active = True;
    cancellations[target].id = id;
  }
  pthread_mutex_unlock(&cancel_mutex);

  pthread_mutex_lock(&job_mutex);
  for (JobBlock* job = pending_jobs_head; job; job = job->next) {
    if (job->has_id && job->id == id) {
      job->cancelled = True;
    }
  }
  pthread_mutex_unlock(&job_mutex);
}

auto Server::Rpc::Executor::process_job(Allocator::Arena& arena, JobBlock* job)
    -> void {
  if (job->cancelled || (job->has_id && is_cancelled(job->id))) {
    return;
  }

  Request request(arena, job->get_data());
  if (!request.is_valid()) {
    Diagnostics::Log::warning("RPC Json could not be decoded."_view);
    return;
  }

  const Bool request_has_id = request.has_numeric_id();
  const Signed_64 request_id = request_has_id ? request.get_id() : 0;
  if (request_has_id && is_cancelled(request_id)) {
    return;
  }

  auto method_name = request.get_method();
  auto job_function = lookup_dispatch(method_name);
  if (job_function == nullptr) {
    Diagnostics::Log::Message<128> warning_message(
        Diagnostics::Log::Level::Warning);
    warning_message << "Unregistered RPC method `"_view << method_name
                    << "`"_view;

    if (request.is_request()) {
      auto error_response = request.report_error(warning_message.get_message());
      auto json_response = error_response.format(arena);
      write_response(json_response);
    }
    return;
  }

  {
    Diagnostics::Log::Message<128> accepted_message(
        Diagnostics::Log::Level::Info);
    accepted_message << "Job Accepted: RPC method `"_view << method_name
                     << "`"_view;
  }

  auto response = job_function(*this, request);

  if (!request.is_request() || (request_has_id && is_cancelled(request_id))) {
    return;
  }

  write_response(response.format(arena));
}

auto Server::Rpc::Executor::run_worker() -> void {
  Allocator::Arena arena;
  while (connection_is_open()) {
    JobBlock* job = take_job();
    if (job == nullptr) {
      return;
    }

    process_job(arena, job);
    retire_job(job);
    arena.reset();
  }
}

auto Server::Rpc::Executor::process_events() -> void {
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

    reader.append(chunk.slice(0, Count(bytes_read)));

    View::Bytes message = reader.next_message();
    while (!message.is_empty()) {
      create_job(message);
      reader.consume_message();
      message = reader.next_message();
    }
  }
}

auto Server::Rpc::Executor::close_connection() -> void {
  pthread_mutex_lock(&job_mutex);
  connection_open = False;
  pthread_cond_broadcast(&job_signal);
  pthread_mutex_unlock(&job_mutex);
}

auto Server::Rpc::Executor::connection_is_open() -> Bool {
  pthread_mutex_lock(&job_mutex);
  const Bool result = connection_open;
  pthread_mutex_unlock(&job_mutex);
  return result;
}
