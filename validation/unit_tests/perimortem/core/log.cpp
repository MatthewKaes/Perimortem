// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/diagnostics/log.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Core::Diagnostics;
using namespace Validation;

constexpr Count max_message_length = 256;

struct LogEvent {
  Static::Bytes<max_message_length> message;
  Count message_size;
};

constexpr Count event_log_size = 8;
static Static::Vector<LogEvent, event_log_size> log_events;
static Count total_events = 0;

static auto capture_sink(Log::Level level, View::Bytes message) -> void {
  Count index = total_events++ % event_log_size;
  log_events[index].message_size =
      Math::min(max_message_length, message.get_size());
  log_events[index].message = message;
}

static auto last_entry() -> View::Bytes {
  auto& event = log_events[(total_events - 1) % event_log_size];
  return View::Bytes(event.message.get_data(), event.message_size);
}

static auto contains(View::Bytes haystack, View::Bytes message) -> Bool {
  return Algorithm::search(haystack, message) != Count(-1);
}

static auto has_valid_header(View::Bytes entry) -> Bool {
  constexpr auto header_length = 14;
  if (entry.get_size() < header_length) {
    return false;
  }

  // Header bytes
  switch (entry[0]) {
  case 'D':
  case 'I':
  case 'W':
  case 'E':
  case 'F':
    break;
  default:
    return false;
  }

  // Validate all number values
  const Byte* b = entry.get_data() + 2;
  if (b[2] != ':' || b[5] != ':' || b[8] != '.') {
    return false;
  }

  // Validate all number values
  constexpr Static::Vector<Count, 9> number_indexes = {
    {0, 1, 3, 4, 6, 7, 9, 10, 11}};
  for (Count i = 0; i < number_indexes.get_size(); i++) {
    if (b[number_indexes[i]] < '0' || b[number_indexes[i]] > '9') {
      return false;
    }
  }

  return true;
}

static Harness DiagnosticsLog = {
  .name = "Diagnostics::Log"_view,
  .setup =
      []() {
        Log::set_sink(capture_sink);
        Log::set_level(Log::Level::Debug);
        total_events = 0;
      },
  .teardown =
      []() {
        Log::set_sink(Log::default_sink);
        Log::set_level(Log::Level::Info);
        total_events = 0;
      },
};

PERIMORTEM_UNIT_TEST(DiagnosticsLog, timestamp_format) {
  Log::info("timestamp test"_view);
  ASSERT(last_entry().get_size() > 0);
  EXPECT(has_valid_header(last_entry()));
}

PERIMORTEM_UNIT_TEST(DiagnosticsLog, newline_terminator) {
  Log::info("newline test"_view);
  View::Bytes entry = last_entry();
  ASSERT(entry.get_size() > 0);
  EXPECT_EQ(entry[entry.get_size() - 1], Byte('\n'));
}

PERIMORTEM_UNIT_TEST(DiagnosticsLog, source_location) {
  Log::info("location test"_view);
  EXPECT(contains(
      last_entry(), "validation/unit_tests/perimortem/core/log.cpp:"_view));
}

PERIMORTEM_UNIT_TEST(DiagnosticsLog, main_thread_name) {
  Log::info("thread name test"_view);
  ASSERT(last_entry().get_size() > 0);
  EXPECT(contains(last_entry(), "[main]"_view));
}

PERIMORTEM_UNIT_TEST(DiagnosticsLog, thread_name_after_timestamp) {
  Log::info("ordering test"_view);
  View::Bytes entry = last_entry();
  ASSERT(entry.get_size() > 21);
  // Format: "X HH:MM:SS.mmm [main] ..."
  EXPECT_TEXT(entry.slice(15, 6), "[main]"_view);
}

PERIMORTEM_UNIT_TEST(DiagnosticsLog, message_content) {
  Log::info("unique message string"_view);
  EXPECT(contains(last_entry(), "unique message string"_view));
}

PERIMORTEM_UNIT_TEST(DiagnosticsLog, suppress_messages) {
  Log::set_level(Log::Level::Error);
  Count events_before = total_events;

  Log::error("should pass through"_view);
  EXPECT(total_events > events_before);

  Log::info("should be suppressed"_view);
  EXPECT_EQ(total_events, events_before);

  // Surpressed message shouldn't override the older message.
  EXPECT(contains(last_entry(), "should pass through"_view));
  EXPECT(has_valid_header(last_entry()));
}

auto logging_function() -> void {
  Log::error("Test Attribution"_view);
}

auto attributing_function() -> void {
  auto scope_attribution = Diagnostics::Log::set_attribution();
  logging_function();
}

PERIMORTEM_UNIT_TEST(DiagnosticsLog, attribution) {
  attributing_function();

  auto attributed_message =
      "[main] validation/unit_tests/perimortem/core/log.cpp:169:28: Test Attribution"_view;
  auto logged = last_entry();
  EXPECT_TEXT(
      logged.slice(15, attributed_message.get_size()), attributed_message);
  EXPECT(has_valid_header(last_entry()));
}
