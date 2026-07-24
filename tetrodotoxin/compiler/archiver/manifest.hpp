// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/archiver/dependency.hpp"

namespace Tetrodotoxin::Archiver {

class Manifest {
 public:
  Manifest(
      Perimortem::Core::View::Bytes name,
      Perimortem::System::Version version,
      Perimortem::Core::View::Vector<Dependency> dependencies)
      : name(name), version(version), dependencies(dependencies) {}

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }

  constexpr auto get_version() const -> Perimortem::System::Version {
    return version;
  }

  constexpr auto get_dependencies() const
      -> Perimortem::Core::View::Vector<Dependency> {
    return dependencies;
  }

  auto is_valid() const -> Bool;
  auto is_valid(Perimortem::Memory::Allocator::Arena& arena) const -> Bool;
  static auto is_valid_name(Perimortem::Core::View::Bytes name) -> Bool;

 private:
  Perimortem::Core::View::Bytes name;
  Perimortem::System::Version version;
  Perimortem::Core::View::Vector<Dependency> dependencies;
};

}  // namespace Tetrodotoxin::Archiver
