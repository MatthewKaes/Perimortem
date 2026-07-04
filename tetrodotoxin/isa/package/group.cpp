// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/package/group.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/boot/documentation.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Package::Group::evaluate(Context& context, Cursor& cursor)
    -> Package::Group {
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after package group declaration."_view)) {
    return Package::Group();
  }

  Managed::Vector<Package::Export> exports(cursor.get_arena());
  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    Ttx::Documentation documentation = Boot::Documentation::evaluate(cursor);
    Package::Export export_ =
        Package::Export::evaluate(context, cursor, documentation);
    if (!export_.is_valid()) {
      return Package::Group();
    }

    exports.insert(export_);
  }

  if (!cursor.require(
          Class::Type::ScopeEnd,
          "Expected `}` after package group declaration."_view)) {
    return Package::Group();
  }

  return Package::Group(exports.get_view());
}
