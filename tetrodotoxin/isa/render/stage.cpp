// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/render/stage.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/table.hpp"

#include "tetrodotoxin/isa/base/layout/evaluator.hpp"
#include "tetrodotoxin/isa/render/interface.hpp"
#include "tetrodotoxin/isa/render/stage_result.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

enum class StageDirective : Bits_8 {
  Reads,
  Input,
  Output,
  Invalid,
};

static constexpr Static::Vector<Pair<View::Bytes, StageDirective>, 3>
    stage_directives = {{
      {"reads"_view, StageDirective::Reads},
      {"input"_view, StageDirective::Input},
      {"output"_view, StageDirective::Output},
    }};

using StageDirectives = Table<StageDirective, stage_directives>;

enum class ReadKind : Bits_8 {
  Constant,
  Push,
  Resource,
  Invalid,
};

static constexpr Static::Vector<Pair<View::Bytes, ReadKind>, 3> read_kinds = {{
  {"constant"_view, ReadKind::Constant},
  {"push"_view, ReadKind::Push},
  {"resource"_view, ReadKind::Resource},
}};

using ReadKinds = Table<ReadKind, read_kinds>;

auto Render::Stage::evaluate(
    Cursor& cursor,
    Base::Context& context,
    const Base::Declaration& definition,
    View::Vector<const Ttx::Type*> facts) -> StageResult {
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after render stage declaration."_view)) {
    return StageResult();
  }

  Managed::Vector<Ttx::Member> parameters(context.get_arena());
  Managed::Vector<Ttx::Member> result(context.get_arena());
  Managed::Vector<Ttx::Member> constants(context.get_arena());
  Managed::Vector<Ttx::Member> pushes(context.get_arena());
  Managed::Vector<Ttx::Member> resources(context.get_arena());
  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    if (!cursor.matches(Class::Type::Addressable)) {
      cursor.token_error("Expected render stage directive."_view);
      return StageResult();
    }

    StageDirective directive = StageDirectives::find_or_default(
        cursor.consume().get_text(), StageDirective::Invalid);
    if (directive == StageDirective::Reads) {
      if (!consume_reads(
              cursor, context, facts, constants, pushes, resources)) {
        return StageResult();
      }

      continue;
    }

    Managed::Vector<Ttx::Member>* target = nullptr;
    switch (directive) {
    case StageDirective::Input:
      target = &parameters;
      break;
    case StageDirective::Output:
      target = &result;
      break;
    default:
      cursor.token_error("Expected render stage input, output, or reads."_view);
      return StageResult();
    }

    if (!Base::Layout::Evaluator::evaluate_bracketed(
            cursor, context, *target)) {
      return StageResult();
    }

    if (!cursor.require(
            Class::Type::EndStatement,
            "Expected `;` after render stage layout."_view)) {
      return StageResult();
    }
  }

  if (!cursor.require(
          Class::Type::ScopeEnd,
          "Expected `}` after render stage declaration."_view)) {
    return StageResult();
  }

  const Ttx::Type* stage_facts =
      build_facts(context, definition.get_name(), constants, pushes, resources);
  return StageResult(
      Ttx::Function(
          definition.get_name(), Ttx::Layout(parameters.get_view()),
          Ttx::Layout(result.get_view()), definition.get_documentation()),
      *stage_facts);
}

auto Render::Stage::insert(
    Cursor& cursor,
    Managed::Vector<Ttx::Function>& functions,
    Ttx::Function stage) -> Bool {
  if (stage.is_empty()) {
    return False;
  }

  for (Count i = 0; i < functions.get_size(); i++) {
    if (functions[i].get_name() == stage.get_name()) {
      cursor.token_error("Render stage name is already defined."_view);
      return False;
    }
  }

  functions.insert(stage);
  return True;
}

auto Render::Stage::consume_reads(
    Cursor& cursor,
    Base::Context&,
    View::Vector<const Ttx::Type*> facts,
    Managed::Vector<Ttx::Member>& constants,
    Managed::Vector<Ttx::Member>& pushes,
    Managed::Vector<Ttx::Member>& resources) -> Bool {
  const Token* kind = cursor.require(
      Class::Type::Addressable, "Expected render stage read kind."_view);
  if (kind == nullptr) {
    return False;
  }

  View::Bytes type_name = Interface::source_to_type_name(kind->get_text());
  if (type_name.is_empty()) {
    cursor.token_error("Expected render stage read kind."_view);
    return False;
  }

  const Ttx::Type* fact_type = Interface::find(facts, type_name);
  if (fact_type == nullptr) {
    cursor.token_error("Render stage read target could not be resolved."_view);
    return False;
  }

  Managed::Vector<Ttx::Member>* target = nullptr;
  switch (ReadKinds::find_or_default(type_name, ReadKind::Invalid)) {
  case ReadKind::Constant:
    target = &constants;
    break;
  case ReadKind::Push:
    target = &pushes;
    break;
  case ReadKind::Resource:
    target = &resources;
    break;
  default:
    cursor.token_error("Expected render stage read kind."_view);
    return False;
  }

  if (!cursor.require(
          Class::Type::IndexStart,
          "Expected `[` after render stage read kind."_view)) {
    return False;
  }

  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::IndexEnd)) {
    const Token* read_name = cursor.require(
        Class::Type::Addressable, "Expected render stage read name."_view);
    if (read_name == nullptr) {
      return False;
    }

    const Ttx::Member* source = fact_type->find_member(read_name->get_text());
    if (source == nullptr) {
      cursor.token_error("Render stage read fact could not be resolved."_view);
      return False;
    }

    if (!Interface::insert(
            cursor, *target,
            Ttx::Member(read_name->get_text(), source->get_type()),
            "Render stage read name is already defined."_view)) {
      return False;
    }

    if (cursor.matches(Class::Type::PackingOp)) {
      cursor.consume();
      continue;
    }

    if (!cursor.matches(Class::Type::IndexEnd)) {
      cursor.token_error("Expected `,` or `]` after render stage read."_view);
      return False;
    }
  }

  return cursor.require(
             Class::Type::IndexEnd,
             "Expected `]` after render stage reads."_view) != nullptr &&
         cursor.require(
             Class::Type::EndStatement,
             "Expected `;` after render stage reads declaration."_view) !=
             nullptr;
}

auto Render::Stage::build_facts(
    Base::Context& context,
    View::Bytes stage_name,
    Managed::Vector<Ttx::Member>& constants,
    Managed::Vector<Ttx::Member>& pushes,
    Managed::Vector<Ttx::Member>& resources) -> const Ttx::Type* {
  Managed::Vector<const Ttx::Type*> types(context.get_arena());
  if (!constants.is_empty()) {
    types.insert(&context.get_arena().construct<Ttx::Type>(
        "constant"_view, constants.get_view()));
  }

  if (!pushes.is_empty()) {
    types.insert(&context.get_arena().construct<Ttx::Type>(
        "push"_view, pushes.get_view()));
  }

  if (!resources.is_empty()) {
    types.insert(&context.get_arena().construct<Ttx::Type>(
        "resource"_view, resources.get_view()));
  }

  return &context.get_arena().construct<Ttx::Type>(
      stage_name, View::Vector<Ttx::Member>(), types.get_view());
}
