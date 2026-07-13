// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/scene/storage.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/declaration.hpp"
#include "tetrodotoxin/isa/base/expression/evaluator.hpp"
#include "tetrodotoxin/isa/base/expression/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Scene::Storage::evaluate(
    Cursor& cursor,
    Base::Context& context,
    Ttx::Documentation documentation,
    Class::Type storage) -> const Ttx::Member* {
  cursor.consume();
  Base::Declaration definition = Base::Declaration::evaluate_after_modifier(
      cursor, documentation, storage, {{Class::Type::Addressable}},
      {{Class::Type::Type}});
  if (!definition.is_valid()) {
    return nullptr;
  }

  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* type =
      Base::Expression::Type::evaluate(cursor, context, definition.get_kind());
  if (cursor.get_errors().get_size() != error_count) {
    return nullptr;
  }

  if (!Base::Expression::Evaluator::consume_initializer(
          cursor, "Expected `;` after Scene member initializer."_view)) {
    return nullptr;
  }

  if (!cursor.require(
          Class::Type::EndStatement, "Expected `;` after Scene member."_view)) {
    return nullptr;
  }

  if (type == nullptr) {
    cursor.token_error("Scene member type could not be resolved."_view);
    return nullptr;
  }

  return &context.get_arena().construct<Ttx::Member>(
      definition.get_name(), *type, definition.get_documentation());
}

auto Scene::Storage::insert(
    Cursor& cursor,
    Managed::Vector<Ttx::Member>& members,
    Ttx::Member member) -> Bool {
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
    Base::Context& context,
    View::Bytes type_name,
    View::Vector<Ttx::Member> members) -> const Ttx::Type* {
  return &context.get_arena().construct<Ttx::Type>(type_name, members);
}
