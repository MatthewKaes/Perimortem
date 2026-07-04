// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/virtual_machine.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/table.hpp"

#include "tetrodotoxin/isa/boot/documentation.hpp"
#include "tetrodotoxin/isa/library/addressable.hpp"
#include "tetrodotoxin/isa/library/alias.hpp"
#include "tetrodotoxin/isa/library/foreign.hpp"
#include "tetrodotoxin/isa/library/scope.hpp"
#include "tetrodotoxin/isa/library/structure.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

using DefinitionEvaluator =
    const Ttx::Type* (*)(Library::Scope & scope,
                         Cursor& cursor,
                         const Tetrodotoxin::Isa::Definition& definition);

constexpr Static::Vector<Pair<View::Bytes, DefinitionEvaluator>, 3>
    library_sub_isas = {{
      {
        Library::Alias::get_name(),
        Library::Alias::evaluate,
      },
      {
        Library::Structure::get_name(),
        Library::Structure::evaluate,
      },
      {
        Library::Foreign::get_name(),
        Library::Foreign::evaluate,
      },
    }};

auto Library::VirtualMachine::evaluate_definition(
    Library::Scope& scope,
    Cursor& cursor,
    Ttx::Documentation documentation,
    Managed::Vector<Ttx::Type::Member>& members,
    Managed::Vector<const Ttx::Type*>& types) -> Bool {
  Tetrodotoxin::Isa::Definition definition =
      Tetrodotoxin::Isa::Definition::evaluate(
          cursor, documentation,
          {{Class::Type::Public, Class::Type::Private, Class::Type::Expose}},
          {{Class::Type::Type, Class::Type::Addressable}},
          {{Class::Type::Addressable, Class::Type::Type, Class::Type::Alias,
            Class::Type::Func}});
  if (!definition.is_valid()) {
    return False;
  }

  if (definition.has_addressable_name()) {
    Ttx::Type::Member member =
        Library::Addressable::evaluate(scope, cursor, definition);
    if (member.is_empty()) {
      return False;
    }

    members.insert(member);
    return True;
  }

  const auto* handler =
      Table<DefinitionEvaluator, library_sub_isas>::find_or_null(
          definition.get_kind());
  if (handler == nullptr) {
    Managed::Bytes message(cursor.get_arena());
    message.append(
        "Definition name provided is not one of the known types {"_view);
    for (Count i = 0; i < library_sub_isas.get_size(); i++) {
      if (i != 0) {
        message.append(", "_view);
      }

      message.append(library_sub_isas[i].key);
    }
    message.append("}"_view);
    cursor.token_error(message.get_view());
    return False;
  }

  const Ttx::Type* type = (*handler)(scope, cursor, definition);
  if (type == nullptr) {
    return False;
  }

  if (!scope.define(*type)) {
    cursor.token_error("Library type name is already defined."_view);
    return False;
  }

  types.insert(type);
  return True;
}

auto Library::VirtualMachine::evaluate(Context& context, Cursor& cursor)
    -> Ttx::Type* {
  Managed::Vector<Ttx::Type::Member> members(context.get_arena());
  Managed::Vector<const Ttx::Type*> types(context.get_arena());
  Library::Scope scope(context);

  while (!cursor.matches(Class::Type::EndOfStream)) {
    Ttx::Documentation documentation = Boot::Documentation::evaluate(cursor);
    if (cursor.matches(Class::Type::EndOfStream)) {
      break;
    }

    if (!evaluate_definition(scope, cursor, documentation, members, types)) {
      return nullptr;
    }
  }

  auto& type = context.get_arena().construct<Ttx::Type>(
      Library::VirtualMachine::get_name(), members.get_view(), types.get_view(),
      View::Vector<Ttx::Type::Function>());
  return &type;
}
