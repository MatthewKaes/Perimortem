// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/system/diagnostics.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include <filesystem>
#include <iostream>
#include <mutex>

std::mutex buffer_mutex;

using namespace Perimortem::System;
using namespace Perimortem::Memory;

Diagnostics::Diagnostics() {
  std::time_t time = std::time({});
  std::strftime(log_file, std::size(log_file), "%F",
                std::gmtime(&time));
}

Diagnostics::~Diagnostics() {
  flush();
}

auto Diagnostics::set_level(Severity minimum) -> void {
  log_level = minimum;
}

auto Diagnostics::enable_stdout(bool enable)  -> void{
  flush_to_stdout = enable;
}

auto Diagnostics::enable_log(bool enable)  -> void{
  flush_to_log = enable;
}

auto Diagnostics::set_log_file(Memory::View::Bytes log_path) -> void {
  Managed::Buffer log_name(log_file);
  log_name
  std::time_t time = std::time({});
  std::strftime(log_file, std::size(log_file), "%F",
                std::gmtime(&time));
  log_file
}
}
auto Diagnostics::info(const Memory::View::Bytes msg,
                       const std::source_location location) {
  std::scoped_lock lock(buffer_mutex)
}
auto Diagnostics::debug(
    const Memory::View::Bytes error,
    const std::source_location location = std::source_location::current());
auto Diagnostics::warning(
    const Memory::View::Bytes error,
    const std::source_location location = std::source_location::current());
auto Diagnostics::error(
    const Memory::View::Bytes error,
    const std::source_location location = std::source_location::current());

auto Diagnostics::flush() -> void{
  if (write_ptr > 0) {

  }

  write_ptr = 0;
}

auto write_level(Severity level) -> void {

}
auto write_message(std::initializer_list<Data> data) -> void {

}
  
auto Diagnostics::write_utc_stamp() -> void {
  // Example of the very popular RFC 3339 format UTC time
  std::time_t time = std::time({});
  char timeString[sizeof("yyyy-mm-dd hh:mm:ss")];
  std::strftime(std::data(timeString), std::size(timeString), "%F %T",
                std::gmtime(&time));
  std::cout << timeString << '\n';
}
