// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/model/package/source.hpp"
#include "ttx/concept/documentation.hpp"

namespace Tetrodotoxin::Model::Enviornment {

// Package is a dependency found by durable package name and exact Major.Minor
// Version. Its binding names the same anonymous Model::Package contract whether
// the resolver supplied an interpreted package or restored a precompiled one.
// Environment owns this edge once and exposes its Alias to every Source in
// the transaction.
class Dependency {
 public:
  constexpr Dependency(
      Perimortem::Core::View::Bytes root_dialect,
      Perimortem::Core::View::Bytes package_name,
      Perimortem::System::Version version,
      Perimortem::Core::View::Bytes local_name,
      const Package::Source& package,
      const Ttx::Concept::Documentation& documentation)
      : Dependency(root_dialect, local_name, package, documentation),
        package_name(package_name),
        version(version),
        package(package) {}

  constexpr auto get_dialect() const -> Perimortem::Core::View::Bytes {
    return package;
  }

  constexpr auto get_package_name() const -> Perimortem::Core::View::Bytes {
    return package_name;
  }

  constexpr auto get_version() const -> Perimortem::System::Version {
    return version;
  }

  constexpr auto get_package() const -> const Package::Source& {
    return package;
  }

 private:
  Perimortem::Core::View::Bytes package_name;
  Perimortem::System::Version version;
  const Package::Source& source;
};

}  // namespace Tetrodotoxin::Model::Enviornment
