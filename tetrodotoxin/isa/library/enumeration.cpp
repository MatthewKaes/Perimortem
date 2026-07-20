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
  Bool has_index = cursor.require(
      Code::Type::LayoutStart,
      "Expected `[` before library enum storage type."_view);
  if (!has_index) {
    return nullptr;
  }

  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* storage = scope.resolve_type(cursor);
  if (cursor.get_errors().get_size() != error_count) {
    return nullptr;
  }

  Bool has_index_end = cursor.require(
      Code::Type::LayoutEnd,
      "Expected `]` after library enum storage type."_view);
  if (!has_index_end) {
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
  Bool has_scope = cursor.require(
      Code::Type::ScopeStart,
      "Expected `{` after library enum storage type."_view);
  if (!has_scope) {
    return nullptr;
  }

  Managed::Vector<Ttx::Member> members(scope.get_context().get_arena());
  Managed::Vector<Base::Definition> member_definitions(
      scope.get_context().get_arena());
  Managed::Vector<Ttx::Function> functions(scope.get_context().get_arena());
  Managed::Vector<Ttx::Function> addressable_functions(
      scope.get_context().get_arena());
  Managed::Vector<Perimortem::Utility::Range> function_sources(
      scope.get_context().get_arena());
  Managed::Vector<Perimortem::Utility::Range> addressable_function_sources(
      scope.get_context().get_arena());
  Managed::Vector<Base::Definition> function_definitions(
      scope.get_context().get_arena());
  Managed::Vector<Base::Definition> addressable_function_definitions(
      scope.get_context().get_arena());
  Ttx::Type* type = scope.get_context().get_arena().reserve<Ttx::Type>();
  Bool staged = scope.stage_type_reference(definition.get_name(), type);
  if (!staged) {
    cursor.token_error(
        "Library enum type could not stage self reference."_view);
    return nullptr;
  }

  Bool valid = True;
  while (!cursor.matches(Code::Type::Terminal) &&
         !cursor.matches(Code::Type::ScopeEnd)) {
    Ttx::Documentation documentation = Base::Documentation::evaluate(cursor);
    Managed::Vector<Ttx::Attribute> source_attributes(
        scope.get_context().get_arena());
    Bool attributes_evaluated =
        Base::Attribute::evaluate_all(cursor, source_attributes);
    if (!attributes_evaluated) {
      return nullptr;
    }

    if (cursor.matches(Code::Type::Addressable)) {
      View::Bytes name = cursor.current().get_text();
      cursor.consume();
      if (!cursor.matches(Code::Type::Assign)) {
        cursor.token_error("Expected `=` after library enum case name."_view);
        valid = False;
        Bool recovered = Library::Syntax::consume_declaration_tail(cursor);
        if (!recovered) {
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

      Bool has_statement_end = cursor.require(
          Code::Type::EndStatement,
          "Expected `;` after library enum case."_view);
      if (!has_statement_end) {
        return nullptr;
      }

      Bool inserted = insert_enum_case(
          cursor, members, name, storage, documentation,
          source_attributes.get_view());
      if (!inserted) {
        valid = False;
      } else {
        member_definitions.insert(
            Base::Definition(
                Code::Type::Expose, source_attributes.get_view(), value));
      }

      continue;
    }

    Code::Type modifier = Base::Modifier::evaluate(
        cursor, {{Code::Type::Public, Code::Type::Private, Code::Type::Expose}},
        "Expected enum body to contain a case name or function modifier."_view);
    if (modifier == Code::Type::Unknown) {
      valid = False;
      Bool recovered = Library::Syntax::consume_declaration_tail(cursor);
      if (!recovered) {
        return nullptr;
      }

      continue;
    }

    if (!cursor.matches(Code::Type::Func)) {
      cursor.token_error("Expected enum member function."_view);
      valid = False;
      Bool recovered = Library::Syntax::consume_declaration_tail(cursor);
      if (!recovered) {
        return nullptr;
      }

      continue;
    }

    Perimortem::Utility::Range source;
    Bool addressable = False;
    Ttx::Function function = Library::Function::evaluate(
        cursor, scope, documentation, source, type, addressable);
    if (function.is_empty()) {
      valid = False;
      Bool recovered = Library::Syntax::consume_declaration_tail(cursor);
      if (!recovered) {
        return nullptr;
      }

      continue;
    }

    View::Vector<Ttx::Function> siblings =
        addressable ? addressable_functions.get_view() : functions.get_view();
    if (has_function(siblings, function.get_name())) {
      cursor.token_error("Library function name is already defined."_view);
      valid = False;
      continue;
    }

    if (addressable) {
      addressable_functions.insert(function);
      addressable_function_sources.insert(source);
      addressable_function_definitions.insert(
          Base::Definition(modifier, source_attributes.get_view()));
    } else {
      functions.insert(function);
      function_sources.insert(source);
      function_definitions.insert(
          Base::Definition(modifier, source_attributes.get_view()));
    }
  }

  Bool has_scope_end = cursor.require(
      Code::Type::ScopeEnd,
      "Expected `}` after library enum declaration."_view);
  if (!has_scope_end) {
    return nullptr;
  }

  if (!valid) {
    return nullptr;
  }

  Managed::Vector<Ttx::Attribute> attributes(scope.get_context().get_arena());
  Base::Attribute::append_all(definition.get_attributes(), attributes);
  new (type) Ttx::Type(
      definition.get_name(), members.get_view(),
      View::Vector<Ttx::Type::Reference>(), functions.get_view(),
      addressable_functions.get_view(), definition.get_documentation(),
      attributes.get_view());
  for (Count i = 0; i < members.get_size(); i++) {
    Bool implementation_defined = scope.get_context().define_implementation(
        members[i], member_definitions[i]);
    if (!implementation_defined) {
      return nullptr;
    }
  }

  Bool published_type = Library::Compiler::Function::publish(
      cursor, scope, *type, definition.get_name(), type->get_type_functions(),
      function_sources.get_view(), function_definitions.get_view(), False);
  if (!published_type) {
    return nullptr;
  }

  Bool published_addressable = Library::Compiler::Function::publish(
      cursor, scope, *type, definition.get_name(),
      type->get_addressable_functions(),
      addressable_function_sources.get_view(),
      addressable_function_definitions.get_view(), True);
  if (!published_addressable) {
    return nullptr;
  }

  return type;
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
