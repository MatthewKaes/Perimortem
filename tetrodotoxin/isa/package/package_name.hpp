// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa::Package {

// PackageName owns package identity values such as `TTX::Graphics`.
//
// Package directives use it through `@package_name = TTX::Name;`, and Puffer's
// Boot ISA uses the same value parser for package imports. This is package
// metadata, not a type query. The full qualified spelling is preserved so
// resolution can load and cache packages by their public package name instead
// of by a private source path or by the root type's short name.
class PackageName {
 public:
  PackageName() = default;
  PackageName(
      Perimortem::Core::View::Bytes name,
      Ttx::Documentation documentation)
      : name(name), documentation(documentation), valid(True) {}

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Ttx::Documentation documentation = Ttx::Documentation())
      -> PackageName;

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }
  constexpr auto get_documentation() const -> Ttx::Documentation {
    return documentation;
  }
  constexpr auto is_valid() const -> Bool { return valid; }

 private:
  Perimortem::Core::View::Bytes name;
  Ttx::Documentation documentation;
  Bool valid = False;
};

}  // namespace Tetrodotoxin::Isa::Package
