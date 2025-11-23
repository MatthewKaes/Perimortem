// Perimortem Engine
// Copyright © Matt Kaes

#include "language_server.hpp"

#include "storage/formats/lazy_json.hpp"

#include <cstring>
#include <iostream>
#include <sstream>

// TODO: This should all be async with promises

using namespace Perimortem::Memory;
using namespace Perimortem::Storage::Json;
using namespace Tetrodotoxin::Lsp;

UnixJsonRPC::UnixJsonRPC(const std::string& pipe_data_)
    : pipe_name(pipe_data_) {
  sock_descriptor = socket(AF_FILE, SOCK_STREAM, 0);
  sockaddr_un address;
  address.sun_family = AF_UNIX;
  std::strcpy(address.sun_path, pipe_name.data());

  if (connect(sock_descriptor, (sockaddr*)&address, sizeof(address)) == -1) {
    std::cout << "Failed to open " << address.sun_path << std::endl;
    return;
  }

  // Successfully started.
  valid = true;
}

UnixJsonRPC::~UnixJsonRPC() {
  close(sock_descriptor);
}

auto UnixJsonRPC::register_method(
    std::string name,
    std::function<std::string(const ManagedString&, uint32_t, const Node&)>
        resolver) -> void {
  method_resolver[name] = resolver;
}

auto UnixJsonRPC::process() -> void {
  if (!valid) {
    std::cout << "JsonRPC server is in an invalid state. Exiting..."
              << std::endl;
    return;
  }

  std::cout << "   -- Starting executor thread..." << std::endl;
  for (int i = 0; i < executor_count; i++) {
    std::cout << "   -- Starting executor thread " << i << "..." << std::endl;
    executors[i] = std::thread([this, i]() {
      uint32_t thread_id = i;
      std::string local_data;
      Arena json_arena;
      while (valid) {
        {
          std::unique_lock lock(job_mutex);
          cv.wait(lock, [this] { return job_queue.size() > 0 && valid; });
          if (!valid) {
            return;
          }
          local_data = std::move(job_queue.front());
          job_queue.pop_front();
        }

        // Use a a lazy loader to quickly rule out any invalid requests.
        LazyNode lazy_loader(ManagedString(local_data.data(), local_data.size()));
        auto header_info = lazy_loader.get_object<3>();

        auto jsonrpc_version = header_info["jsonrpc"].get_string();
        auto id_value = header_info["id"].get_int();
        auto method_name = header_info["method"].get_string();
        if (!method_resolver.contains(method_name.get_view())) {
          std::cout << "[ex=" << thread_id
                    << "] Job Rejected: " << method_name.get_view()
                    << " is not a registered RPC..." << std::endl
                    << std::endl;
          continue;
        }

        json_arena.reset();
        uint32_t position;
        std::cout << local_data << std::endl;
        auto data =
            ::parse(json_arena, local_data, position);
        if (!data) {
          std::cout << "[ex=" << thread_id
                    << "] Failed to parse jsonrpc request..." << std::endl;
          continue;
        }

        if (!data->contains("params")) {
          std::cout << "[ex=" << thread_id
                    << "] Job Rejected: " << method_name.get_view()
                    << " for missing `params`..." << std::endl;
          continue;
        }
        auto params_node = (*data)["params"];


        std::cout << "[ex=" << thread_id
                  << "] Job accepted: " << method_name.get_view() << std::endl;
        std::string response = method_resolver[method_name.get_view()](
            jsonrpc_version, id_value, *params_node);

        auto write_jsonrpc_frame = [this, &method_name](
                                       const std::string_view& view) {
          int32_t bytes_written = 0;
          while (bytes_written < view.size()) {
            int32_t bytes = write(sock_descriptor, view.data() + bytes_written,
                                  view.size() - bytes_written);
            if (bytes < 0) {
              std::cout << "Writting to socket failed while processing job: "
                        << method_name.get_view() << std::endl;
              valid = false;
              return;
            }

            bytes_written += bytes;
          }
        };

        // Write out the annoying Data Frame
        std::stringstream frame;
        frame << "Content-Length: " << response.size() << "\r\n\r\n";

        // Header frame
        write_jsonrpc_frame(frame.view());
        // JSON Response frame
        write_jsonrpc_frame(response);
        std::cout << "[ex=" << thread_id << "] Completed " << id_value
                  << std::endl;
      }
    });
  }

  std::cout << "   -- Starting reader thread..." << std::endl;
  reader = std::thread([this]() {
    // overly large buffer
    constexpr int32_t buffer_size = 1 << 16;
    char ret_value[buffer_size];
    std::string data;
    while (valid) {
      const int32_t bytes_read =
          read(sock_descriptor, ret_value, sizeof(ret_value));
      if (bytes_read < 0) {
        std::cout << "Error while reading from " << pipe_name << std::endl;
        valid = false;
        break;
      }

      if (bytes_read == 0) {
        std::cout << "Lsp pipe closed client side" << std::endl;
        valid = false;
        break;
      }

      const uint32_t original_size = data.size();
      data.resize(data.size() + bytes_read);
      std::memcpy(data.data() + original_size, ret_value, bytes_read);

      // More data to process. Tehcnically you could have an object right on
      // the edge but the odds that happens should be zero at 1KB.
      // TODO: Double check the Lsp standard.
      if (bytes_read == buffer_size) {
        continue;
      }

      // TODO: We don't currently use the Lsp / LSIF header packets.
      if (data.starts_with("Content-Length:") ||
          data.starts_with("Content-Type:")) {
        data.clear();
        continue;
      }

      {
        std::lock_guard guard(job_mutex);
        job_queue.push_back(data);
      }

      cv.notify_one();
      data.clear();
    }
  });

  std::cout << " -- TTX Service Running!" << std::endl;
  for (int i = 0; i < executor_count; i++)
    executors[i].join();
  reader.join();
}

auto UnixJsonRPC::shutdown() -> void {
  valid = true;
}