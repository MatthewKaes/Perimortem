// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/package/group.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/documentation.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Package::Group::evaluate(
    Cursor& cursor,
    Base::Context& context,
    Managed::Vector<Package::Export>& exports) -> Bool {
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after package group declaration."_view)) {
    return False;
  }

  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    Ttx::Documentation documentation = Base::Documentation::evaluate(cursor);
    Package::Export export_ =
        Package::Export::evaluate(cursor, context, documentation);
    if (!export_.is_valid()) {
      return False;
    }

    if (Package::Export::contains_name(
            exports.get_view(), export_.get_definition().get_name())) {
      cursor.token_error("Package export name is already defined."_view);
      return False;
    }

    exports.insert(export_);
  }

  if (!cursor.require(
          Class::Type::ScopeEnd,
          "Expected `}` after package group declaration."_view)) {
    return False;
  }

  return True;
}
