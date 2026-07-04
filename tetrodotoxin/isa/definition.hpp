// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "ttx/documentation.hpp"
#include "ttx/lexical/class.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa {

// Definition decodes the shared declaration prefix used by body ISAs:
//
//   modifier Name : kind
//
// The allowed token classes come from the ISA that calls it. The qualifier is a
// source name, not a closed enum. Package, Library, and later ISAs decide
// whether that name maps to an alias, struct, package, function, or another
// sub-ISA.
class Definition {
 public:
  Definition() = default;
  Definition(
      Ttx::Documentation documentation,
      Ttx::Lexical::Class::Type modifier,
      Ttx::Lexical::Class::Type name_class,
      Perimortem::Core::View::Bytes name,
      Ttx::Lexical::Class::Type kind_class,
      Perimortem::Core::View::Bytes kind)
      : documentation(documentation),
        modifier(modifier),
        name_class(name_class),
        name(name),
        kind_class(kind_class),
        kind(kind),
        valid(True) {}

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Ttx::Documentation documentation,
      Perimortem::Core::View::Vector<Ttx::Lexical::Class::Type>
          allowed_modifiers,
      Perimortem::Core::View::Vector<Ttx::Lexical::Class::Type> allowed_names,
      Perimortem::Core::View::Vector<Ttx::Lexical::Class::Type>
          allowed_qualifiers) -> Definition;
  static auto evaluate_after_modifier(
      Ttx::Lexical::Cursor& cursor,
      Ttx::Documentation documentation,
      Ttx::Lexical::Class::Type modifier,
      Perimortem::Core::View::Vector<Ttx::Lexical::Class::Type> allowed_names,
      Perimortem::Core::View::Vector<Ttx::Lexical::Class::Type>
          allowed_qualifiers) -> Definition;

  constexpr auto get_documentation() const -> Ttx::Documentation {
    return documentation;
  }
  constexpr auto get_modifier() const -> Ttx::Lexical::Class::Type {
    return modifier;
  }
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }
  constexpr auto get_kind_class() const -> Ttx::Lexical::Class::Type {
    return kind_class;
  }
  constexpr auto get_kind() const -> Perimortem::Core::View::Bytes {
    return kind;
  }
  constexpr auto has_addressable_name() const -> Bool {
    return name_class == Ttx::Lexical::Class::Type::Addressable;
  }
  constexpr auto is_valid() const -> Bool { return valid; }

 private:
  Ttx::Documentation documentation;
  Ttx::Lexical::Class::Type modifier = Ttx::Lexical::Class::Type::Unknown;
  Ttx::Lexical::Class::Type name_class = Ttx::Lexical::Class::Type::Unknown;
  Perimortem::Core::View::Bytes name;
  Ttx::Lexical::Class::Type kind_class = Ttx::Lexical::Class::Type::Unknown;
  Perimortem::Core::View::Bytes kind;
  Bool valid = False;
};

}  // namespace Tetrodotoxin::Isa
