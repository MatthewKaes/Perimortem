// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/attribute.hpp"
#include "tetrodotoxin/isa/base/expression/type.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Base::Layout {

// Evaluator parses the layout micro-language shared by function signatures,
// return carriers, type parameters, and later loop bindings.
class Evaluator {
 public:
  template <typename Scope>
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      Perimortem::Memory::Managed::Vector<Ttx::Member>& members) -> Bool {
    if (cursor.matches(Ttx::Lexical::Class::Type::IndexStart)) {
      return evaluate_bracketed(cursor, scope, members);
    }

    return evaluate_type_member(
        cursor, scope, Perimortem::Core::View::Bytes(), {}, members);
  }

  template <typename Scope>
  static auto evaluate_bracketed(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      Perimortem::Memory::Managed::Vector<Ttx::Member>& members) -> Bool {
    if (!cursor.require(
            Ttx::Lexical::Class::Type::IndexStart,
            "Expected `[` before layout."_view)) {
      return False;
    }

    while (!cursor.matches(Ttx::Lexical::Class::Type::EndOfStream) &&
           !cursor.matches(Ttx::Lexical::Class::Type::IndexEnd)) {
      Perimortem::Memory::Managed::Vector<Ttx::Attribute> attributes(
          get_context(scope).get_arena());
      if (!Tetrodotoxin::Isa::Base::Attribute::evaluate_all(
              cursor, attributes)) {
        return False;
      }

      Perimortem::Core::View::Bytes name;
      if (cursor.matches(Ttx::Lexical::Class::Type::AddressOp)) {
        cursor.consume();
        const Ttx::Lexical::Token* name_token = cursor.require(
            Ttx::Lexical::Class::Type::Addressable,
            "Expected member name after `.` in layout."_view);
        if (name_token == nullptr) {
          return False;
        }

        name = name_token->get_text();
        if (!cursor.require(
                Ttx::Lexical::Class::Type::Define,
                "Expected `:` after layout member name."_view)) {
          return False;
        }
      }

      if (!evaluate_type_member(
              cursor, scope, name, attributes.get_view(), members)) {
        return False;
      }

      if (cursor.matches(Ttx::Lexical::Class::Type::PackingOp)) {
        cursor.consume();
        continue;
      }

      if (!cursor.matches(Ttx::Lexical::Class::Type::IndexEnd)) {
        cursor.token_error("Expected `,` or `]` after layout member."_view);
        return False;
      }
    }

    return cursor.require(
               Ttx::Lexical::Class::Type::IndexEnd,
               "Expected `]` after layout."_view) != nullptr;
  }

 private:
  static constexpr auto get_context(Tetrodotoxin::Isa::Base::Context& context)
      -> Tetrodotoxin::Isa::Base::Context& {
    return context;
  }

  template <typename Scope>
  static constexpr auto get_context(Scope& scope)
      -> Tetrodotoxin::Isa::Base::Context& {
    return scope.get_context();
  }

  static auto evaluate_type(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context) -> const Ttx::Type* {
    return Tetrodotoxin::Isa::Base::Expression::Type::evaluate(cursor, context);
  }

  template <typename Scope>
  static auto evaluate_type(Ttx::Lexical::Cursor& cursor, Scope& scope)
      -> const Ttx::Type* {
    return scope.resolve_type(cursor);
  }

  template <typename Scope>
  static auto evaluate_type_member(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<Ttx::Attribute> attributes,
      Perimortem::Memory::Managed::Vector<Ttx::Member>& members) -> Bool {
    const Count error_count = cursor.get_errors().get_size();
    const Ttx::Type* type = evaluate_type(cursor, scope);
    if (cursor.get_errors().get_size() != error_count) {
      return False;
    }

    if (type == nullptr) {
      cursor.token_error("Layout type could not be resolved."_view);
      return False;
    }

    members.insert(
        Ttx::Member(name, *type, False, Ttx::Documentation(), attributes));
    return True;
  }
};

}  // namespace Tetrodotoxin::Isa::Base::Layout
