// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/structure.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/attribute.hpp"
#include "tetrodotoxin/isa/documentation.hpp"
#include "tetrodotoxin/isa/library/addressable.hpp"
#include "tetrodotoxin/isa/library/function.hpp"
#include "tetrodotoxin/isa/library/syntax.hpp"
#include "tetrodotoxin/isa/modifier.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

// The struct evaluation of the bytecode is less than ideal due to functions
// having a unique syntax.
auto Library::Structure::evaluate(
    Cursor& cursor,
    Library::Scope& scope,
    const Tetrodotoxin::Isa::Definition& definition) -> const Ttx::Type* {
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after library aggregate declaration."_view)) {
    return nullptr;
  }

  Managed::Vector<Ttx::Type::Member> members(scope.get_context().get_arena());
  Managed::Vector<Ttx::Type::Function> functions(
      scope.get_context().get_arena());
  Ttx::Type& type = scope.get_context().get_arena().allocate<Ttx::Type>();
  if (!scope.stage_type_reference(definition.get_name(), type)) {
    cursor.token_error(
        "Library aggregate type could not stage self reference."_view);
    return nullptr;
  }

  Bool valid = True;
  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    Ttx::Documentation member_documentation =
        Documentation::evaluate(cursor);
    if (!Attribute::consume_all(cursor)) {
      return nullptr;
    }

    Modifier modifier = Modifier::evaluate(
        cursor,
        {{Class::Type::Public, Class::Type::Private, Class::Type::Expose,
          Class::Type::State, Class::Type::Const}},
        "Expected a definition to start with one of the following modifiers "
        "{public, private, expose, state, const}"_view);
    if (!modifier.is_valid()) {
      valid = False;
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return nullptr;
      }
      continue;
    }

    if (cursor.matches(Class::Type::Func)) {
      Ttx::Type::Function function =
          Library::Function::evaluate(cursor, scope, member_documentation);
      if (function.is_empty()) {
        valid = False;
        if (!Library::Syntax::consume_declaration_tail(cursor)) {
          return nullptr;
        }
        continue;
      }

      for (Count i = 0; i < functions.get_size(); i++) {
        if (functions[i].get_name() == function.get_name()) {
          cursor.token_error("Library function name is already defined."_view);
          valid = False;
          break;
        }
      }
      if (!valid) {
        continue;
      }

      functions.insert(function);
      continue;
    }

    Tetrodotoxin::Isa::Definition member_definition =
        Tetrodotoxin::Isa::Definition::evaluate_after_modifier(
            cursor, member_documentation, modifier.get_type(),
            {{Class::Type::Addressable}}, {{Class::Type::Type}});
    if (!member_definition.is_valid()) {
      valid = False;
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return nullptr;
      }
      continue;
    }

    Ttx::Type::Member member =
        Library::Addressable::evaluate(cursor, scope, member_definition);
    if (member.is_empty()) {
      valid = False;
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return nullptr;
      }
      continue;
    }

    for (Count i = 0; i < members.get_size(); i++) {
      if (members[i].get_name() == member.get_name()) {
        cursor.token_error("Library member name is already defined."_view);
        valid = False;
        break;
      }
    }
    if (!valid) {
      continue;
    }

    members.insert(member);
  }

  if (!cursor.require(
          Class::Type::ScopeEnd,
          "Expected `}` after library struct declaration."_view)) {
    return nullptr;
  }

  if (!valid) {
    return nullptr;
  }

  Managed::Vector<Ttx::Attribute> attributes(scope.get_context().get_arena());
  attributes.insert({"isa"_view, "Struct"_view});
  new (&type) Ttx::Type(
      definition.get_name(), members.get_view(),
      View::Vector<const Ttx::Type*>(), functions.get_view(),
      definition.get_documentation(), attributes.get_view());
  return &type;
}
