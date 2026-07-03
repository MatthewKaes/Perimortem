// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/dialect/package/namespace.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "ttx/dialect/source/documentation.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Dialect;
using namespace Ttx::Lexical;

auto Package::Namespace::parse(Cursor& cursor) -> Package::Namespace {
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after package namespace declaration."_view)) {
    return Package::Namespace();
  }

  Managed::Vector<Package::Export> exports(cursor.get_arena());
  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    Ttx::Documentation documentation = Source::Documentation::parse(cursor);
    Package::Export export_ = Package::Export::parse(cursor, documentation);
    if (!export_.is_valid()) {
      return Package::Namespace();
    }

    exports.insert(export_);
  }

  if (!cursor.require(
          Class::Type::ScopeEnd,
          "Expected `}` after package namespace declaration."_view)) {
    return Package::Namespace();
  }

  return Package::Namespace(exports.get_view());
}
