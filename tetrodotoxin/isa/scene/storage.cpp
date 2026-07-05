// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/scene/storage.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/definition.hpp"
#include "tetrodotoxin/isa/expression.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Scene::Storage::evaluate(
    Context& context,
    Cursor& cursor,
    Ttx::Documentation documentation,
    Class::Type storage) -> Ttx::Type::Member {
  cursor.consume();
  Definition definition = Definition::evaluate_after_modifier(
      cursor, documentation, storage, {{Class::Type::Addressable}},
      {{Class::Type::Type}});
  if (!definition.is_valid()) {
    return Ttx::Type::Member();
  }

  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* type = context.resolve_type(cursor, definition.get_kind());
  if (cursor.get_errors().get_size() != error_count) {
    return Ttx::Type::Member();
  }

  if (!Expression::consume_initializer(
          cursor, "Expected `;` after Scene member initializer."_view)) {
    return Ttx::Type::Member();
  }

  if (!cursor.require(
          Class::Type::EndStatement, "Expected `;` after Scene member."_view)) {
    return Ttx::Type::Member();
  }

  if (type == nullptr) {
    cursor.token_error("Scene member type could not be resolved."_view);
    return Ttx::Type::Member();
  }

  return Ttx::Type::Member(
      definition.get_name(), *type, definition.get_documentation());
}

auto Scene::Storage::insert(
    Cursor& cursor,
    Managed::Vector<Ttx::Type::Member>& members,
    Ttx::Type::Member member) -> Bool {
  if (member.is_empty()) {
    return False;
  }

  for (Count i = 0; i < members.get_size(); i++) {
    if (members[i].get_name() == member.get_name()) {
      cursor.token_error("Scene member name is already defined."_view);
      return False;
    }
  }

  members.insert(member);
  return True;
}

auto Scene::Storage::build_fact_type(
    Context& context,
    View::Bytes type_name,
    View::Bytes block_name,
    View::Vector<Ttx::Type::Member> members) -> const Ttx::Type* {
  Managed::Vector<Ttx::Attribute> attributes(context.get_arena());
  attributes.insert({"isa"_view, "SceneFacts"_view});
  attributes.insert({"scene_block"_view, block_name});
  return &context.get_arena().construct<Ttx::Type>(
      type_name, members, View::Vector<const Ttx::Type*>(),
      View::Vector<Ttx::Type::Function>(), Ttx::Documentation(),
      attributes.get_view());
}
