// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/render/interface.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/table.hpp"

#include "tetrodotoxin/isa/base/attribute.hpp"
#include "tetrodotoxin/isa/base/declaration.hpp"
#include "tetrodotoxin/isa/base/documentation.hpp"
#include "tetrodotoxin/isa/base/expression/evaluator.hpp"
#include "tetrodotoxin/isa/base/expression/type.hpp"
#include "tetrodotoxin/isa/base/modifier.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

constexpr Static::Vector<Pair<View::Bytes, View::Bytes>, 6>
    render_interface_names = {{
      {"constants"_view, "constant"_view},
      {"constant"_view, "constant"_view},
      {"push_constants"_view, "push"_view},
      {"push"_view, "push"_view},
      {"resources"_view, "resource"_view},
      {"resource"_view, "resource"_view},
    }};

using RenderInterfaceNames = Table<View::Bytes, render_interface_names>;

auto Render::Interface::source_to_type_name(View::Bytes block_name)
    -> View::Bytes {
  return RenderInterfaceNames::find_or_default(block_name, View::Bytes());
}

auto Render::Interface::find(
    View::Vector<const Ttx::Type*> types,
    View::Bytes name) -> const Ttx::Type* {
  for (Count i = 0; i < types.get_size(); i++) {
    if (types[i] != nullptr && types[i]->get_name() == name) {
      return types[i];
    }
  }

  return nullptr;
}

auto Render::Interface::evaluate(
    Cursor& cursor,
    Base::Context& context,
    View::Bytes block_name) -> const Ttx::Type* {
  View::Bytes type_name = source_to_type_name(block_name);
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

  Managed::Vector<Ttx::Member> members(context.get_arena());
  Bool valid = True;
  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    Ttx::Documentation documentation = Base::Documentation::evaluate(cursor);
    Class::Type modifier = Base::Modifier::evaluate(
        cursor, {{Class::Type::Const, Class::Type::State}},
        "Expected render fact to start with `const` or `state`."_view);
    if (modifier == Class::Type::Unknown) {
      return nullptr;
    }

    Base::Declaration definition = Base::Declaration::evaluate_after_modifier(
        cursor, documentation, modifier, {{Class::Type::Addressable}},
        {{Class::Type::Type}});
    if (!definition.is_valid()) {
      return nullptr;
    }

    const Count error_count = cursor.get_errors().get_size();
    const Ttx::Type* type = Base::Expression::Type::evaluate(
        cursor, context, definition.get_kind());
    if (cursor.get_errors().get_size() != error_count) {
      return nullptr;
    }

    if (!Base::Expression::Evaluator::consume_initializer(
            cursor, "Expected `;` after render fact initializer."_view)) {
      return nullptr;
    }

    if (!cursor.require(
            Class::Type::EndStatement,
            "Expected `;` after render member."_view)) {
      return nullptr;
    }

    if (type == nullptr) {
      cursor.token_error("Render fact type could not be resolved."_view);
      return nullptr;
    }

    if (!insert(
            cursor, members,
            Ttx::Member(
                definition.get_name(), *type, definition.get_documentation()),
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

  return &context.get_arena().construct<Ttx::Type>(
      type_name, members.get_view());
}

auto Render::Interface::insert(
    Cursor& cursor,
    Managed::Vector<Ttx::Member>& members,
    Ttx::Member member,
    View::Bytes duplicate_error) -> Bool {
  for (Count i = 0; i < members.get_size(); i++) {
    if (members[i].get_name() == member.get_name()) {
      cursor.token_error(duplicate_error);
      return False;
    }
  }

  members.insert(member);
  return True;
}
