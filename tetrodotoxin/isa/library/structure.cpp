// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/structure.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/attribute.hpp"
#include "tetrodotoxin/isa/base/documentation.hpp"
#include "tetrodotoxin/isa/base/modifier.hpp"
#include "tetrodotoxin/isa/library/addressable.hpp"
#include "tetrodotoxin/isa/library/compiler/function.hpp"
#include "tetrodotoxin/isa/library/function.hpp"
#include "tetrodotoxin/isa/library/syntax.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Library::Structure::evaluate(
    Cursor& cursor,
    Library::Scope& scope,
    const Tetrodotoxin::Isa::Base::Declaration& definition)
    -> const Ttx::Type* {
  Bool has_scope = cursor.require(
      Code::Type::ScopeStart,
      "Expected `{` after library aggregate declaration."_view);
  if (!has_scope) {
    return nullptr;
  }

  Managed::Vector<Ttx::Member> members(scope.get_context().get_arena());
  Managed::Vector<Base::Definition> member_definitions(
      scope.get_context().get_arena());
  Managed::Vector<Ttx::Function> type_functions(
      scope.get_context().get_arena());
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
        "Library aggregate type could not stage self reference."_view);
    return nullptr;
  }

  Bool valid = True;
  while (!cursor.matches(Code::Type::Terminal) &&
         !cursor.matches(Code::Type::ScopeEnd)) {
    Ttx::Documentation member_documentation =
        Base::Documentation::evaluate(cursor);
    Managed::Vector<Ttx::Attribute> member_attributes(
        scope.get_context().get_arena());
    Bool attributes_evaluated =
        Base::Attribute::evaluate_all(cursor, member_attributes);
    if (!attributes_evaluated) {
      return nullptr;
    }

    Code::Type modifier = Base::Modifier::evaluate(
        cursor,
        {{Code::Type::Public, Code::Type::Private, Code::Type::Expose,
          Code::Type::State, Code::Type::Const}},
        "Expected a definition to start with one of the following modifiers "
        "{public, private, expose, state, const}"_view);
    if (modifier == Code::Type::Unknown) {
      valid = False;
      Bool recovered = Library::Syntax::consume_declaration_tail(cursor);
      if (!recovered) {
        return nullptr;
      }

      continue;
    }

    if (cursor.matches(Code::Type::Func)) {
      Perimortem::Utility::Range source;
      Bool addressable = False;
      Ttx::Function function = Library::Function::evaluate(
          cursor, scope, member_documentation, source, type, addressable);
      if (function.is_empty()) {
        valid = False;
        Bool recovered = Library::Syntax::consume_declaration_tail(cursor);
        if (!recovered) {
          return nullptr;
        }

        continue;
      }

      View::Vector<Ttx::Function> siblings =
          addressable ? addressable_functions.get_view()
                      : type_functions.get_view();
      Bool is_duplicate = False;
      for (Count i = 0; i < siblings.get_size(); i++) {
        if (siblings[i].get_name() == function.get_name()) {
          is_duplicate = True;
          break;
        }
      }

      if (is_duplicate) {
        cursor.token_error("Library function name is already defined."_view);
        valid = False;
      }

      if (!valid) {
        continue;
      }

      if (addressable) {
        addressable_functions.insert(function);
        addressable_function_sources.insert(source);
        addressable_function_definitions.insert(
            Base::Definition(modifier, member_attributes.get_view()));
      } else {
        type_functions.insert(function);
        function_sources.insert(source);
        function_definitions.insert(
            Base::Definition(modifier, member_attributes.get_view()));
      }

      continue;
    }

    Tetrodotoxin::Isa::Base::Declaration member_definition =
        Tetrodotoxin::Isa::Base::Declaration::evaluate_after_modifier(
            cursor, member_documentation, modifier, {{Code::Type::Addressable}},
            {{Code::Type::Type}}, member_attributes.get_view());
    if (member_definition.is_empty()) {
      valid = False;
      Bool recovered = Library::Syntax::consume_declaration_tail(cursor);
      if (!recovered) {
        return nullptr;
      }

      continue;
    }

    Base::Definition member_implementation;
    const Ttx::Member* member = Library::Addressable::evaluate(
        cursor, scope, member_definition, member_implementation);
    if (member == nullptr) {
      valid = False;
      Bool recovered = Library::Syntax::consume_declaration_tail(cursor);
      if (!recovered) {
        return nullptr;
      }

      continue;
    }

    for (Count i = 0; i < members.get_size(); i++) {
      if (members[i].get_name() == member->get_name()) {
        cursor.token_error("Library member name is already defined."_view);
        valid = False;
        break;
      }
    }

    if (!valid) {
      continue;
    }

    members.insert(*member);
    member_definitions.insert(member_implementation);
  }

  Bool has_scope_end = cursor.require(
      Code::Type::ScopeEnd,
      "Expected `}` after library struct declaration."_view);
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
      View::Vector<Ttx::Type::Reference>(), type_functions.get_view(),
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
