// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/alias.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/merged.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

auto Tetrodotoxin::Library::Language::Alias::create_authored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    TypeReference target_reference) -> Alias& {
  return create(domain, definition, target_reference);
}

auto Tetrodotoxin::Library::Language::Alias::create(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    TypeReference target_reference) -> Alias& {
  return domain.construct_from<Alias>(
      [&]() -> Alias { return Alias(domain, definition, target_reference); });
}

auto Alias::link() -> Bool {
  if (linked) {
    return True;
  }
  Option<const Abstract&> selected;
  target_reference.resolve_lexical(definition.get_host())
      .visit(
          [&](const Abstract& resolved) { selected = resolved; },
          [](const TypeReference::Failure&) {});
  BAIL_IF(!selected);

  auto target = selected->select<Ttx::Model::Type>();
  BAIL_IF(!target);

  if (!bind_target(*target)) {
    return False;
  }

  // Alias never copies or exposes its target. Documentation is the one local
  // fact it can extend, so the merged view preserves both authored explanations
  // while every semantic query still observes only resolve().
  const Documentation& local = get_definition().get_documentation();
  if (local.is_empty()) {
    documentation = target->get_documentation();
  } else {
    documentation = domain.construct<Ttx::Model::Documentations::Merged>(
        local, target->get_documentation());
  }

  linked = True;
  return True;
}

auto Alias::report_unresolved(Cursor& cursor) const -> void {
  Option<TypeReference::Failure> failure;
  Option<const Abstract&> selected;
  target_reference.resolve_lexical(definition.get_host())
      .visit(
          [&](const Abstract& resolved) { selected = resolved; },
          [&](const TypeReference::Failure& rejected) { failure = rejected; });
  if (failure) {
    target_reference.report(cursor, *failure);
    return;
  }

  if (selected && selected->is<Ttx::Model::Type>()) {
    return;
  }
  cursor.create_expression_error(
      target_reference.get_anchor(),
      "Library Alias Type reference did not resolve to one Type."_view,
      "Publish the selected Type and remove any Alias cycle before linking."_view);
}

auto Alias::get_documentation() const -> const Documentation& {
  return documentation.visit(
      [&]() -> const Documentation& {
        return get_definition().get_documentation();
      },
      [](const Documentation& selected) -> const Documentation& {
        return selected;
      });
}
