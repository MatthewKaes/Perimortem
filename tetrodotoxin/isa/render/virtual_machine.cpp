// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/render/virtual_machine.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/attribute.hpp"
#include "tetrodotoxin/isa/base/declaration.hpp"
#include "tetrodotoxin/isa/base/documentation.hpp"
#include "tetrodotoxin/isa/base/expression/evaluator.hpp"
#include "tetrodotoxin/isa/base/expression/type.hpp"
#include "tetrodotoxin/isa/base/modifier.hpp"
#include "tetrodotoxin/isa/render/interface.hpp"
#include "tetrodotoxin/isa/render/stage.hpp"
#include "tetrodotoxin/isa/render/stage_result.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Render::VirtualMachine::evaluate_member(
    Cursor& cursor,
    Base::Context& context,
    const Base::Declaration& definition,
    View::Bytes initializer_error,
    View::Bytes unresolved_error) -> const Ttx::Member* {
  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* type =
      Base::Expression::Type::evaluate(cursor, context, definition.get_kind());
  if (cursor.get_errors().get_size() != error_count) {
    return nullptr;
  }

  Bool initializer_consumed = Base::Expression::Evaluator::consume_initializer(
      cursor, initializer_error);
  if (!initializer_consumed) {
    return nullptr;
  }

  Bool has_statement_end = cursor.require(
      Code::Type::EndStatement, "Expected `;` after render member."_view);
  if (!has_statement_end) {
    return nullptr;
  }

  if (type == nullptr) {
    cursor.token_error(unresolved_error);
    return nullptr;
  }

  return &context.get_arena().construct<Ttx::Member>(
      definition.get_name(), *type, definition.get_documentation());
}

auto Render::VirtualMachine::evaluate(Cursor& cursor, Base::Context& context)
    -> Ttx::Type* {
  Ttx::Documentation documentation = Base::Documentation::evaluate(cursor);
  Bool attributes_consumed = Base::Attribute::consume_all(cursor);
  if (!attributes_consumed) {
    return nullptr;
  }

  Base::Declaration render_definition = Base::Declaration::evaluate(
      cursor, documentation, {{Code::Type::Public}}, {{Code::Type::Type}},
      {{Code::Type::Type}});
  if (render_definition.is_empty()) {
    return nullptr;
  }

  if (render_definition.get_kind() != Render::VirtualMachine::get_name()) {
    cursor.token_error("Expected render definition kind `Render`."_view);
    return nullptr;
  }

  Bool has_scope = cursor.require(
      Code::Type::ScopeStart, "Expected `{` after render declaration."_view);
  if (!has_scope) {
    return nullptr;
  }

  Managed::Vector<Ttx::Member> members(context.get_arena());
  Managed::Vector<Ttx::Type::Reference> types(context.get_arena());
  Managed::Vector<Ttx::Function> functions(context.get_arena());
  Bool valid = True;
  while (!cursor.matches(Code::Type::Terminal) &&
         !cursor.matches(Code::Type::ScopeEnd)) {
    Ttx::Documentation member_documentation =
        Base::Documentation::evaluate(cursor);
    Bool member_attributes_consumed = Base::Attribute::consume_all(cursor);
    if (!member_attributes_consumed) {
      return nullptr;
    }

    if (cursor.matches(Code::Type::Addressable)) {
      const Ttx::Type* fact_block =
          Interface::evaluate(cursor, context, cursor.current().get_text());
      if (fact_block == nullptr) {
        return nullptr;
      }

      types.insert(Ttx::Type::Reference(*fact_block));
      continue;
    }

    Code::Type modifier = Base::Modifier::evaluate(
        cursor, {{Code::Type::Public, Code::Type::Private}},
        "Expected render member or stage to start with public or private."_view);
    if (modifier == Code::Type::Unknown) {
      return nullptr;
    }

    Base::Declaration definition = Base::Declaration::evaluate_after_modifier(
        cursor, member_documentation, modifier, {{Code::Type::Addressable}},
        {{Code::Type::Type, Code::Type::Addressable}});
    if (definition.is_empty()) {
      return nullptr;
    }

    if (definition.get_kind() == "stage"_view) {
      StageResult stage =
          Stage::evaluate(cursor, context, definition, types.get_view());
      if (stage.is_empty()) {
        return nullptr;
      }

      Bool inserted = Stage::insert(cursor, functions, stage.get_function());
      if (!inserted) {
        valid = False;
      }

      types.insert(Ttx::Type::Reference(stage.get_facts()));
      continue;
    }

    const Ttx::Member* member = evaluate_member(
        cursor, context, definition,
        "Expected `;` after render member initializer."_view,
        "Render member type could not be resolved."_view);
    if (member == nullptr) {
      return nullptr;
    }

    Bool inserted = Interface::insert(
        cursor, members, *member,
        "Render member name is already defined."_view);
    if (!inserted) {
      valid = False;
    }
  }

  Bool has_scope_end = cursor.require(
      Code::Type::ScopeEnd, "Expected `}` after render declaration."_view);
  if (!has_scope_end) {
    return nullptr;
  }

  if (!valid || !cursor.matches(Code::Type::Terminal)) {
    return nullptr;
  }

  auto& render_type = context.get_arena().construct<Ttx::Type>(
      render_definition.get_name(), members.get_view(), types.get_view(),
      functions.get_view(), View::Vector<Ttx::Function>(),
      render_definition.get_documentation(),
      render_definition.get_attributes());

  Managed::Vector<Ttx::Type::Reference> root_types(context.get_arena());
  root_types.insert(Ttx::Type::Reference(render_type));
  return &context.get_arena().construct<Ttx::Type>(
      Render::VirtualMachine::get_name(), View::Vector<Ttx::Member>(),
      root_types.get_view());
}

auto Render::VirtualMachine::lower(
    Tetrodotoxin::Isa::Lowering::Context&,
    const Tetrodotoxin::Isa::Lowering::Input&) -> Bool {
  return True;
}
