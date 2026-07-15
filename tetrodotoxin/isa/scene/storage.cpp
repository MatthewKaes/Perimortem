// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/scene/storage.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/declaration.hpp"
#include "tetrodotoxin/isa/base/expression/type.hpp"
#include "tetrodotoxin/isa/base/expression/value.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Scene::Storage::evaluate(
    Cursor& cursor,
    Base::Context& context,
    Ttx::Documentation documentation,
    Class::Type storage,
    Base::Definition& implementation) -> const Ttx::Member* {
  cursor.consume();

  Base::Declaration declaration = Base::Declaration::evaluate_after_modifier(
      cursor, documentation, storage, {{Class::Type::Addressable}},
      {{Class::Type::Type}});
  if (declaration.is_empty()) {
    return nullptr;
  }

  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* type =
      Base::Expression::Type::evaluate(cursor, context, declaration.get_kind());
  if (cursor.get_errors().get_size() != error_count) {
    return nullptr;
  }

  Base::Expression::Value initializer;
  if (cursor.matches(Class::Type::Assign)) {
    cursor.consume();

    initializer = Base::Expression::Value::evaluate(cursor, context);
    if (initializer.is_empty()) {
      return nullptr;
    }
  }

  Bool has_statement_end = cursor.require(
      Class::Type::EndStatement, "Expected `;` after Scene member."_view);
  if (!has_statement_end) {
    return nullptr;
  }

  if (type == nullptr) {
    cursor.token_error("Scene member type could not be resolved."_view);
    return nullptr;
  }

  implementation = Base::Definition(storage, {}, initializer);
  return &context.get_arena().construct<Ttx::Member>(
      declaration.get_name(), *type, declaration.get_documentation());
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
