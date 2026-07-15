// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa::Package {

// PackageName owns dependency selector values such as `Perimortem.Graphics`.
//
// Puffer Boot uses this parser for package imports. Package identity itself is
// compiler configuration because Bazel requires package outputs to be declared
// before source evaluation runs.
class PackageName {
 public:
  PackageName() = default;
  PackageName(
      Perimortem::Core::View::Bytes name,
      Ttx::Documentation documentation)
      : name(name), documentation(documentation) {}

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Ttx::Documentation documentation = Ttx::Documentation()) -> PackageName;

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }

  constexpr auto get_documentation() const -> Ttx::Documentation {
    return documentation;
  }

  constexpr auto is_empty() const -> Bool { return name.is_empty(); }

 private:
  Perimortem::Core::View::Bytes name;
  Ttx::Documentation documentation;
};

}  // namespace Tetrodotoxin::Isa::Package
