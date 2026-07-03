// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "ttx/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa {

// Definition evaluates the reusable package declaration spine:
//
//   @public Name : alias
//   @public Name : Package
//   @public Name : Namespace
//
// The prefix is common enough that it should be one composable ISA piece
// instead of each package construct re-reading sigil, name, define, and kind.
// It is still syntax, not a semantic type. Resolving the definition to real
// Ttx::Type objects happens after resolution has the imported sources in hand.
class Definition {
 public:
  enum class Kind : Bits_8 {
    Alias,
    Namespace,
    Package,
  };

  Definition() = default;
  Definition(
      Ttx::Documentation documentation,
      Perimortem::Core::View::Bytes name,
      Kind kind)
      : documentation(documentation), name(name), kind(kind), valid(True) {}

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Ttx::Documentation documentation,
      Perimortem::Core::View::Vector<Kind> allowed_kinds) -> Definition;

  constexpr auto get_documentation() const -> Ttx::Documentation {
    return documentation;
  }
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }
  constexpr auto get_kind() const -> Kind { return kind; }
  constexpr auto is_alias() const -> Bool { return kind == Kind::Alias; }
  constexpr auto is_namespace() const -> Bool {
    return kind == Kind::Namespace;
  }
  constexpr auto is_package() const -> Bool { return kind == Kind::Package; }
  constexpr auto is_valid() const -> Bool { return valid; }

 private:
  Ttx::Documentation documentation;
  Perimortem::Core::View::Bytes name;
  Kind kind = Kind::Alias;
  Bool valid = False;
};

}  // namespace Tetrodotoxin::Isa
