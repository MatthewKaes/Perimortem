// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/thread/worker.hpp"
#include "perimortem/core/time.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Core::Diagnostics;

#include <stdio.h>
#include <stdlib.h>

#ifdef PERI_LINUX
#include <execinfo.h>
#endif

static thread_local Log::Sink message_sink = Log::default_sink;
static thread_local Log::Level thread_log_level = Log::Level::Info;
static thread_local Source attribution_override;

constexpr auto level_char(Log::Level level) -> Byte {
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

constexpr Count max_message_capacity = 1 << 11;

auto format_entry(
    Log::Level level,
    View::Bytes msg,
    const Source& location,
    Access::Bytes buf) -> Count {
  Writer::Textual writer(buf);

  writer << level_char(level) << Byte(' ');
  writer << Time::now().calculate_clock() << Byte(' ');
  writer << "["_view << Thread::Worker::thread_name() << "] "_view;

  const Source& target_source =
      attribution_override.is_set() ? attribution_override : location;
  writer << target_source.get_file();
  writer << Byte(':') << Long(target_source.get_line());
  writer << Byte(':') << Long(target_source.get_column());
  writer << ": "_view << msg << Byte('\n');

  return writer.get_location();
}

struct ThreadWriter {
  FILE* file = nullptr;
  Bool file_ready = False;

  ~ThreadWriter() {
    if (file) {
      fflush(file);
      fclose(file);
      file = nullptr;
    }
  }

  auto prepare_file() -> void {
    Static::Bytes<512> name_buf;
    // Reserve the last byte for the null terminator.
    Access::Bytes name_access(name_buf.get_data(), name_buf.get_size() - 1);
    Writer::Textual writer(name_access);

    writer << "perimortem_"_view;
    writer << "["_view << Thread::Worker::thread_name() << "]_"_view;

    writer << Time::now().get_stamp();
    writer << ".log"_view;
    name_buf[writer.get_location()] = Byte('\0');

    file = fopen(Data::cast<char>(name_buf.get_data()), "ab");
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
};

static thread_local ThreadWriter thread_writer;

auto Log::file_sink(Level level, View::Bytes message) -> void {
  thread_writer.accumulate(message);
}

auto Log::console_sink(Level level, View::Bytes message) -> void {
  FILE* stream = (level >= Level::Error) ? stderr : stdout;
  fwrite(message.get_data(), 1, message.get_size(), stream);
}

auto Log::stderr_sink(Level /*level*/, View::Bytes message) -> void {
  fwrite(message.get_data(), 1, message.get_size(), stderr);
  fflush(stderr);
}

auto Log::color_sink(Level level, View::Bytes message) -> void {
  if (message.empty()) {
    return;
  }

  // 16 extra bytes is enough for any color code + the clear code.
  Static::Bytes<max_message_capacity + 16> buf;
  Access::Bytes access(buf.get_data(), buf.get_size());
  Writer::Textual writer(access);

  writer << level_color(level);
  writer << message;
  writer << "\x1b[0m"_view;

  console_sink(level, writer);
}

auto Log::debug_sink(Level level, View::Bytes message) -> void {
  console_sink(level, message);
  file_sink(level, message);
}

auto Log::set_sink(Sink sink) -> void {
  message_sink = sink;
}

auto Log::get_sink() -> Sink {
  return message_sink;
}

auto Log::set_level(Level level) -> void {
  thread_log_level = level;
}

Log::Attribution::~Attribution() {
  if (primary_guard) {
    attribution_override = Source();
  }
}

Log::Attribution::Attribution(Attribution&& rhs) {
  primary_guard = rhs.primary_guard;
  rhs.primary_guard = False;
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

auto Log::log(Level level, View::Bytes msg, const Source& location) -> void {
  if (level < thread_log_level || !message_sink) {
    return;
  }

  Static::Bytes<max_message_capacity> buf;
  Access::Bytes access(buf.get_data(), buf.get_size());
  Count len = format_entry(level, msg, location, access);
  message_sink(level, View::Bytes(buf.get_data(), len));
}

auto Log::debug(View::Bytes msg, const Source& location) -> void {
  log(Level::Debug, msg, location);
}

auto Log::info(View::Bytes msg, const Source& location) -> void {
  log(Level::Info, msg, location);
}

auto Log::warning(View::Bytes msg, const Source& location) -> void {
  log(Level::Warning, msg, location);
}

auto Log::error(View::Bytes msg, const Source& location) -> void {
  log(Level::Error, msg, location);
}

auto Log::fatal(View::Bytes msg, const Source& location) -> void {
  log(Level::Fatal, msg, location);
  flush();

#ifdef PERI_LINUX
  void* frames[64];
  int frame_count = backtrace(frames, 64);
  backtrace_symbols_fd(frames, frame_count, 2);
#endif

  abort();
}

auto Log::flush() -> void {
  thread_writer.flush();
}

auto Log::set_thread_name(View::Bytes /*name*/) -> void {
  // Thread names are managed by Thread::Worker via thread_info; nothing to do.
}
