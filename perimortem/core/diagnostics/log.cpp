// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/diagnostics/log.hpp"

using namespace Perimortem::Core::Diagnostics;

Log::Severity log_level = Log::Severity::Info;
Bool flush_to_stdout = false;
Bool flush_to_log = false;

constexpr auto max_log_file_size = 1 << 9;
Byte log_file[max_log_file_size];
Count log_file_size = 0;

constexpr auto log_buffer_size = 1 << 12;
Byte log_buffer[log_buffer_size];
Count write_ptr = 0;

auto Log::set_level(Severity minimum) -> void {
  log_level = minimum;
}
auto Log::enable_stdout(Bool enable) -> void {
  flush_to_stdout = enable;
}

auto Log::enable_log(Bool enable) -> void {
  flush_to_log = enable;
}

Log::Log() {}

Log::~Log() {
  flush();
}

// auto Log::set_level(Severity minimum) -> void {
//   log_level = minimum;
// }

// auto Log::enable_stdout(Bool enable)  -> void{
//   flush_to_stdout = enable;
// }

// auto Log::enable_log(Bool enable)  -> void{
//   flush_to_log = enable;
// }

// auto Log::set_log_file(Core::View::Amorphous log_path) -> void {
//   Managed::Buffer log_name(log_file);
//   log_name
//   std::time_t time = std::time({});
//   std::strftime(log_file, std::size(log_file), "%F",
//                 std::gmtime(&time));
//   log_file
// }

// auto Log::info(const Core::View::Amorphous msg,
//                        const std::source_location location) {
//   std::scoped_lock lock(buffer_mutex)
// }
// auto Log::debug(
//     const Core::View::Amorphous error,
//     const std::source_location location = std::source_location::current());
// auto Log::warning(
//     const Core::View::Amorphous error,
//     const std::source_location location = std::source_location::current());
// auto Log::error(
//     const Core::View::Amorphous error,
//     const std::source_location location = std::source_location::current());

// auto Log::flush() -> void{
//   if (write_ptr > 0) {

//   }

//   write_ptr = 0;
// }

// auto write_level(Severity level) -> void {

// }
// auto write_message(std::initializer_list<Data> data) -> void {

// }

// auto Log::write_utc_stamp() -> void {
//   // Example of the very popular RFC 3339 format UTC time
//   std::time_t time = std::time({});
//   char timeString[sizeof("yyyy-mm-dd hh:mm:ss")];
//   std::strftime(std::data(timeString), std::size(timeString), "%F %T",
//                 std::gmtime(&time));
//   std::cout << timeString << '\n';
// }
