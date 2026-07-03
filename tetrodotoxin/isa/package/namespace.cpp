// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/package/namespace.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/boot/documentation.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Namespace::evaluate(Cursor& cursor) -> Namespace {
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after package namespace declaration."_view)) {
    return Namespace();
  }

  Managed::Vector<Export> exports(cursor.get_arena());
  while (!cursor.matches(Class::Type::EndOfStream) &&
         !cursor.matches(Class::Type::ScopeEnd)) {
    Ttx::Documentation documentation = Documentation::evaluate(cursor);
    Export export_ = Export::evaluate(cursor, documentation);
    if (!export_.is_valid()) {
      return Namespace();
    }

    exports.insert(export_);
  }

  if (!cursor.require(
          Class::Type::ScopeEnd,
          "Expected `}` after package namespace declaration."_view)) {
    return Namespace();
  }

  return Namespace(exports.get_view());
}
