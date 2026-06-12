// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/lsp/server/rpc/executor.hpp"

#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/reader/textual.hpp"
#include "perimortem/core/thread/worker.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/managed/bytes.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Core;
using namespace Tetrodotoxin::Lsp;

static pthread_mutex_t job_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t retire_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t job_signal = PTHREAD_COND_INITIALIZER;

Signed_32 sock_descriptor = -1;
Bits_8* pending_jobs = nullptr;
Bits_8* retired_jobs = nullptr;
Bool connection_open = False;

struct Dispatch {
  View::Bytes name;
  Server::Rpc::Executor::DispatchFunction func;
};
Static::Vector<Dispatch, 32> dispatch_resolver;
Count dispatch_count = 0;

class Job {
 public:
  Job() {
    pthread_mutex_lock(&job_mutex);
    while (connection_open && pending_jobs == nullptr) {
      pthread_cond_wait(&job_signal, &job_mutex);
    }

    // Something went wrong with the server.
    if (!connection_open || pending_jobs == nullptr) {
      pthread_mutex_unlock(&job_mutex);
      return;
    }

    // Advance to the next job queue.
    job_source = pending_jobs;
    pending_jobs = *Data::cast<Bits_8*>(pending_jobs);

    auto job_size = *Data::cast<Count>(job_source + sizeof(Bits_8*));
    auto job_start = job_source + sizeof(Bits_8*) + sizeof(Count);
    data = View::Bytes(job_start, job_size);
    valid = true;

    // Wake up a thread in the event there are more jobs.
    pthread_cond_signal(&job_signal);
    pthread_mutex_unlock(&job_mutex);
  }

  ~Job() {
    if (job_source == nullptr) {
      return;
    }

    pthread_mutex_lock(&retire_mutex);

    // Move the job to the retired job queue so it can be freed by the thread
    // that allocated the actual data.
    Data::copy<Bits_8*>(job_source, retired_jobs);
    retired_jobs = job_source;

    pthread_mutex_unlock(&retire_mutex);
  }

  auto get_view() { return data; }
  auto is_valid() { return valid; }

 private:
  Bits_8* job_source = nullptr;
  View::Bytes data;
  Bool valid = False;
};

auto clean_up(Bits_8*& job_queue) -> void {
  while (job_queue) {
    auto job = job_queue;
    job_queue = *Data::cast<Bits_8*>(job_queue);

    Bibliotheca::remit(job);
  }
}

auto create_job(View::Bytes data) -> void {
  auto alloc =
      Bibliotheca::check_out(data.get_size() + sizeof(Bits_8*) + sizeof(Count));

  // Steal the retired list on this thread before touching job_mutex so we can
  // free it without holding either mutex (Bibliotheca is thread-local).
  Bits_8* to_clean = nullptr;
  pthread_mutex_lock(&retire_mutex);
  to_clean = retired_jobs;
  retired_jobs = nullptr;
  pthread_mutex_unlock(&retire_mutex);

  pthread_mutex_lock(&job_mutex);
  Data::copy<Bits_8*>(alloc.ptr, pending_jobs);
  Data::copy<Count>(alloc.ptr + sizeof(Bits_8*), data.get_size());
  Data::copy(
      alloc.ptr + sizeof(Bits_8*) + sizeof(Count), data.get_data(),
      data.get_size());
  pending_jobs = alloc.ptr;
  pthread_cond_broadcast(&job_signal);
  pthread_mutex_unlock(&job_mutex);

  // Free retired blocks on the reader thread which allocated them.
  clean_up(to_clean);
}

auto lookup_dispatch(View::Bytes name)
    -> Server::Rpc::Executor::DispatchFunction {
  for (Count i = 0; i < dispatch_count; i++) {
    if (dispatch_resolver[i].name == name) {
      return dispatch_resolver[i].func;
    }
  }

  return nullptr;
}

auto create_connection(View::Bytes pipe_name) -> Bool {
  sock_descriptor = socket(AF_FILE, SOCK_STREAM, 0);

  sockaddr_un address;
  address.sun_family = AF_UNIX;

  Count path_length =
      Math::min(pipe_name.get_size(), Count(sizeof(address.sun_path) - 1));
  Data::copy(
      Data::cast<Bits_8>(address.sun_path), pipe_name.get_data(), path_length);
  address.sun_path[path_length] = '\0';

  // Check if we were able to successfully connect to the pipe.
  auto connect_result =
      connect(sock_descriptor, (sockaddr*)&address, sizeof(address));
  if (connect_result == -1) {
    Static::Bytes<128> buffer;
    Writer::Textual error_message(buffer);
    error_message << "Failed to connect to pipe at "_view << pipe_name;
    Diagnostics::Log::error(error_message);
    return False;
  }

  connection_open = True;
  return True;
}

auto Server::Rpc::Executor::register_method(
    View::Bytes name,
    DispatchFunction resolver) -> void {
  dispatch_resolver[dispatch_count++] = {name, resolver};
}

auto rpc_executor() -> void {
  Allocator::Arena json_arena;

  auto write_jsonrpc_frame = [](const View::Bytes& view) {
    // Don't waste time attempting to write to a closed connection.
    if (!connection_open) {
      return;
    }

    Signed_64 bytes_written = 0;
    while (static_cast<Count>(bytes_written) < view.get_size()) {
      Signed_64 bytes = write(
          sock_descriptor, view.get_data() + bytes_written,
          view.get_size() - bytes_written);

      if (bytes < 0) {
        Diagnostics::Log::error(
            "Writing RPC frame to socket failed, closing connection"_view);
        connection_open = False;
        return;
      }
      bytes_written += bytes;
    }
  };

  while (connection_open) {
    // Try to fetch a job which will block the thread until either a valid job
    // is assigned or something went wrong with the server.
    Job assigned_job;
    if (!assigned_job.is_valid()) {
      return;
    }

    // Decode the request from the job.
    Server::Rpc::Request request(json_arena, assigned_job.get_view());
    if (!request.is_valid()) {
      Diagnostics::Log::warning(
          "Job rejected: RPC Json could not be decoded."_view);
      json_arena.reset();
      continue;
    }

    // Look up if the requested RPC is something we support.
    // The LSP spec is massive so if we don't support the request it's not
    // technically an error but we should log a warning.
    auto method_name = request.get_method();
    auto job_function = lookup_dispatch(method_name);
    if (job_function == nullptr) {
      Static::Bytes<128> buffer;
      Writer::Textual warning_message(buffer);
      warning_message << "Unregistered RPC method `"_view << method_name
                      << "`"_view;
      Diagnostics::Log::warning(warning_message);

      // Notifications have no id and expect no reply. Requests do — send a
      // Method Not Found error so the client doesn't hang waiting.
      if (request.is_request()) {
        auto error_response = request.report_error(warning_message);
        auto json_response = error_response.format(json_arena);
        Static::Bytes<256> header_buffer;
        Writer::Textual content_length(header_buffer);
        content_length << "Content-Length: "_view << json_response.get_size()
                       << "\r\n\r\n"_view;
        pthread_mutex_lock(&write_mutex);
        write_jsonrpc_frame(content_length);
        write_jsonrpc_frame(json_response);
        pthread_mutex_unlock(&write_mutex);
      }

      json_arena.reset();
      continue;
    }

    // Log that the job was accepted and what method is being proccessed.
    {
      Static::Bytes<128> buffer;
      Writer::Textual info_message(buffer);
      info_message << "Job Accepted: RPC method `"_view << method_name
                   << "`"_view;
      Diagnostics::Log::info(info_message);
    }

    auto response = job_function(request);

    // LSP notifications carry no id and expect no reply.
    if (!request.is_request()) {
      json_arena.reset();
      continue;
    }

    // Serialize the RPC response into the correct wire format.
    auto json_response = response.format(json_arena);
    Static::Bytes<256> header_buffer;
    Writer::Textual content_length(header_buffer);
    content_length << "Content-Length: "_view << json_response.get_size()
                   << "\r\n\r\n"_view;

    // Hold write_mutex for both writes so concurrent threads can't interleave
    // their header and body onto the socket.
    pthread_mutex_lock(&write_mutex);
    write_jsonrpc_frame(content_length);
    write_jsonrpc_frame(json_response);
    pthread_mutex_unlock(&write_mutex);
    json_arena.reset();
  }
}

auto try_consume_message(
    Dynamic::Bytes& data_stream,
    Bool& header_found,
    Bits_64& data_bytes_to_read) -> Bool {
  if (!header_found) {
    Count break_location =
        Algorithm::search(data_stream.get_view(), "\r\n\r\n"_view);
    if (break_location == Count(-1)) {
      return False;
    }

    constexpr auto header_info = "Content-Length:"_view;
    Count content_size_location =
        Algorithm::search(data_stream.get_view(), header_info);

    if (content_size_location != Count(-1)) {
      const auto value_start = content_size_location + header_info.get_size();
      Reader::Textual count_reader(data_stream.get_view().slice(value_start));
      data_bytes_to_read = count_reader.read_unsigned();
      header_found = True;
    }

    data_stream.shrink(break_location + "\r\n\r\n"_view.get_size());
  }

  if (header_found && data_stream.get_size() >= data_bytes_to_read) {
    create_job(data_stream.slice(0, data_bytes_to_read));
    data_stream.shrink(data_bytes_to_read);
    header_found = False;
    data_bytes_to_read = 0;
    return True;
  }

  return False;
}

auto process_events() -> void {
  constexpr Count chunk_size = 1 << 16;
  Static::Bytes<chunk_size> chunk;
  Dynamic::Bytes data_stream;

  Bool header_found = False;
  Bits_64 data_bytes_to_read = 0;

  while (connection_open) {
    const auto bytes_read =
        read(sock_descriptor, chunk.get_data(), chunk.get_size());

    if (bytes_read < 0) {
      Diagnostics::Log::error("Error while reading from pipe"_view);
      connection_open = False;
      return;
    }

    // Read is blocking so zero means the client closed the pipe.
    if (bytes_read == 0) {
      Diagnostics::Log::info("LSP pipe was closed by the client"_view);
      connection_open = False;
      return;
    }

    data_stream.concat(chunk.slice(0, Count(bytes_read)));

    // Drain all complete messages already in the buffer before going back
    // to read(). When the OS delivers multiple requests in one read call
    // this prevents blocking on read() while data is still unconsumed.
    while (try_consume_message(data_stream, header_found, data_bytes_to_read)) {
    }
  }
}

auto Server::Rpc::Executor::execute(View::Bytes pipe_name) -> void {
  if (!create_connection(pipe_name)) {
    Diagnostics::Log::error("RPC server is in an bad state. Exiting..."_view);
    return;
  }

  {
    constexpr auto executor_count = 4;
    Static::Vector<Thread::Worker, executor_count> executors;
    for (Bits_32 i = 0; i < executor_count; i++) {
      Static::Bytes<16> name_buffer;
      Writer::Textual name_writer(name_buffer);
      name_writer << "ttx exec #"_view << i;
      executors[i] = Thread::Worker::start(name_writer, rpc_executor);
    }

    Diagnostics::Log::info("TTX RPC Service now Running..."_view);
    process_events();

    // All threads are going to join if they haven't already so make sure to
    // wake them up.
    Diagnostics::Log::info(
        "TTX RPC Service is shutting down, joining worker threads..."_view);
    pthread_cond_broadcast(&job_signal);
  }

  // After all of the threads have joined perform any required clean up.
  Diagnostics::Log::info("Closing socket and any outstanding RPC jobs..."_view);
  close(sock_descriptor);
  clean_up(pending_jobs);
  clean_up(retired_jobs);
}
