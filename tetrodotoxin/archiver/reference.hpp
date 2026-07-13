// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/archiver/package.hpp"

namespace Tetrodotoxin::Archiver {

// Reference is one resolved package visible to an archive transaction.
//
// The package already owns its manifest identity and restored type table, so a
// reference does not repeat either fact. Archive writers assign compact local
// indexes to these packages; readers match those indexes back to packages by
// manifest name and version rather than borrowing caller vector positions.
class Reference {
 public:
  Reference() = default;
  explicit constexpr Reference(const Package& package) : package(&package) {}

  constexpr auto get_package() const -> const Package& { return *package; }
  constexpr auto get_types() const
      -> Perimortem::Core::View::Vector<const Ttx::Type*> {
    return package->get_types();
  }

  constexpr auto get_source_name() const -> Perimortem::Core::View::Bytes {
    return package->get_manifest().get_name();
  }

  constexpr auto get_version() const -> Version {
    return package->get_manifest().get_version();
  }

  constexpr auto is_valid() const -> Bool {
    return package != nullptr && package->is_valid() &&
           !package->get_types().is_empty();
  }

 private:
  const Package* package = nullptr;
};

}  // namespace Tetrodotoxin::Archiver
