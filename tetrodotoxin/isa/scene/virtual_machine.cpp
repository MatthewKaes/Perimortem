// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/scene/virtual_machine.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/attribute.hpp"
#include "tetrodotoxin/isa/documentation.hpp"
#include "tetrodotoxin/isa/scene/function.hpp"
#include "tetrodotoxin/isa/scene/lifecycle.hpp"
#include "tetrodotoxin/isa/scene/storage.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Scene::VirtualMachine::evaluate(Context& context, Cursor& cursor)
    -> Ttx::Type* {
  Managed::Vector<Ttx::Type::Member> members(context.get_arena());
  Managed::Vector<Ttx::Type::Member> state(context.get_arena());
  Managed::Vector<Ttx::Type::Member> constants(context.get_arena());
  Managed::Vector<Ttx::Type::Function> functions(context.get_arena());
  Bool valid = True;

  while (!cursor.matches(Class::Type::EndOfStream)) {
    Ttx::Documentation documentation = Documentation::evaluate(cursor);
    if (!Attribute::consume_all(cursor)) {
      return nullptr;
    }
    if (cursor.matches(Class::Type::EndOfStream)) {
      break;
    }

    Class::Type current = cursor.current().get_class().get_type();
    if (Scene::Storage::is_modifier(current)) {
      Ttx::Type::Member member =
          Scene::Storage::evaluate(context, cursor, documentation, current);
      if (member.is_empty()) {
        return nullptr;
      }
      if (!Scene::Storage::insert(cursor, members, member)) {
        valid = False;
        continue;
      }

      Managed::Vector<Ttx::Type::Member>& storage_members =
          current == Class::Type::State ? state : constants;
      storage_members.insert(member);
      continue;
    }

    if (Scene::Function::is_modifier(current)) {
      Ttx::Type::Function function =
          Scene::Function::evaluate(context, cursor, documentation);
      if (function.is_empty()) {
        return nullptr;
      }
      if (!Scene::Function::insert(cursor, functions, function)) {
        valid = False;
      }
      continue;
    }

    if (cursor.matches(Class::Type::Addressable)) {
      Ttx::Type::Function function =
          Scene::Lifecycle::evaluate(context, cursor, documentation);
      if (function.is_empty()) {
        return nullptr;
      }
      if (!Scene::Function::insert(cursor, functions, function)) {
        valid = False;
      }
      continue;
    }

    cursor.token_error(
        "Expected Scene state, const, function, or lifecycle root."_view);
    return nullptr;
  }

  if (!valid) {
    return nullptr;
  }

  Managed::Vector<const Ttx::Type*> types(context.get_arena());
  if (state.get_size() != 0) {
    types.insert(Scene::Storage::build_fact_type(
        context, "state"_view, "state"_view, state.get_view()));
  }
  if (constants.get_size() != 0) {
    types.insert(Scene::Storage::build_fact_type(
        context, "const"_view, "const"_view, constants.get_view()));
  }

  Managed::Vector<Ttx::Attribute> attributes(context.get_arena());
  attributes.insert({"isa"_view, Scene::VirtualMachine::get_name()});
  return &context.get_arena().construct<Ttx::Type>(
      Scene::VirtualMachine::get_name(), members.get_view(), types.get_view(),
      functions.get_view(), Ttx::Documentation(), attributes.get_view());
}
