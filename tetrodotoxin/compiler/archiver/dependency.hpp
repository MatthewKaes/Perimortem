// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/hash.hpp"

#include "perimortem/system/version.hpp"

namespace Tetrodotoxin::Archiver {

// Dependency is one exact authored package identity in Manifest order. It is
// physical repository data rather than a semantic Dependency edge: local
// source bindings and root Dialects do not survive package compilation.
class Dependency {
 public:
  constexpr Dependency(
      Perimortem::Core::View::Bytes name,
      Perimortem::System::Version version)
      : name(name), version(version) {}

  constexpr auto operator==(const Dependency& rhs) const -> Bool {
    return version == rhs.version && name == rhs.name;
  }

  constexpr auto operator!=(const Dependency& rhs) const -> Bool {
    return !(*this == rhs);
  }

  constexpr auto hash() const -> Unsigned_64 {
    Unsigned_64 encoded =
        (Unsigned_64(version.get_major()) << 16) | version.get_minor();
    return Perimortem::Core::Hash(name).Rehash(encoded);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }

  constexpr auto get_version() const -> Perimortem::System::Version {
    return version;
  }

 private:
  Perimortem::Core::View::Bytes name;
  Perimortem::System::Version version;
};

}  // namespace Tetrodotoxin::Archiver
