// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/model/initialization.hpp"

#include "tetrodotoxin/library/language/model/type.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

static auto select(const Model::Type& target)
    -> Option<const Model::Initialization&> {
  return target.resolve_concept("initialization"_view)
      .select<Model::Initialization>();
}

auto Model::initialize_default(
    const Type& target,
    Perimortem::Memory::Allocator::Arena& arena) -> Option<Pack&> {
  auto initialization = select(target);
  return initialization ? initialization->create_default(arena)
                        : Option<Pack&>();
}

auto Model::initialize_supplied(
    const Type& target,
    Ttx::Lexical::Cursor& cursor,
    Pack& source,
    Option<const Ttx::Concept::Abstract&> access_scope,
    Option<Ttx::Lexical::Anchor> anchor) -> Option<Pack&> {
  auto initialization = select(target);
  if (!initialization) {
    cursor.create_expression_error(
        anchor, "Selected Domain does not provide Library initialization."_view,
        "Choose a value Domain that defines how supplied flow is constructed."_view);
    return {};
  }
  return initialization->create_supplied(cursor, source, access_scope, anchor);
}

auto Model::initialize_supplied_restored(
    const Type& target,
    Perimortem::Memory::Allocator::Arena& arena,
    Pack& source,
    Option<const Ttx::Concept::Abstract&> access_scope) -> Option<Pack&> {
  auto initialization = select(target);
  return initialization ? initialization->create_supplied_restored(
                              arena, source, access_scope)
                        : Option<Pack&>();
}
