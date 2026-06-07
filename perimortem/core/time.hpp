// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/math.hpp"
#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Core {

// Time captures a relative timestamp that can be used to compare time points.
class Time {
 public:
  // Duration captures the explicit distance between any two time points.
  // Allows for centralized conversion into actual time units for measurement.
  class Duration {
   public:
    Duration(Bits_64 delta) : nanosecond_delta(delta) {};

    // Converts a delta time into nanoseconds.
    constexpr auto convert_to_nanoseconds() -> Bits_64 {
      return nanosecond_delta;
    }

    // Converts a delta time into microseconds.
    constexpr auto convert_to_microseconds() -> Real_64 {
      return nanosecond_delta / Real_64(1'000.0);
    }

    // Converts a delta time into milliseconds.
    constexpr auto convert_to_milliseconds() -> Real_64 {
      return nanosecond_delta / Real_64(1'000'000.0);
    }

    // Converts a delta time into seconds.
    // Drops the micro & nano second portions.
    constexpr auto convert_to_seconds() -> Real_64 {
      return (nanosecond_delta / 1'000'000.0) / Real_64(1'000.0);
    }

   private:
    Bits_64 nanosecond_delta;
  };

  Time() : timestamp(0) {};
  Time(Bits_64 stamp) : timestamp(stamp) {};
  Time(Bits_64 seconds, Bits_64 nanoseconds)
      : timestamp(seconds * 1'000'000'000 + nanoseconds) {};

  // Returns a time object capturing the delta time from UNIX epoch.
  static auto now() -> Time;

  // Returns the time the application booted.
  static auto boot() -> Time;

  // Checks if two deltas are equal to each other, but doesn't guarantee they
  // are in the same reference frame.
  constexpr auto operator==(const Time& rhs) -> Bool {
    return get_stamp() == rhs.get_stamp();
  };

  // Gets the raw 64 bit time stamp in nanoseconds.
  constexpr auto get_stamp() const -> Bits_64 { return timestamp; }

  // Calculation the time between two time points.
  // If none is provided then `Time::now()` is used as the end point.
  constexpr auto measure(const Time& end_point = Time::now()) const
      -> Duration {
    // Ensure duration is always the absolute value.
    auto start_stamp = Math::min(get_stamp(), end_point.get_stamp());
    auto end_stamp = Math::max(get_stamp(), end_point.get_stamp());

    return Duration(end_stamp - start_stamp);
  }

  // Returns the wall clock time as a formatted buffer.
  auto calculate_clock() const -> Static::Bytes<12>;

 private:
  Bits_64 timestamp;
};

}  // namespace Perimortem::Core
