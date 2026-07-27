// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/perimortem.hpp"

namespace Perimortem::System {

// A compact `Major.Minor` version representation optimized for fast comparison
// and storage.
//
// Zero is valid in either component but 0.0 is reserved as the null value for
// an unset version.
class Version {
 public:
  constexpr Version() = default;

  constexpr Version(Unsigned_16 major, Unsigned_16 minor)
      : major(major), minor(minor) {}

  // Parses the canonical textual representation without ever constructing a
  // floating-point value. Leading zeroes are rejected so accepted text has one
  // stable round trip. Invalid text and the reserved `0.0` value return the
  // null Version.
  static constexpr auto parse(Perimortem::Core::View::Bytes text) -> Version {
    Count separator = Count(-1);
    if (text.get_size() < 3) {
      return {};
    }

    for (Count i = 0; i < text.get_size(); i++) {
      Unsigned_8 byte = text[i];
      if (byte == '.') {
        if (separator != Count(-1)) {
          return {};
        }

        separator = i;
        continue;
      }

      if (byte < '0' || byte > '9') {
        return {};
      }
    }

    if (separator == Count(-1) || separator == 0 ||
        separator + 1 == text.get_size()) {
      return {};
    }
    if ((separator > 1 && text[0] == '0') ||
        (text.get_size() - separator > 2 && text[separator + 1] == '0')) {
      return {};
    }

    Unsigned_32 parsed_major = 0;
    for (Count i = 0; i < separator; i++) {
      Unsigned_32 digit = Unsigned_32(text[i] - '0');
      if (parsed_major > (Unsigned_16(-1) - digit) / 10) {
        return {};
      }
      parsed_major = parsed_major * 10 + digit;
    }

    Unsigned_32 parsed_minor = 0;
    for (Count i = separator + 1; i < text.get_size(); i++) {
      Unsigned_32 digit = Unsigned_32(text[i] - '0');
      if (parsed_minor > (Unsigned_16(-1) - digit) / 10) {
        return {};
      }
      parsed_minor = parsed_minor * 10 + digit;
    }

    return Version(Unsigned_16(parsed_major), Unsigned_16(parsed_minor));
  }

  constexpr auto operator==(const Version& rhs) const -> Bool {
    return major == rhs.major && minor == rhs.minor;
  }

  constexpr auto operator!=(const Version& rhs) const -> Bool {
    return !(*this == rhs);
  }

  constexpr auto operator<(const Version& rhs) const -> Bool {
    if (major == rhs.major) {
      return minor < rhs.minor;
    }

    return major < rhs.major;
  }

  constexpr auto operator>(const Version& rhs) const -> Bool {
    return rhs < *this;
  }

  constexpr auto get_major() const -> Unsigned_16 { return major; }

  constexpr auto get_minor() const -> Unsigned_16 { return minor; }

  constexpr auto is_null() const -> Bool { return major == 0 && minor == 0; }

 private:
  Unsigned_16 major = 0;
  Unsigned_16 minor = 0;
};

static_assert(sizeof(Version) == 4);

}  // namespace Perimortem::System
