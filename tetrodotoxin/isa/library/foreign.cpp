// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/foreign.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/attribute.hpp"
#include "tetrodotoxin/isa/boot/documentation.hpp"
#include "tetrodotoxin/isa/library/function.hpp"
#include "tetrodotoxin/isa/library/syntax.hpp"
#include "tetrodotoxin/isa/modifier.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Library::Foreign::evaluate(
    Library::Scope& scope,
    Cursor& cursor,
    const Tetrodotoxin::Isa::Definition& definition) -> const Ttx::Type* {
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after library foreign declaration."_view)) {
    return nullptr;
  }

  Managed::Vector<Ttx::Type::Function> functions(
      scope.get_context().get_arena());
  Bool valid = True;
  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    Ttx::Documentation documentation = Boot::Documentation::evaluate(cursor);
    if (!Attribute::consume_all(cursor)) {
      return nullptr;
    }

    Modifier modifier = Modifier::evaluate(
        cursor, {{Class::Type::Expose}},
        "Expected a foreign function declaration to start with expose."_view);
    if (!modifier.is_valid()) {
      valid = False;
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return nullptr;
      }
      continue;
    }

    Ttx::Type::Function function =
        Library::Function::evaluate_declaration(scope, cursor, documentation);
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
  }

  if (!cursor.require(
          Class::Type::ScopeEnd,
          "Expected `}` after library foreign declaration."_view)) {
    return nullptr;
  }

  if (!valid) {
    return nullptr;
  }

  Managed::Vector<Ttx::Attribute> attributes(scope.get_context().get_arena());
  attributes.insert({"isa"_view, "Foreign"_view});
  auto& type = scope.get_context().get_arena().construct<Ttx::Type>(
      definition.get_name(), View::Vector<Ttx::Type::Member>(),
      View::Vector<const Ttx::Type*>(), functions.get_view(),
      definition.get_documentation(), attributes.get_view());
  return &type;
}
