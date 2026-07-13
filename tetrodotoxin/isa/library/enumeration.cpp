// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/enumeration.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/attribute.hpp"
#include "tetrodotoxin/isa/base/documentation.hpp"
#include "tetrodotoxin/isa/base/expression/evaluator.hpp"
#include "tetrodotoxin/isa/base/expression/value.hpp"
#include "tetrodotoxin/isa/base/modifier.hpp"
#include "tetrodotoxin/isa/library/compiler/function.hpp"
#include "tetrodotoxin/isa/library/function.hpp"
#include "tetrodotoxin/isa/library/syntax.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

static auto parse_storage_type(Cursor& cursor, Library::Scope& scope)
    -> const Ttx::Type* {
  if (!cursor.require(
          Class::Type::IndexStart,
          "Expected `[` before library enum storage type."_view)) {
    return nullptr;
  }

  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* storage = scope.resolve_type(cursor);
  if (cursor.get_errors().get_size() != error_count) {
    return nullptr;
  }

  if (!cursor.require(
          Class::Type::IndexEnd,
          "Expected `]` after library enum storage type."_view)) {
    return nullptr;
  }

  if (storage == nullptr) {
    cursor.token_error("Library enum storage type could not be resolved."_view);
  }

  return storage;
}

static auto has_member(View::Vector<Ttx::Member> members, View::Bytes name)
    -> Bool {
  for (Count i = 0; i < members.get_size(); i++) {
    if (members[i].get_name() == name) {
      return True;
    }
  }

  return False;
}

static auto has_function(
    View::Vector<Ttx::Function> functions,
    View::Bytes name) -> Bool {
  for (Count i = 0; i < functions.get_size(); i++) {
    if (functions[i].get_name() == name) {
      return True;
    }
  }

  return False;
}

static auto insert_enum_case(
    Cursor& cursor,
    Managed::Vector<Ttx::Member>& members,
    View::Bytes name,
    const Ttx::Type& storage,
    Ttx::Documentation documentation,
    View::Vector<Ttx::Attribute> attributes) -> Bool {
  if (has_member(members.get_view(), name)) {
    cursor.token_error("Library enum case name is already defined."_view);
    return False;
  }

  members.insert(Ttx::Member(name, storage, documentation, False, attributes));
  return True;
}

static auto evaluate_cases(
    Cursor& cursor,
    Library::Scope& scope,
    const Tetrodotoxin::Isa::Base::Declaration& definition,
    const Ttx::Type& storage) -> const Ttx::Type* {
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after library enum storage type."_view)) {
    return nullptr;
  }

  Managed::Vector<Ttx::Member> members(scope.get_context().get_arena());
  Managed::Vector<Base::Definition> member_definitions(
      scope.get_context().get_arena());
  Managed::Vector<Ttx::Function> functions(scope.get_context().get_arena());
  Managed::Vector<Perimortem::Utility::Range> function_sources(
      scope.get_context().get_arena());
  Managed::Vector<Base::Definition> function_definitions(
      scope.get_context().get_arena());
  Ttx::Type& type = scope.get_context().get_arena().allocate<Ttx::Type>();
  if (!scope.stage_type_reference(definition.get_name(), type)) {
    cursor.token_error(
        "Library enum type could not stage self reference."_view);
    return nullptr;
  }

  Bool valid = True;
  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    Ttx::Documentation documentation = Base::Documentation::evaluate(cursor);
    Managed::Vector<Ttx::Attribute> source_attributes(
        scope.get_context().get_arena());
    if (!Base::Attribute::evaluate_all(cursor, source_attributes)) {
      return nullptr;
    }

    if (cursor.matches(Class::Type::Addressable)) {
      View::Bytes name = cursor.current().get_text();
      cursor.consume();
      if (!cursor.matches(Class::Type::Assign)) {
        cursor.token_error("Expected `=` after library enum case name."_view);
        valid = False;
        if (!Library::Syntax::consume_declaration_tail(cursor)) {
          return nullptr;
        }

        continue;
      }

      cursor.consume();

      Base::Expression::Value value =
          Base::Expression::Value::evaluate(cursor, scope.get_context());
      if (value.is_empty()) {
        return nullptr;
      }

      const Base::Expression::Value& initializer =
          scope.get_context().get_arena().construct<Base::Expression::Value>(
              value);
      if (!cursor.require(
              Class::Type::EndStatement,
              "Expected `;` after library enum case."_view)) {
        return nullptr;
      }

      if (!insert_enum_case(
              cursor, members, name, storage, documentation,
              source_attributes.get_view())) {
        valid = False;
      } else {
        member_definitions.insert(
            Base::Definition(
                Class::Type::Expose, source_attributes.get_view(),
                &initializer));
      }

      continue;
    }

    Class::Type modifier = Base::Modifier::evaluate(
        cursor,
        {{Class::Type::Public, Class::Type::Private, Class::Type::Expose}},
        "Expected enum body to contain a case name or function modifier."_view);
    if (modifier == Class::Type::Unknown) {
      valid = False;
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return nullptr;
      }

      continue;
    }

    if (!cursor.matches(Class::Type::Func)) {
      cursor.token_error("Expected enum member function."_view);
      valid = False;
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return nullptr;
      }

      continue;
    }

    Perimortem::Utility::Range source;
    Ttx::Function function =
        Library::Function::evaluate(cursor, scope, documentation, source);
    if (function.is_empty()) {
      valid = False;
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return nullptr;
      }

      continue;
    }

    if (has_function(functions.get_view(), function.get_name())) {
      cursor.token_error("Library function name is already defined."_view);
      valid = False;
      continue;
    }

    functions.insert(function);
    function_sources.insert(source);
    function_definitions.insert(
        Base::Definition(modifier, source_attributes.get_view()));
  }

  if (!cursor.require(
          Class::Type::ScopeEnd,
          "Expected `}` after library enum declaration."_view)) {
    return nullptr;
  }

  if (!valid) {
    return nullptr;
  }

  Managed::Vector<Ttx::Attribute> attributes(scope.get_context().get_arena());
  Base::Attribute::append_all(definition.get_attributes(), attributes);
  new (&type) Ttx::Type(
      definition.get_name(), members.get_view(),
      View::Vector<const Ttx::Type*>(), functions.get_view(),
      definition.get_documentation(), attributes.get_view());
  for (Count i = 0; i < members.get_size(); i++) {
    if (!scope.get_context().define_implementation(
            members[i], member_definitions[i])) {
      return nullptr;
    }
  }

  if (!Library::Compiler::Function::publish(
          cursor, scope, type, function_sources.get_view(),
          function_definitions.get_view())) {
    return nullptr;
  }

  return &type;
}

auto Library::Enumeration::evaluate(
    Cursor& cursor,
    Library::Scope& scope,
    const Tetrodotoxin::Isa::Base::Declaration& definition)
    -> const Ttx::Type* {
  const Ttx::Type* storage = parse_storage_type(cursor, scope);
  if (storage == nullptr) {
    return nullptr;
  }

  return evaluate_cases(cursor, scope, definition, *storage);
}
