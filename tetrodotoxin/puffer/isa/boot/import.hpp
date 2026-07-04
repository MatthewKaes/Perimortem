// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/registry.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Puffer::Isa::Boot {

// Import is the unresolved dependency request written in the source preamble.
//
// Resolution owns loading and binding because only the source graph knows the
// source tree and the already loaded package graph. Boot preserves just the
// authored request: local alias, expected ISA, and source selector.
class Import {
 public:
  Import() = default;

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      const Tetrodotoxin::Isa::Registry& registry) -> Import;

  constexpr auto get_local_name() const -> Perimortem::Core::View::Bytes {
    return local_name;
  }
  constexpr auto get_isa() const -> Perimortem::Core::View::Bytes {
    return isa;
  }
  constexpr auto get_source_name() const -> Perimortem::Core::View::Bytes {
    return source_name;
  }

  constexpr auto is_package() const -> Bool { return package; }
  constexpr auto is_valid() const -> Bool {
    return !local_name.is_empty() && !source_name.is_empty() &&
           !isa.is_empty();
  }

 private:
  Import(
      Perimortem::Core::View::Bytes local_name,
      Perimortem::Core::View::Bytes source_name,
      Perimortem::Core::View::Bytes isa,
      Bool package)
      : local_name(local_name),
        source_name(source_name),
        isa(isa),
        package(package) {}

  Perimortem::Core::View::Bytes local_name;
  Perimortem::Core::View::Bytes source_name;
  Perimortem::Core::View::Bytes isa;
  Bool package = False;
};

}  // namespace Tetrodotoxin::Puffer::Isa::Boot
