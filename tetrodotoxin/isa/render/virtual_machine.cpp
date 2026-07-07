// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/render/virtual_machine.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/attribute.hpp"
#include "tetrodotoxin/isa/definition.hpp"
#include "tetrodotoxin/isa/documentation.hpp"
#include "tetrodotoxin/isa/expression.hpp"
#include "tetrodotoxin/isa/expression/type.hpp"
#include "tetrodotoxin/isa/modifier.hpp"
#include "tetrodotoxin/isa/render/interface.hpp"
#include "tetrodotoxin/isa/render/stage.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

static auto evaluate_member(
    Cursor& cursor,
    Context& context,
    const Definition& definition,
    View::Bytes initializer_error,
    View::Bytes unresolved_error) -> Ttx::Type::Member {
  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* type =
      Expression::Type::evaluate(cursor, context, definition.get_kind());
  if (cursor.get_errors().get_size() != error_count) {
    return Ttx::Type::Member();
  }

  if (!Expression::consume_initializer(cursor, initializer_error)) {
    return Ttx::Type::Member();
  }

  if (!cursor.require(
          Class::Type::EndStatement,
          "Expected `;` after render member."_view)) {
    return Ttx::Type::Member();
  }

  if (type == nullptr) {
    cursor.token_error(unresolved_error);
    return Ttx::Type::Member();
  }

  return Ttx::Type::Member(
      definition.get_name(), *type, definition.get_documentation());
}

auto Render::VirtualMachine::evaluate(Cursor& cursor, Context& context)
    -> Ttx::Type* {
  Ttx::Documentation documentation = Documentation::evaluate(cursor);
  if (!Attribute::consume_all(cursor)) {
    return nullptr;
  }

  Definition render_definition = Definition::evaluate(
      cursor, documentation, {{Class::Type::Public}},
      {{Class::Type::Type}}, {{Class::Type::Type}});
  if (!render_definition.is_valid()) {
    return nullptr;
  }
  if (render_definition.get_kind() != Render::VirtualMachine::get_name()) {
    cursor.token_error("Expected render definition kind `Render`."_view);
    return nullptr;
  }
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after render declaration."_view)) {
    return nullptr;
  }

  Managed::Vector<Ttx::Type::Member> members(context.get_arena());
  Managed::Vector<const Ttx::Type*> types(context.get_arena());
  Managed::Vector<Ttx::Type::Function> functions(context.get_arena());
  Bool valid = True;
  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    Ttx::Documentation member_documentation = Documentation::evaluate(cursor);
    if (!Attribute::consume_all(cursor)) {
      return nullptr;
    }

    if (cursor.matches(Class::Type::Addressable)) {
      const Ttx::Type* fact_block =
          Interface::evaluate(cursor, context, cursor.current().get_text());
      if (fact_block == nullptr) {
        return nullptr;
      }
      types.insert(fact_block);
      continue;
    }

    Modifier modifier = Modifier::evaluate(
        cursor, {{Class::Type::Public, Class::Type::Private}},
        "Expected render member or stage to start with public or private."_view);
    if (!modifier.is_valid()) {
      return nullptr;
    }

    Definition definition = Definition::evaluate_after_modifier(
        cursor, member_documentation, modifier.get_type(),
        {{Class::Type::Addressable}},
        {{Class::Type::Type, Class::Type::Addressable}});
    if (!definition.is_valid()) {
      return nullptr;
    }

    if (definition.get_kind() == "stage"_view) {
      Stage::Result stage =
          Stage::evaluate(cursor, context, definition, types.get_view());
      if (stage.is_empty()) {
        return nullptr;
      }
      if (!Stage::insert(cursor, functions, stage.get_function())) {
        valid = False;
      }
      types.insert(stage.get_facts());
      continue;
    }

    Ttx::Type::Member member = evaluate_member(
        cursor, context, definition,
        "Expected `;` after render member initializer."_view,
        "Render member type could not be resolved."_view);
    if (!Interface::insert(
            cursor, members, member,
            "Render member name is already defined."_view)) {
      valid = False;
    }
  }

  if (!cursor.require(
          Class::Type::ScopeEnd,
          "Expected `}` after render declaration."_view)) {
    return nullptr;
  }

  if (!valid || !cursor.matches(Class::Type::EndOfStream)) {
    return nullptr;
  }

  Managed::Vector<Ttx::Attribute> render_attributes(context.get_arena());
  render_attributes.insert({"isa"_view, "Render"_view});
  auto& render_type = context.get_arena().construct<Ttx::Type>(
      render_definition.get_name(), members.get_view(), types.get_view(),
      functions.get_view(), render_definition.get_documentation(),
      render_attributes.get_view());

  Managed::Vector<const Ttx::Type*> root_types(context.get_arena());
  root_types.insert(&render_type);
  Managed::Vector<Ttx::Attribute> root_attributes(context.get_arena());
  root_attributes.insert({"isa"_view, "RenderSource"_view});
  return &context.get_arena().construct<Ttx::Type>(
      Render::VirtualMachine::get_name(), View::Vector<Ttx::Type::Member>(),
      root_types.get_view(), View::Vector<Ttx::Type::Function>(),
      Ttx::Documentation(), root_attributes.get_view());
}
