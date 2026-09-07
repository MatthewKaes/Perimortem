// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/monograph.hpp"

using namespace Tetrodotoxin::Language;
using namespace Perimortem;

Monograph::Monograph(
    Memory::Allocator::Arena& arena,
    ttx_abstract language,
    const Ttx::Concept::Documentation& documentation,
    ttx_abstract context)
    : arena(arena),
      context(context),
      language(language),
      documentation(documentation),
      imports(arena) {}

auto Monograph::get_layer(ttx_abstract requested) const -> ttx_abstract {
  return ttx_abstract_same(requested, language) ? get_abi() : ttx_none();
}

auto Monograph::retain_import(
    const Import::Description& description,
    Core::Option<Ttx::Lexical::Associations&> associations) -> Bool {
  for (const auto* retained : imports.get_view()) {
    if (retained->get_name() == description.get_name()) {
      return False;
    }
  }
  auto& imported = arena.construct<Import>(arena, description);
  imports.insert(&imported);
  if (associations) {
    associations->create(
        description.get_declaration_anchor(), imported.get_abi());
    associations->create(
        description.get_expression_anchor(), imported.get_abi());
  }
  return True;
}

auto Monograph::resolve_concept(ttx_borrowed_bytes route) const
    -> ttx_abstract {
  const Core::View::Bytes name(route.data, route.size);
  for (const auto* imported : imports.get_view()) {
    if (imported->get_name() == name) {
      return imported->get_visibility() == Visibility::Private
                 ? ttx_unknown()
                 : imported->get_abi();
    }
  }
  return Ttx::resolve_concept(context, route);
}

void Monograph::visit_concepts(ttx_concept_sink result) const {
  for (const auto* imported : imports.get_view()) {
    if (imported->get_visibility() != Visibility::Private) {
      const auto name = imported->get_name();
      result.operations->item(
          result, {name.get_data(), name.get_size()}, imported->get_abi());
    }
  }
  result.operations->completed(result);
}

auto Monograph::layout() const -> ttx_layout {
  return ttx_empty_layout();
}

auto Monograph::validate(Ttx::Lexical::Cursor& cursor) const -> Bool {
  Bool complete = True;
  for (const auto* imported : imports.get_view()) {
    complete &= imported->validate(cursor);
  }
  return complete;
}
