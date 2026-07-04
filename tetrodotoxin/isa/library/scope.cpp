// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/scope.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Library::Scope::define(const Ttx::Type& type) -> Bool {
  return context.define_type(type);
}

auto Library::Scope::resolve_type(Cursor& cursor) const -> const Ttx::Type* {
  return context.resolve_type(cursor);
}

auto Library::Scope::resolve_type(Cursor& cursor, View::Bytes root_name) const
    -> const Ttx::Type* {
  return context.resolve_type(cursor, root_name);
}
