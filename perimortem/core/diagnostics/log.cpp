// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/diagnostics/log.hpp"

#include <stdio.h>
#include <stdlib.h>

#ifdef PERI_LINUX
#include <execinfo.h>
#endif

#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/thread/worker.hpp"
#include "perimortem/core/time.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Core::Diagnostics;

constexpr Count max_message_capacity = 1 << 11;

class ThreadWriter {
 public:
  ~ThreadWriter() {
    if (file) {
      fflush(file);
      fclose(file);
      file = nullptr;
    }
  }

  auto prepare_file() -> void {
    Static::Bytes<512> name_buffer;
    Writer::Textual writer(name_buffer.get_access().slice(0, 511));

    writer << "perimortem_"_view;
    writer << "["_view << Thread::Worker::get_thread_name() << "]_"_view;

    writer << Time::now().get_stamp();
    writer << ".log"_view;
    name_buffer[writer.get_location()] = '\0';

    file = fopen(Data::cast<char>(name_buffer.get_data()), "ab");
    file_ready = True;
  }

  auto accumulate(View::Bytes entry) -> void {
    if (!file_ready) {
      prepare_file();
    }

    if (!file) {
      return;
    }

    fwrite(entry.get_data(), 1, entry.get_size(), file);
  }

  auto flush() -> void {
    if (file) {
      fflush(file);
    }
  }

 private:
  FILE* file = nullptr;
  Bool file_ready = False;
};

static thread_local Log::Sink message_sink = Log::default_sink;
static thread_local Log::Level thread_log_level = Log::Level::Info;
static thread_local Bool disable_log_header = False;
static thread_local Source attribution_override;
static thread_local ThreadWriter thread_writer;

constexpr auto level_char(Log::Level level) -> Signed_8 {
  switch (level) {
  case Log::Level::Debug:
    return 'D';
  case Log::Level::Info:
    return 'I';
  case Log::Level::Warning:
    return 'W';
  case Log::Level::Error:
    return 'E';
  case Log::Level::Fatal:
    return 'F';
  }
  return '?';
}

constexpr auto level_color(Log::Level level) -> View::Bytes {
  switch (level) {
  case Log::Level::Debug:
    return "\x1b[38;5;246m"_view;
  case Log::Level::Info:
    return ""_view;
  case Log::Level::Warning:
    return "\x1b[38;5;220m"_view;
  case Log::Level::Error:
  case Log::Level::Fatal:
    return "\x1b[38;5;160m"_view;
  }
  return ""_view;
}

static auto format_message(
    Log::Level level,
    View::Bytes message,
    const Source& location,
    Access::Bytes output) -> Count {
  if (!disable_log_header) {
    return Log::format_entry(level, message, location, output);
  }

  Writer::Textual writer(output);
  if (location.is_set()) {
    writer << location.get_file() << ':' << location.get_line() << ':'
           << location.get_column() << ": "_view;
  }
  writer << message;
  return writer.get_location();
}

Log::Attribution::Attribution(Attribution&& rhs) {
  primary_guard = rhs.primary_guard;
  rhs.primary_guard = False;
}

Log::Attribution::~Attribution() {
  if (primary_guard) {
    attribution_override = Source();
  }
}

auto Log::file_sink(Level level, View::Bytes message, const Source& location)
    -> void {
  Static::Bytes<max_message_capacity> message_buffer;
  Count message_length =
      format_message(level, message, location, message_buffer.get_access());
  thread_writer.accumulate(message_buffer.slice(0, message_length));
}

auto Log::console_sink(Level level, View::Bytes message, const Source& location)
    -> void {
  Static::Bytes<max_message_capacity> message_buffer;
  Count message_length =
      format_message(level, message, location, message_buffer.get_access());
  View::Bytes formatted = message_buffer.slice(0, message_length);
  FILE* stream = (level >= Level::Error) ? stderr : stdout;
  fwrite(formatted.get_data(), 1, formatted.get_size(), stream);
}

auto Log::color_sink(Level level, View::Bytes message, const Source& location)
    -> void {
  Static::Bytes<max_message_capacity> entry_buffer;
  Count entry_length =
      format_message(level, message, location, entry_buffer.get_access());
  View::Bytes entry = entry_buffer.slice(0, entry_length);

  if (message.is_empty()) {
    return;
  }

  // 16 extra bytes is enough for any color code + the clear code.
  Static::Bytes<max_message_capacity + 16> color_buffer;
  Writer::Textual writer(color_buffer.get_access());

  writer << level_color(level);
  writer << entry;
  writer << "\x1b[0m"_view;

  FILE* stream = (level >= Level::Error) ? stderr : stdout;
  fwrite(color_buffer.get_data(), 1, writer.get_location(), stream);
}

auto Log::stderr_sink(Level level, View::Bytes message, const Source& location)
    -> void {
  Static::Bytes<max_message_capacity> message_buffer;
  Count message_length =
      format_message(level, message, location, message_buffer.get_access());
  View::Bytes formatted = message_buffer.slice(0, message_length);
  fwrite(formatted.get_data(), 1, formatted.get_size(), stderr);
  fflush(stderr);
}

auto Log::debug_sink(Level level, View::Bytes message, const Source& location)
    -> void {
  console_sink(level, message, location);
  file_sink(level, message, location);
}

auto Log::set_sink(Sink sink) -> void {
  message_sink = sink;
}

auto Log::get_sink() -> Sink {
  return message_sink;
}

auto Log::set_disable_header(Bool disable_header) -> void {
  disable_log_header = disable_header;
}

auto Log::get_disable_header() -> Bool {
  return disable_log_header;
}

auto Log::set_level(Level level) -> void {
  thread_log_level = level;
}

auto Log::set_attribution(const Source& location) -> Attribution {
  // If we have someone already claiming attribution higher on the stack then
  // ignore the request.
  // If there is no attribution then create an attribution point.
  Attribution scope_guard;
  scope_guard.primary_guard = !attribution_override.is_set();
  if (scope_guard.primary_guard) {
    attribution_override = location;
  }

  return scope_guard;
}

auto Log::log(Level level, View::Bytes message, const Source& location)
    -> void {
  if (level < thread_log_level || !message_sink) {
    return;
  }

  const Source& target_source =
      attribution_override.is_set() ? attribution_override : location;
  message_sink(level, message, target_source);
}

auto Log::format_entry(
    Log::Level level,
    View::Bytes message,
    const Source& location,
    Access::Bytes output) -> Count {
  Writer::Textual writer(output);

  writer << level_char(level) << ' ';
  writer << Time::now().calculate_clock() << ' ';
  writer << "["_view << Thread::Worker::get_thread_name() << "] "_view;

  writer << location.get_file();
  writer << ':' << location.get_line();
  writer << ':' << location.get_column();
  writer << ": "_view << message << '\n';

  return writer.get_location();
}

auto Log::debug(View::Bytes message, const Source& location) -> void {
  log(Level::Debug, message, location);
}

auto Log::info(View::Bytes message, const Source& location) -> void {
  log(Level::Info, message, location);
}

auto Log::warning(View::Bytes message, const Source& location) -> void {
  log(Level::Warning, message, location);
}

auto Log::error(View::Bytes message, const Source& location) -> void {
  log(Level::Error, message, location);
}

auto Log::fatal(View::Bytes message, const Source& location) -> void {
  log(Level::Fatal, message, location);
  flush();

#ifdef PERI_LINUX
  Static::Vector<void*, 64> frames;
  int frame_count = backtrace(frames.get_data(), frames.get_size());
  backtrace_symbols_fd(frames.get_data(), frame_count, 2);
#endif

  abort();
}

auto Log::flush() -> void {
  thread_writer.flush();
}
