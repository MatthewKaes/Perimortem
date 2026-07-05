// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/render/virtual_machine.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/attribute.hpp"
#include "tetrodotoxin/isa/definition.hpp"
#include "tetrodotoxin/isa/documentation.hpp"
#include "tetrodotoxin/isa/expression.hpp"
#include "tetrodotoxin/isa/layout/evaluator.hpp"
#include "tetrodotoxin/isa/modifier.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

static auto block_type_name(View::Bytes block_name) -> View::Bytes {
  if (block_name == "constants"_view) {
    return "constant"_view;
  }
  if (block_name == "push_constants"_view) {
    return "push"_view;
  }
  if (block_name == "resources"_view) {
    return "resource"_view;
  }
  return View::Bytes();
}

static auto insert_member(
    Cursor& cursor,
    Managed::Vector<Ttx::Type::Member>& members,
    Ttx::Type::Member member,
    View::Bytes duplicate_error) -> Bool {
  if (member.is_empty()) {
    return False;
  }

  for (Count i = 0; i < members.get_size(); i++) {
    if (members[i].get_name() == member.get_name()) {
      cursor.token_error(duplicate_error);
      return False;
    }
  }

  members.insert(member);
  return True;
}

static auto evaluate_member(
    Context& context,
    Cursor& cursor,
    const Definition& definition,
    View::Bytes initializer_error,
    View::Bytes unresolved_error) -> Ttx::Type::Member {
  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* type = context.resolve_type(cursor, definition.get_kind());
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

static auto evaluate_fact_block(
    Context& context,
    Cursor& cursor,
    View::Bytes block_name) -> const Ttx::Type* {
  View::Bytes type_name = block_type_name(block_name);
  if (type_name.is_empty()) {
    cursor.token_error("Expected render fact block."_view);
    return nullptr;
  }

  cursor.consume();
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after render fact block name."_view)) {
    return nullptr;
  }

  Managed::Vector<Ttx::Type::Member> members(context.get_arena());
  Bool valid = True;
  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    Ttx::Documentation documentation = Documentation::evaluate(cursor);
    Modifier modifier = Modifier::evaluate(
        cursor, {{Class::Type::Const, Class::Type::State}},
        "Expected render fact to start with `const` or `state`."_view);
    if (!modifier.is_valid()) {
      return nullptr;
    }

    Definition definition = Definition::evaluate_after_modifier(
        cursor, documentation, modifier.get_type(),
        {{Class::Type::Addressable}}, {{Class::Type::Type}});
    if (!definition.is_valid()) {
      return nullptr;
    }

    Ttx::Type::Member member = evaluate_member(
        context, cursor, definition,
        "Expected `;` after render fact initializer."_view,
        "Render fact type could not be resolved."_view);
    if (!insert_member(
            cursor, members, member,
            "Render fact name is already defined."_view)) {
      valid = False;
    }
  }

  if (!cursor.require(
          Class::Type::ScopeEnd,
          "Expected `}` after render fact block."_view)) {
    return nullptr;
  }

  if (!valid) {
    return nullptr;
  }

  Managed::Vector<Ttx::Attribute> attributes(context.get_arena());
  attributes.insert({"isa"_view, "RenderFacts"_view});
  attributes.insert({"render_block"_view, block_name});
  return &context.get_arena().construct<Ttx::Type>(
      type_name, members.get_view(), View::Vector<const Ttx::Type*>(),
      View::Vector<Ttx::Type::Function>(), Ttx::Documentation(),
      attributes.get_view());
}

static auto evaluate_stage(
    Context& context,
    Cursor& cursor,
    const Definition& definition) -> Ttx::Type::Function {
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after render stage declaration."_view)) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Member> parameters(context.get_arena());
  Managed::Vector<Ttx::Type::Member> result(context.get_arena());
  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    if (!cursor.matches(Class::Type::Addressable)) {
      cursor.token_error("Expected render stage directive."_view);
      return Ttx::Type::Function();
    }

    View::Bytes directive = cursor.current().get_text();
    cursor.consume();
    if (directive == "reads"_view) {
      if (!Expression::consume(
              cursor,
              "Expected `;` after render stage reads declaration."_view)) {
        return Ttx::Type::Function();
      }
      if (!cursor.require(
              Class::Type::EndStatement,
              "Expected `;` after render stage reads declaration."_view)) {
        return Ttx::Type::Function();
      }
      continue;
    }

    Managed::Vector<Ttx::Type::Member>* target = nullptr;
    if (directive == "input"_view) {
      target = &parameters;
    } else if (directive == "output"_view) {
      target = &result;
    } else {
      cursor.token_error("Expected render stage input, output, or reads."_view);
      return Ttx::Type::Function();
    }

    if (!Layout::Evaluator::evaluate_bracketed(context, cursor, *target)) {
      return Ttx::Type::Function();
    }
    if (!cursor.require(
            Class::Type::EndStatement,
            "Expected `;` after render stage layout."_view)) {
      return Ttx::Type::Function();
    }
  }

  if (!cursor.require(
          Class::Type::ScopeEnd,
          "Expected `}` after render stage declaration."_view)) {
    return Ttx::Type::Function();
  }

  return Ttx::Type::Function(
      definition.get_name(), parameters.get_view(), result.get_view(),
      definition.get_documentation());
}

auto Render::VirtualMachine::evaluate(Context& context, Cursor& cursor)
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
          evaluate_fact_block(context, cursor, cursor.current().get_text());
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
      Ttx::Type::Function stage =
          evaluate_stage(context, cursor, definition);
      if (stage.is_empty()) {
        return nullptr;
      }
      for (Count i = 0; i < functions.get_size(); i++) {
        if (functions[i].get_name() == stage.get_name()) {
          cursor.token_error("Render stage name is already defined."_view);
          valid = False;
        }
      }
      if (valid) {
        functions.insert(stage);
      }
      continue;
    }

    Ttx::Type::Member member = evaluate_member(
        context, cursor, definition,
        "Expected `;` after render member initializer."_view,
        "Render member type could not be resolved."_view);
    if (!insert_member(
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
