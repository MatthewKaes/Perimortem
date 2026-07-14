// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/system/uuid.hpp"

#include "tetrodotoxin/archiver/dependency.hpp"

namespace Tetrodotoxin::Archiver {

class Manifest {
 public:
  Manifest(
      Perimortem::Core::View::Bytes name,
      Perimortem::System::Uuid version,
      Perimortem::Core::View::Vector<Dependency> imports)
      : name(name), version(version), imports(imports) {}

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }

  constexpr auto get_version() const -> Perimortem::System::Uuid {
    return version;
  }

  constexpr auto get_imports() const
      -> Perimortem::Core::View::Vector<Dependency> {
    return imports;
  }

 private:
  Perimortem::Core::View::Bytes name;
  Perimortem::System::Uuid version;
  Perimortem::Core::View::Vector<Dependency> imports;
};

}  // namespace Tetrodotoxin::Archiver
