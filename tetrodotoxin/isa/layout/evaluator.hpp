// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/attribute.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Layout {

// Evaluator parses the layout micro-language shared by function signatures,
// return carriers, type parameters, and later loop bindings.
class Evaluator {
 public:
  template <typename Scope>
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Member>& members)
      -> Bool {
    if (cursor.matches(Ttx::Lexical::Class::Type::IndexStart)) {
      return evaluate_bracketed(cursor, scope, members);
    }

    return evaluate_type_member(
        cursor, scope, Perimortem::Core::View::Bytes(), members);
  }

  template <typename Scope>
  static auto evaluate_bracketed(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Member>& members)
      -> Bool {
    using namespace Ttx::Lexical;

    if (!cursor.require(
            Class::Type::IndexStart, "Expected `[` before layout."_view)) {
      return False;
    }

    while (!cursor.matches(Class::Type::EndOfStream) &&
           !cursor.matches(Class::Type::IndexEnd)) {
      if (!Tetrodotoxin::Isa::Attribute::consume_all(cursor)) {
        return False;
      }

      Perimortem::Core::View::Bytes name;
      if (cursor.matches(Class::Type::AddressOp)) {
        cursor.consume();
        const Token* name_token = cursor.require(
            Class::Type::Addressable,
            "Expected member name after `.` in layout."_view);
        if (name_token == nullptr) {
          return False;
        }

        name = name_token->get_text();
        if (!cursor.require(
                Class::Type::Define,
                "Expected `:` after layout member name."_view)) {
          return False;
        }
      }

      if (!evaluate_type_member(cursor, scope, name, members)) {
        return False;
      }

      if (cursor.matches(Class::Type::PackingOp)) {
        cursor.consume();
        continue;
      }

      if (!cursor.matches(Class::Type::IndexEnd)) {
        cursor.token_error("Expected `,` or `]` after layout member."_view);
        return False;
      }
    }

    return cursor.require(
               Class::Type::IndexEnd, "Expected `]` after layout."_view) !=
           nullptr;
  }

 private:
  template <typename Scope>
  static auto evaluate_type_member(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      Perimortem::Core::View::Bytes name,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Member>& members)
      -> Bool {
    const Count error_count = cursor.get_errors().get_size();
    const Ttx::Type* type = scope.resolve_type(cursor);
    if (cursor.get_errors().get_size() != error_count) {
      return False;
    }

    if (type == nullptr) {
      cursor.token_error("Layout type could not be resolved."_view);
      return False;
    }

    members.insert(Ttx::Type::Member(name, *type));
    return True;
  }
};

}  // namespace Tetrodotoxin::Isa::Layout
