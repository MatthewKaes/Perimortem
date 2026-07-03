// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/lexical/cursor.hpp"

namespace Ttx::Dialect::Source {

// The parse concept for a TTX dialect decleration.
class Dialect {
 public:
  Dialect() = default;
  Dialect(Perimortem::Core::View::Bytes name) : name(name) {}

  static auto parse(Lexical::Cursor& cursor) -> Dialect {
    // Parse the dialect and required define syntax to get to the actual dialect
    // name which is stored as a single token in TTX's Type format (PascalCase).
    if (!cursor.require(
            Lexical::Class::Type::Define,
            "Expected `:` after import name."_view)) {
      return Dialect();
    }

    auto name = cursor.require(
        Lexical::Class::Type::Type, "expected a TTX::Dialect name"_view);
    if (!name) {
      return Dialect();
    }

    return Dialect(name->get_text());
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }

  constexpr auto is_valid() const -> Bool { return name.is_empty(); }

  constexpr auto operator==(const Dialect& rhs) const -> Bool {
    return name == rhs.name;
  }

 private:
  Perimortem::Core::View::Bytes name;
};

}  // namespace Ttx::Dialect::Source
