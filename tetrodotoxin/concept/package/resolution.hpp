// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/system/version.hpp"

namespace Tetrodotoxin::Concept::Package {

// A Resolution is the exact external Package request written in package.ttx.
// Parser creates the request, Puffer uses it to select a Package, and
// Environment turns the completed request into a dependency edge. It retains
// no repository result so all three owners agree on one authored identity.
class Resolution {
 public:
  constexpr Resolution(
      Perimortem::Core::View::Bytes local_name,
      Perimortem::Core::View::Bytes package_name,
      Perimortem::System::Version version)
      : local_name(local_name), package_name(package_name), version(version) {}

  constexpr auto get_local_name() const -> Perimortem::Core::View::Bytes {
    return local_name;
  }

  constexpr auto get_package_name() const -> Perimortem::Core::View::Bytes {
    return package_name;
  }

  constexpr auto get_version() const -> Perimortem::System::Version {
    return version;
  }

 private:
  Perimortem::Core::View::Bytes local_name;
  Perimortem::Core::View::Bytes package_name;
  Perimortem::System::Version version;
};

}  // namespace Tetrodotoxin::Concept::Package
