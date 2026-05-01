// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/diagnostics.hpp"

#include "perimortem/memory/static/bytes.hpp"

using namespace Perimortem::System;
using namespace Perimortem::Memory;

Diagnostics::Severity log_level = Diagnostics::Severity::Info;
Bool flush_to_stdout = false;
Bool flush_to_log = false;
Static::Bytes<1 << 9> log_file;
Static::Bytes<1 << 12> log_buffer;
Count write_ptr = 0;

auto Diagnostics::set_level(Severity minimum) -> void {
  log_level = minimum;
}
auto Diagnostics::enable_stdout(Bool enable) -> void {
  flush_to_stdout = enable;
}

auto Diagnostics::enable_log(Bool enable) -> void {
  flush_to_log = enable;
}

Diagnostics::Diagnostics() {}

Diagnostics::~Diagnostics() {
  flush();
}

// auto Diagnostics::set_level(Severity minimum) -> void {
//   log_level = minimum;
// }

// auto Diagnostics::enable_stdout(Bool enable)  -> void{
//   flush_to_stdout = enable;
// }

// auto Diagnostics::enable_log(Bool enable)  -> void{
//   flush_to_log = enable;
// }

// auto Diagnostics::set_log_file(Core::View::Amorphous log_path) -> void {
//   Managed::Buffer log_name(log_file);
//   log_name
//   std::time_t time = std::time({});
//   std::strftime(log_file, std::size(log_file), "%F",
//                 std::gmtime(&time));
//   log_file
// }

// auto Diagnostics::info(const Core::View::Amorphous msg,
//                        const std::source_location location) {
//   std::scoped_lock lock(buffer_mutex)
// }
// auto Diagnostics::debug(
//     const Core::View::Amorphous error,
//     const std::source_location location = std::source_location::current());
// auto Diagnostics::warning(
//     const Core::View::Amorphous error,
//     const std::source_location location = std::source_location::current());
// auto Diagnostics::error(
//     const Core::View::Amorphous error,
//     const std::source_location location = std::source_location::current());

// auto Diagnostics::flush() -> void{
//   if (write_ptr > 0) {

//   }

//   write_ptr = 0;
// }

// auto write_level(Severity level) -> void {

// }
// auto write_message(std::initializer_list<Data> data) -> void {

// }

// auto Diagnostics::write_utc_stamp() -> void {
//   // Example of the very popular RFC 3339 format UTC time
//   std::time_t time = std::time({});
//   char timeString[sizeof("yyyy-mm-dd hh:mm:ss")];
//   std::strftime(std::data(timeString), std::size(timeString), "%F %T",
//                 std::gmtime(&time));
//   std::cout << timeString << '\n';
// }
