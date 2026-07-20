// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/model/namespace.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Model::Dialects {

// Group is the Dialect for package namespace syntax. Its body hands each nested
// definition back to the same Definitions continuation, then returns a
// complete Tetrodotoxin Namespace. Definitions owns publication into the
// enclosing Namespace. Group is grammar, not an Abstract contract.
class Group final {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "group"_view;

  template <typename definitions_type>
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes name,
      const Ttx::Concept::Documentation& documentation,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token>,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Concept::Abstract>> visible)
      -> const Ttx::Concept::Abstract& {
    using namespace Perimortem::Core;
    using namespace Ttx::Concept;
    using namespace Ttx::Lexical;

    cursor.consume();
    const Token scope_start = cursor.require(
        Code::Type::ScopeStart, "Expected `{` after Group definition."_view);
    if (!scope_start.is_valid()) {
      return Invalid::get_invalid();
    }

    Tetrodotoxin::Model::Namespace& body =
        cursor.get_arena().construct<Tetrodotoxin::Model::Namespace>(
            cursor.get_arena(), name, documentation);

    while (!cursor.matches(Code::Type::Terminal) &&
           !cursor.matches(Code::Type::ScopeEnd)) {
      const Abstract& child = definitions_type::evaluate(cursor, body, visible);
      if (child.is<Invalid>()) {
        return Invalid::get_invalid();
      }
    }

    const Token scope_end = cursor.require(
        Code::Type::ScopeEnd, "Expected `}` after Group definition."_view);
    if (!scope_end.is_valid()) {
      return Invalid::get_invalid();
    }

    return body;
  }
};

}  // namespace Tetrodotoxin::Model::Dialects
