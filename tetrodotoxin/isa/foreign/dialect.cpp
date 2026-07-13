// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/foreign/dialect.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/compiler/linkage.hpp"
#include "tetrodotoxin/isa/base/attribute.hpp"
#include "tetrodotoxin/isa/base/documentation.hpp"
#include "tetrodotoxin/isa/base/modifier.hpp"
#include "tetrodotoxin/isa/library/function.hpp"
#include "tetrodotoxin/isa/library/syntax.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Foreign::Dialect::evaluate(
    Cursor& cursor,
    Library::Scope& scope,
    const Base::Declaration& definition) -> const Ttx::Type* {
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after foreign declaration."_view)) {
    return nullptr;
  }

  Managed::Vector<Ttx::Function> functions(scope.get_context().get_arena());
  Bool valid = True;
  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    Ttx::Documentation documentation = Base::Documentation::evaluate(cursor);
    if (!Base::Attribute::consume_all(cursor)) {
      return nullptr;
    }

    Class::Type modifier = Base::Modifier::evaluate(
        cursor, {{Class::Type::Expose}},
        "Expected a foreign function declaration to start with expose."_view);
    if (modifier == Class::Type::Unknown) {
      valid = False;
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return nullptr;
      }

      continue;
    }

    Ttx::Function function =
        Library::Function::evaluate_declaration(cursor, scope, documentation);
    if (function.is_empty()) {
      valid = False;
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return nullptr;
      }

      continue;
    }

    for (Count i = 0; i < functions.get_size(); i++) {
      if (functions[i].get_name() == function.get_name()) {
        cursor.token_error("Foreign function name is already defined."_view);
        valid = False;
        break;
      }
    }

    if (valid) {
      functions.insert(function);
    }
  }

  if (!cursor.require(
          Class::Type::ScopeEnd,
          "Expected `}` after foreign declaration."_view) ||
      !valid) {
    return nullptr;
  }

  Managed::Vector<Ttx::Attribute> attributes(scope.get_context().get_arena());
  Base::Attribute::append_all(definition.get_attributes(), attributes);
  auto& type = scope.get_context().get_arena().construct<Ttx::Type>(
      definition.get_name(), View::Vector<Ttx::Member>(),
      View::Vector<const Ttx::Type*>(), functions.get_view(),
      definition.get_documentation(), attributes.get_view());

  View::Vector<Ttx::Function> published = type.get_functions();
  for (Count i = 0; i < published.get_size(); i++) {
    if (!scope.get_context().define_linkage(
            Tetrodotoxin::Compiler::Linkage(
                type, published[i], published[i].get_name()))) {
      return nullptr;
    }
  }

  return &type;
}
