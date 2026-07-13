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
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after library aggregate declaration."_view)) {
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
        "Library aggregate type could not stage self reference."_view);
    return nullptr;
  }

  Bool valid = True;
  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    Ttx::Documentation member_documentation =
        Base::Documentation::evaluate(cursor);
    Managed::Vector<Ttx::Attribute> member_attributes(
        scope.get_context().get_arena());
    if (!Base::Attribute::evaluate_all(cursor, member_attributes)) {
      return nullptr;
    }

    Class::Type modifier = Base::Modifier::evaluate(
        cursor,
        {{Class::Type::Public, Class::Type::Private, Class::Type::Expose,
          Class::Type::State, Class::Type::Const}},
        "Expected a definition to start with one of the following modifiers "
        "{public, private, expose, state, const}"_view);
    if (modifier == Class::Type::Unknown) {
      valid = False;
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return nullptr;
      }

      continue;
    }

    if (cursor.matches(Class::Type::Func)) {
      Perimortem::Utility::Range source;
      Ttx::Function function = Library::Function::evaluate(
          cursor, scope, member_documentation, source);
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
      function_sources.insert(source);
      function_definitions.insert(
          Base::Definition(modifier, member_attributes.get_view()));
      continue;
    }

    Tetrodotoxin::Isa::Base::Declaration member_definition =
        Tetrodotoxin::Isa::Base::Declaration::evaluate_after_modifier(
            cursor, member_documentation, modifier,
            {{Class::Type::Addressable}}, {{Class::Type::Type}},
            member_attributes.get_view());
    if (!member_definition.is_valid()) {
      valid = False;
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
        return nullptr;
      }

      continue;
    }

    Base::Definition member_implementation;
    const Ttx::Member* member = Library::Addressable::evaluate(
        cursor, scope, member_definition, member_implementation);
    if (member == nullptr) {
      valid = False;
      if (!Library::Syntax::consume_declaration_tail(cursor)) {
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

  if (!cursor.require(
          Class::Type::ScopeEnd,
          "Expected `}` after library struct declaration."_view)) {
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
