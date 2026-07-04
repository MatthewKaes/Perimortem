// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/structure.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/boot/documentation.hpp"
#include "tetrodotoxin/isa/library/addressable.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Library::Structure::evaluate(
    Library::Scope& scope,
    Cursor& cursor,
    const Tetrodotoxin::Isa::Definition& definition) -> const Ttx::Type* {
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after library struct declaration."_view)) {
    return nullptr;
  }

  Managed::Vector<Ttx::Type::Member> members(scope.get_context().get_arena());
  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    Ttx::Documentation member_documentation =
        Boot::Documentation::evaluate(cursor);

    Tetrodotoxin::Isa::Definition member_definition =
        Tetrodotoxin::Isa::Definition::evaluate(
            cursor, member_documentation,
            {{Class::Type::Public, Class::Type::Private,
              Class::Type::Expose}},
            {{Class::Type::Addressable}}, {{Class::Type::Type}});
    if (!member_definition.is_valid()) {
      return nullptr;
    }

    Ttx::Type::Member member =
        Library::Addressable::evaluate(scope, cursor, member_definition);
    if (member.is_empty()) {
      return nullptr;
    }

    members.insert(member);
  }

  if (!cursor.require(
          Class::Type::ScopeEnd,
          "Expected `}` after library struct declaration."_view)) {
    return nullptr;
  }

  auto& type = scope.get_context().get_arena().construct<Ttx::Type>(
      definition.get_name(), members.get_view(),
      View::Vector<const Ttx::Type*>(), View::Vector<Ttx::Type::Function>(),
      definition.get_documentation());
  return &type;
}
