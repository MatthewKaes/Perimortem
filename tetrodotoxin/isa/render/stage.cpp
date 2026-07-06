// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/render/stage.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/layout/evaluator.hpp"
#include "tetrodotoxin/isa/render/interface.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Render::Stage::evaluate(
    Cursor& cursor,
    Context& context,
    const Definition& definition,
    View::Vector<const Ttx::Type*> facts) -> Result {
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after render stage declaration."_view)) {
    return Result();
  }

  Managed::Vector<Ttx::Type::Member> parameters(context.get_arena());
  Managed::Vector<Ttx::Type::Member> result(context.get_arena());
  Managed::Vector<Ttx::Type::Member> constants(context.get_arena());
  Managed::Vector<Ttx::Type::Member> pushes(context.get_arena());
  Managed::Vector<Ttx::Type::Member> resources(context.get_arena());

  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    if (!cursor.matches(Class::Type::Addressable)) {
      cursor.token_error("Expected render stage directive."_view);
      return Result();
    }

    View::Bytes directive = cursor.consume().get_text();
    if (directive == "reads"_view) {
      if (!consume_reads(
              cursor, context, facts, constants, pushes, resources)) {
        return Result();
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
      return Result();
    }

    if (!Layout::Evaluator::evaluate_bracketed(cursor, context, *target)) {
      return Result();
    }

    if (!cursor.require(
            Class::Type::EndStatement,
            "Expected `;` after render stage layout."_view)) {
      return Result();
    }
  }

  if (!cursor.require(
          Class::Type::ScopeEnd,
          "Expected `}` after render stage declaration."_view)) {
    return Result();
  }

  const Ttx::Type* stage_facts =
      build_facts(context, definition.get_name(), constants, pushes, resources);
  return Result(
      Ttx::Type::Function(
          definition.get_name(), parameters.get_view(), result.get_view(),
          definition.get_documentation()),
      *stage_facts);
}

auto Render::Stage::insert(
    Cursor& cursor,
    Managed::Vector<Ttx::Type::Function>& functions,
    Ttx::Type::Function stage) -> Bool {
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
    Context&,
    View::Vector<const Ttx::Type*> facts,
    Managed::Vector<Ttx::Type::Member>& constants,
    Managed::Vector<Ttx::Type::Member>& pushes,
    Managed::Vector<Ttx::Type::Member>& resources) -> Bool {
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

  Managed::Vector<Ttx::Type::Member>* target = &resources;
  if (type_name == "constant"_view) {
    target = &constants;
  } else if (type_name == "push"_view) {
    target = &pushes;
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

    const Ttx::Type::Member* source =
        fact_type->find_member(read_name->get_text());
    if (source == nullptr || source->get_type() == nullptr) {
      cursor.token_error("Render stage read fact could not be resolved."_view);
      return False;
    }

    if (!Interface::insert(
            cursor, *target,
            Ttx::Type::Member(read_name->get_text(), *source->get_type()),
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
    Context& context,
    View::Bytes stage_name,
    Managed::Vector<Ttx::Type::Member>& constants,
    Managed::Vector<Ttx::Type::Member>& pushes,
    Managed::Vector<Ttx::Type::Member>& resources) -> const Ttx::Type* {
  Managed::Vector<const Ttx::Type*> types(context.get_arena());
  Managed::Vector<Ttx::Attribute> attributes(context.get_arena());
  attributes.insert({"isa"_view, "RenderStage"_view});

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
      stage_name, View::Vector<Ttx::Type::Member>(), types.get_view(),
      View::Vector<Ttx::Type::Function>(), Ttx::Documentation(),
      attributes.get_view());
}
