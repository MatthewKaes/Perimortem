// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/diagnostics/log.hpp"

#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Diagnostics;

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef PERI_LINUX
#include <execinfo.h>
#endif

static Bits_64 next_thread_id = 0;
static thread_local Bits_64 this_thread_id =
    __atomic_fetch_add(&next_thread_id, 1, __ATOMIC_RELAXED);
static thread_local Log::Sink message_sink = Log::file_sink;
static thread_local Log::Level thread_log_level = Log::Level::Info;

struct TimeData {
  Byte hour;
  Byte min;
  Byte sec;
  Long millis;
};

auto get_utc() -> TimeData {
  timespec os_time;
  clock_gettime(CLOCK_REALTIME, &os_time);
  Long seconds_today = os_time.tv_sec % 86400;
  TimeData result{};
  result.hour = Byte(seconds_today / 3600);
  result.min = Byte((seconds_today % 3600) / 60);
  result.sec = Byte(seconds_today % 60);
  result.millis = os_time.tv_nsec / 1000000;
  return result;
}

auto level_char(Log::Level level) -> Byte {
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

auto level_color(Log::Level level) -> View::Bytes {
  switch (level) {
  case Log::Level::Debug:
    return "\x1B[2m"_view;
  case Log::Level::Info:
    return "\x1B[32m"_view;
  case Log::Level::Warning:
    return "\x1B[33m"_view;
  case Log::Level::Error:
    return "\x1B[31m"_view;
  case Log::Level::Fatal:
    return "\x1B[1;31m"_view;
  }
  return ""_view;
}

auto write_padded(Writer::Textual& writer, ULong value, Count width) -> void {
  Count digit_count = 1;
  ULong temp = value;
  while (temp >= 10) {
    temp /= 10;
    digit_count++;
  }
  for (Count pad = digit_count; pad < width; pad++) {
    writer << Byte('0');
  }
  writer << value;
}

constexpr Count max_message_capacity = 1 << 11;

auto format_entry(
    Log::Level level,
    View::Bytes msg,
    const Source& loc,
    Access::Bytes buf) -> Count {
  Writer::Textual writer(buf);

  writer << level_char(level) << Byte(' ');

  auto t = get_utc();
  write_padded(writer, ULong(t.hour), 2);
  writer << Byte(':');
  write_padded(writer, ULong(t.min), 2);
  writer << Byte(':');
  write_padded(writer, ULong(t.sec), 2);
  writer << Byte('.');
  write_padded(writer, ULong(t.millis), 3);

  writer << Byte(' ') << Byte('[');
  if (this_thread_id == 0) {
    writer << "main"_view;
  } else {
    writer << "unknown #"_view << ULong(this_thread_id);
  }
  writer << Byte(']') << Byte(' ');

  writer << loc.get_file();
  writer << Byte(':') << Long(loc.get_line());
  writer << Byte(':') << Long(loc.get_column());
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
    file_ready = True;

    Static::Bytes<512> name_buf;
    // Reserve the last byte for the null terminator.
    Access::Bytes name_access(name_buf.get_data(), name_buf.get_size() - 1);
    Writer::Textual name_writer(name_access);

    name_writer << "perimortem_"_view;
    auto t = get_utc();
    write_padded(name_writer, ULong(t.hour), 2);
    name_writer << Byte('-');
    write_padded(name_writer, ULong(t.min), 2);
    name_writer << Byte('-');
    write_padded(name_writer, ULong(t.sec), 2);
    name_writer << Byte('_') << ULong(this_thread_id);
    name_writer << ".log"_view;
    name_buf[name_writer.get_location()] = Byte('\0');

    file = fopen(Data::cast<char>(name_buf.get_data()), "ab");
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

thread_local ThreadWriter thread_writer;

auto Log::file_sink(Level level, View::Bytes message) -> void {
  thread_writer.accumulate(message);
}

auto Log::console_sink(Level level, View::Bytes message) -> void {
  FILE* stream = (level >= Level::Error) ? stderr : stdout;
  fwrite(message.get_data(), 1, message.get_size(), stream);
}

auto Log::color_sink(Level level, View::Bytes message) -> void {
  if (message.empty()) {
    return;
  }

  // Format: [color][level char][reset][rest of message]
  // The +16 gives headroom for the two ANSI escape sequences.
  Static::Bytes<max_message_capacity + 16> buf;
  Access::Bytes access(buf.get_data(), buf.get_size());
  Writer::Textual writer(access);

  writer << level_color(level) << message[0] << "\x1B[0m"_view;
  writer << message.slice(1);

  FILE* stream = (level >= Level::Error) ? stderr : stdout;
  fwrite(buf.get_data(), 1, writer.get_location(), stream);
}

auto Log::debug_sink(Level level, View::Bytes message) -> void {
  console_sink(level, message);
  file_sink(level, message);
}

auto Log::set_sink(Sink sink) -> void {
  message_sink = sink;
}

auto Log::set_level(Level level) -> void {
  thread_log_level = level;
}

auto Log::log(Level level, View::Bytes msg, const Source& loc) -> void {
  if (level < thread_log_level || !message_sink) {
    return;
  }

  Static::Bytes<max_message_capacity> buf;
  Access::Bytes access(buf.get_data(), buf.get_size());
  Count len = format_entry(level, msg, loc, access);
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
