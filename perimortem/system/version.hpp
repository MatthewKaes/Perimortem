// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

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
