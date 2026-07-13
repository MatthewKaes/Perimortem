// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/scene/virtual_machine.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/attribute.hpp"
#include "tetrodotoxin/isa/base/documentation.hpp"
#include "tetrodotoxin/isa/scene/function.hpp"
#include "tetrodotoxin/isa/scene/lifecycle.hpp"
#include "tetrodotoxin/isa/scene/storage.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Scene::VirtualMachine::evaluate(Cursor& cursor, Base::Context& context)
    -> Ttx::Type* {
  Managed::Vector<Ttx::Member> members(context.get_arena());
  Managed::Vector<Ttx::Member> state(context.get_arena());
  Managed::Vector<Ttx::Member> constants(context.get_arena());
  Managed::Vector<Ttx::Function> functions(context.get_arena());
  Managed::Vector<Base::Definition> function_definitions(context.get_arena());
  Bool valid = True;
  while (!cursor.matches(Class::Type::EndOfStream)) {
    Ttx::Documentation documentation = Base::Documentation::evaluate(cursor);
    if (!Base::Attribute::consume_all(cursor)) {
      return nullptr;
    }

    if (cursor.matches(Class::Type::EndOfStream)) {
      break;
    }

    Class::Type current = cursor.current().get_class().get_type();
    if (Scene::Storage::is_modifier(current)) {
      const Ttx::Member* member =
          Scene::Storage::evaluate(cursor, context, documentation, current);
      if (member == nullptr) {
        return nullptr;
      }

      if (!Scene::Storage::insert(cursor, members, *member)) {
        valid = False;
        continue;
      }

      Managed::Vector<Ttx::Member>& storage_members =
          current == Class::Type::State ? state : constants;
      storage_members.insert(*member);
      continue;
    }

    if (Scene::Function::is_modifier(current)) {
      Ttx::Function function =
          Scene::Function::evaluate(cursor, context, documentation);
      if (function.is_empty()) {
        return nullptr;
      }

      if (!Scene::Function::insert(cursor, functions, function)) {
        valid = False;
      } else {
        function_definitions.insert(Base::Definition(current));
      }

      continue;
    }

    if (cursor.matches(Class::Type::Addressable)) {
      Ttx::Function function =
          Scene::Lifecycle::evaluate(cursor, context, documentation);
      if (function.is_empty()) {
        return nullptr;
      }

      if (!Scene::Function::insert(cursor, functions, function)) {
        valid = False;
      } else {
        function_definitions.insert(Base::Definition());
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

  for (Count i = 0; i < functions.get_size(); i++) {
    if (!context.define_implementation(functions[i], function_definitions[i])) {
      return nullptr;
    }
  }

  Managed::Vector<const Ttx::Type*> types(context.get_arena());
  if (state.get_size() != 0) {
    types.insert(
        Scene::Storage::build_fact_type(
            context, "state"_view, state.get_view()));
  }

  if (constants.get_size() != 0) {
    types.insert(
        Scene::Storage::build_fact_type(
            context, "const"_view, constants.get_view()));
  }

  return &context.get_arena().construct<Ttx::Type>(
      Scene::VirtualMachine::get_name(), members.get_view(), types.get_view(),
      functions.get_view());
}
