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
  Managed::Vector<Base::Definition> member_definitions(context.get_arena());
  Managed::Vector<Ttx::Function> type_functions(context.get_arena());
  Managed::Vector<Ttx::Function> addressable_functions(context.get_arena());
  Managed::Vector<Base::Definition> function_definitions(context.get_arena());
  Managed::Vector<Base::Definition> addressable_function_definitions(
      context.get_arena());
  Ttx::Type* scene = context.get_arena().reserve<Ttx::Type>();
  Bool valid = True;
  while (!cursor.matches(Code::Type::Terminal)) {
    Ttx::Documentation documentation = Base::Documentation::evaluate(cursor);
    Bool attributes_consumed = Base::Attribute::consume_all(cursor);
    if (!attributes_consumed) {
      return nullptr;
    }

    if (cursor.matches(Code::Type::Terminal)) {
      break;
    }

    Code::Type current = cursor.current().get_code().get_type();
    if (Scene::Storage::is_modifier(current)) {
      Base::Definition definition;
      const Ttx::Member* member = Scene::Storage::evaluate(
          cursor, context, documentation, current, definition);
      if (member == nullptr) {
        return nullptr;
      }

      Bool inserted = Scene::Storage::insert(cursor, members, *member);
      if (!inserted) {
        valid = False;
        continue;
      }

      member_definitions.insert(definition);
      continue;
    }

    if (Scene::Function::is_modifier(current)) {
      Bool addressable = False;
      Ttx::Function function = Scene::Function::evaluate(
          cursor, context, documentation, scene, addressable);
      if (function.is_empty()) {
        return nullptr;
      }

      Managed::Vector<Ttx::Function>& functions =
          addressable ? addressable_functions : type_functions;
      Managed::Vector<Base::Definition>& definitions =
          addressable ? addressable_function_definitions : function_definitions;
      Bool inserted = Scene::Function::insert(cursor, functions, function);
      if (!inserted) {
        valid = False;
      } else {
        definitions.insert(Base::Definition(current));
      }

      continue;
    }

    if (cursor.matches(Code::Type::Addressable)) {
      Ttx::Function function =
          Scene::Lifecycle::evaluate(cursor, context, documentation, scene);
      if (function.is_empty()) {
        return nullptr;
      }

      Bool inserted =
          Scene::Function::insert(cursor, addressable_functions, function);
      if (!inserted) {
        valid = False;
      } else {
        addressable_function_definitions.insert(Base::Definition());
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

  for (Count i = 0; i < type_functions.get_size(); i++) {
    Bool implementation_defined = context.define_implementation(
        type_functions[i], function_definitions[i]);
    if (!implementation_defined) {
      return nullptr;
    }
  }

  for (Count i = 0; i < addressable_functions.get_size(); i++) {
    Bool implementation_defined = context.define_implementation(
        addressable_functions[i], addressable_function_definitions[i]);
    if (!implementation_defined) {
      return nullptr;
    }
  }

  new (scene) Ttx::Type(
      Scene::VirtualMachine::get_name(), members.get_view(),
      View::Vector<Ttx::Type::Reference>(), type_functions.get_view(),
      addressable_functions.get_view());
  for (Count i = 0; i < members.get_size(); i++) {
    Bool implementation_defined =
        context.define_implementation(members[i], member_definitions[i]);
    if (!implementation_defined) {
      return nullptr;
    }
  }

  return scene;
}
