// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/virtual_machine.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/table.hpp"

#include "tetrodotoxin/isa/attribute.hpp"
#include "tetrodotoxin/isa/documentation.hpp"
#include "tetrodotoxin/isa/library/addressable.hpp"
#include "tetrodotoxin/isa/library/alias.hpp"
#include "tetrodotoxin/isa/library/enumeration.hpp"
#include "tetrodotoxin/isa/library/foreign.hpp"
#include "tetrodotoxin/isa/library/function.hpp"
#include "tetrodotoxin/isa/library/scope.hpp"
#include "tetrodotoxin/isa/library/structure.hpp"
#include "tetrodotoxin/isa/library/syntax.hpp"
#include "tetrodotoxin/isa/modifier.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

using DefinitionEvaluator =
    const Ttx::Type* (*)(Library::Scope & scope,
                         Cursor& cursor,
                         const Tetrodotoxin::Isa::Definition& definition);

constexpr Static::Vector<Class::Type, 3> library_modifiers = {{
  Class::Type::Public,
  Class::Type::Private,
  Class::Type::Expose,
}};

constexpr Static::Vector<Pair<View::Bytes, DefinitionEvaluator>, 5>
    library_sub_isas = {{
      {
        Library::Alias::get_name(),
        Library::Alias::evaluate,
      },
      {
        Library::Enumeration::get_name(),
        Library::Enumeration::evaluate,
      },
      {
        Library::Structure::get_name(),
        Library::Structure::evaluate,
      },
      {
        // TODO: object is just a struct with life time management so it uses
        // the same general ISA for now until we have garbage collection.
        "object"_view,
        Library::Structure::evaluate,
      },
      {
        Library::Foreign::get_name(),
        Library::Foreign::evaluate,
      },
    }};

static auto materialize_library_type(
    Library::Scope& scope,
    Cursor& cursor,
    const Tetrodotoxin::Isa::Definition& definition) -> const Ttx::Type* {
  const auto* handler =
      Table<DefinitionEvaluator, library_sub_isas>::find_or_null(
          definition.get_kind());
  return handler == nullptr ? nullptr : (*handler)(scope, cursor, definition);
}

static auto predeclare_library_definitions(
    Library::Scope& scope,
    Cursor& cursor) -> Bool {
  while (!cursor.matches(Class::Type::EndOfStream)) {
    Ttx::Documentation documentation = Documentation::evaluate(cursor);
    if (!Attribute::consume_all(cursor)) {
      return False;
    }
    if (cursor.matches(Class::Type::EndOfStream)) {
      break;
    }

    if (!cursor.is_one_of(library_modifiers)) {
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return False;
      }
      continue;
    }

    Class::Type modifier = cursor.current().get_class().get_type();
    cursor.consume();
    if (cursor.matches(Class::Type::Func)) {
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return False;
      }
      continue;
    }

    const Token& name = cursor.current();
    if (name.get_class() != Class::Type::Type) {
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return False;
      }
      continue;
    }

    cursor.consume();
    if (!cursor.matches(Class::Type::Define)) {
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return False;
      }
      continue;
    }

    cursor.consume();
    const Token& kind = cursor.current();
    if (Table<DefinitionEvaluator, library_sub_isas>::find_or_null(
            kind.get_text()) == nullptr) {
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return False;
      }
      continue;
    }

    Tetrodotoxin::Isa::Definition definition(
        documentation, modifier, name.get_class().get_type(), name.get_text(),
        kind.get_class().get_type(), kind.get_text());
    cursor.consume();
    Count body_index = cursor.get_token_index();
    if (!Library::Syntax::consume_declaration_tail(cursor)) {
      return False;
    }
    Count next_index = cursor.get_token_index();

    if (!scope.declare_type(definition, body_index, next_index)) {
      cursor.range_error(
          name, name, "Library type name is already defined."_view);
      return False;
    }
  }

  return True;
}

auto Library::VirtualMachine::evaluate_definition(
    Library::Scope& scope,
    Cursor& cursor,
    Ttx::Documentation documentation,
    Managed::Vector<Ttx::Type::Member>& members,
    Managed::Vector<const Ttx::Type*>& types,
    Managed::Vector<Ttx::Type::Function>& functions) -> Bool {
  Modifier modifier = Modifier::evaluate(
      cursor, library_modifiers,
      "Expected a definition to start with one of the following modifiers "
      "{public, private, expose}"_view);
  if (!modifier.is_valid()) {
    return False;
  }

  if (cursor.matches(Class::Type::Func)) {
    Ttx::Type::Function function =
        Library::Function::evaluate(scope, cursor, documentation);
    if (function.is_empty()) {
      return False;
    }

    for (Count i = 0; i < functions.get_size(); i++) {
      if (functions[i].get_name() == function.get_name()) {
        cursor.token_error("Library function name is already defined."_view);
        return False;
      }
    }

    functions.insert(function);
    return True;
  }

  Tetrodotoxin::Isa::Definition definition =
      Tetrodotoxin::Isa::Definition::evaluate_after_modifier(
          cursor, documentation, modifier.get_type(),
          {{Class::Type::Type, Class::Type::Addressable}},
          {{Class::Type::Addressable, Class::Type::Type, Class::Type::Alias,
            Class::Type::Func}});
  if (!definition.is_valid()) {
    return False;
  }

  if (definition.has_addressable_name()) {
    Ttx::Type::Member member =
        Library::Addressable::evaluate(scope, cursor, definition);
    if (member.is_empty()) {
      return False;
    }

    for (Count i = 0; i < members.get_size(); i++) {
      if (members[i].get_name() == member.get_name()) {
        cursor.token_error("Library member name is already defined."_view);
        return False;
      }
    }

    members.insert(member);
    return True;
  }

  const auto* handler =
      Table<DefinitionEvaluator, library_sub_isas>::find_or_null(
          definition.get_kind());
  if (handler == nullptr) {
    Managed::Bytes message(cursor.get_arena());
    message.concat(
        "Definition name provided is not one of the known types {"_view);
    for (Count i = 0; i < library_sub_isas.get_size(); i++) {
      if (i != 0) {
        message.concat(", "_view);
      }

      message.concat(library_sub_isas[i].key);
    }
    message.concat("}"_view);
    cursor.token_error(message.get_view());
    return False;
  }

  const Ttx::Type* type = scope.materialize_type(cursor, definition.get_name());
  if (type == nullptr) {
    return False;
  }

  if (!scope.seek_after_type(cursor, definition.get_name())) {
    if (!Library::Syntax::consume_declaration_tail(cursor)) {
      return False;
    }
  }

  types.insert(type);
  return True;
}

auto Library::VirtualMachine::evaluate(Context& context, Cursor& cursor)
    -> Ttx::Type* {
  Managed::Vector<Ttx::Type::Member> members(context.get_arena());
  Managed::Vector<const Ttx::Type*> types(context.get_arena());
  Managed::Vector<Ttx::Type::Function> functions(context.get_arena());
  Library::Scope scope(context, materialize_library_type);
  Count body_start = cursor.get_token_index();

  if (!predeclare_library_definitions(scope, cursor)) {
    return nullptr;
  }

  cursor.seek_token(body_start);

  while (!cursor.matches(Class::Type::EndOfStream)) {
    Ttx::Documentation documentation = Documentation::evaluate(cursor);
    if (!Attribute::consume_all(cursor)) {
      return nullptr;
    }
    if (cursor.matches(Class::Type::EndOfStream)) {
      break;
    }

    if (!evaluate_definition(
            scope, cursor, documentation, members, types, functions)) {
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return nullptr;
      }
      continue;
    }
  }

  if (!cursor.get_errors().is_empty()) {
    return nullptr;
  }

  Managed::Vector<Ttx::Attribute> attributes(context.get_arena());
  attributes.insert({"isa"_view, "Library"_view});
  auto& type = context.get_arena().construct<Ttx::Type>(
      Library::VirtualMachine::get_name(), members.get_view(), types.get_view(),
      functions.get_view(), Ttx::Documentation(), attributes.get_view());
  return &type;
}
