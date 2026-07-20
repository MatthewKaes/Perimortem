// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "ttx/attribute.hpp"
#include "ttx/documentation.hpp"
#include "ttx/lexical/code.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa::Base {

// Declaration decodes the shared declaration prefix used by body ISAs:
//
//   modifier Name : kind
//
// The allowed token Codes come from the ISA that calls it. The qualifier is a
// source name, not a closed enum. Package, Library, and later ISAs decide
// whether that name maps to an alias, struct, package, function, or another
// sub-ISA.
class Declaration {
 public:
  Declaration() = default;
  Declaration(
      Ttx::Documentation documentation,
      Ttx::Lexical::Code::Type modifier,
      Ttx::Lexical::Code::Type name_class,
      Perimortem::Core::View::Bytes name,
      Ttx::Lexical::Code::Type kind_class,
      Perimortem::Core::View::Bytes kind,
      Perimortem::Core::View::Vector<Ttx::Attribute> attributes =
          Perimortem::Core::View::Vector<Ttx::Attribute>())
      : documentation(documentation),
        modifier(modifier),
        name_class(name_class),
        name(name),
        kind_class(kind_class),
        kind(kind),
        attributes(attributes) {}

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Ttx::Documentation documentation,
      Perimortem::Core::View::Vector<Ttx::Lexical::Code::Type>
          allowed_modifiers,
      Perimortem::Core::View::Vector<Ttx::Lexical::Code::Type> allowed_names,
      Perimortem::Core::View::Vector<Ttx::Lexical::Code::Type>
          allowed_qualifiers) -> Declaration;
  static auto evaluate_after_modifier(
      Ttx::Lexical::Cursor& cursor,
      Ttx::Documentation documentation,
      Ttx::Lexical::Code::Type modifier,
      Perimortem::Core::View::Vector<Ttx::Lexical::Code::Type> allowed_names,
      Perimortem::Core::View::Vector<Ttx::Lexical::Code::Type>
          allowed_qualifiers,
      Perimortem::Core::View::Vector<Ttx::Attribute> attributes = {})
      -> Declaration;

  constexpr auto get_documentation() const -> Ttx::Documentation {
    return documentation;
  }

  constexpr auto get_modifier() const -> Ttx::Lexical::Code::Type {
    return modifier;
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }

  constexpr auto get_kind_class() const -> Ttx::Lexical::Code::Type {
    return kind_class;
  }

  constexpr auto get_kind() const -> Perimortem::Core::View::Bytes {
    return kind;
  }

  constexpr auto get_attributes() const
      -> Perimortem::Core::View::Vector<Ttx::Attribute> {
    return attributes;
  }

  constexpr auto has_addressable_name() const -> Bool {
    return name_class == Ttx::Lexical::Code::Type::Addressable;
  }

  constexpr auto is_empty() const -> Bool {
    return modifier == Ttx::Lexical::Code::Type::Unknown;
  }

 private:
  Ttx::Documentation documentation;
  Ttx::Lexical::Code::Type modifier = Ttx::Lexical::Code::Type::Unknown;
  Ttx::Lexical::Code::Type name_class = Ttx::Lexical::Code::Type::Unknown;
  Perimortem::Core::View::Bytes name;
  Ttx::Lexical::Code::Type kind_class = Ttx::Lexical::Code::Type::Unknown;
  Perimortem::Core::View::Bytes kind;
  Perimortem::Core::View::Vector<Ttx::Attribute> attributes;
};

}  // namespace Tetrodotoxin::Isa::Base
