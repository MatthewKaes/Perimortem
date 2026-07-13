// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/archiver/dependency.hpp"
#include "tetrodotoxin/archiver/version.hpp"

namespace Tetrodotoxin::Archiver {

class Manifest {
 public:
  Manifest() = default;
  Manifest(
      Perimortem::Core::View::Bytes name,
      Version version,
      Perimortem::Core::View::Vector<Dependency> imports)
      : name(name), version(version), imports(imports) {}

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }

  constexpr auto get_version() const -> Version { return version; }

  constexpr auto get_imports() const
      -> Perimortem::Core::View::Vector<Dependency> {
    return imports;
  }

  constexpr auto is_valid() const -> Bool {
    return version.is_set() && !name.is_empty();
  }

 private:
  Perimortem::Core::View::Bytes name;
  Version version;
  Perimortem::Core::View::Vector<Dependency> imports;
};

}  // namespace Tetrodotoxin::Archiver
