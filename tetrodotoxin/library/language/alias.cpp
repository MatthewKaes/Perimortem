// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/alias.hpp"

#include "tetrodotoxin/library/archive/declaration.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/merged.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

auto Tetrodotoxin::Library::Language::Alias::persist(
    Archive::Writer& writer) const -> Bool {
  auto record = writer.begin(Archive::Tag::Alias);
  Archive::Declaration declaration(definition);
  BAIL_IF(
      !declaration.write(writer) || !target_reference.persist(writer) ||
      !writer.finish(record));
  return True;
}

auto Tetrodotoxin::Library::Language::Alias::restore(
    Archive::Reader& reader,
    Allocator::Arena& arena,
    Abstract& host) -> Option<Alias&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Archive::Tag::Alias) ||
      record->is_optional());

  Archive::Reader contents(record->get_payload());
  auto declaration = Archive::Declaration::read(contents, arena);
  auto target = TypeReference::restore(contents, arena, host);
  BAIL_IF(!declaration || !target || !contents.is_complete());

  auto& definition = declaration->create_definition(arena, host);
  return arena.construct_from<Alias>(
      [&]() -> Alias { return Alias(arena, definition, *target); });
}

auto Tetrodotoxin::Library::Language::Alias::interpret(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Option<Alias&> {
  if (definition.get_name_token().get_code() != Code::Type::Type) {
    cursor.create_token_error(
        definition.get_name_token(),
        "Library Alias definitions require a Type shaped name."_view);
    return {};
  }

  if (definition.get_visibility() >
      Tetrodotoxin::Language::Visibility::Exposed) {
    cursor.create_token_error(
        definition.get_visibility_token(),
        "Library Aliases accept only `expose` or `private` visibility."_view);
    return {};
  }

  if (!definition.get_modifiers().is_empty()) {
    cursor.create_token_error(
        definition.get_modifiers().get_data()[0],
        "Library Aliases do not accept evaluation modifiers."_view);
    return {};
  }

  Token alias_token = cursor.require(
      Code::Type::Alias,
      "Library Alias definitions require the `alias` qualifier."_view);
  BAIL_IF(!alias_token);
  BAIL_IF(!cursor.require(
      Code::Type::Assign,
      "Library Alias qualifiers require `=` before their Type route."_view));

  auto target_reference = TypeReference::parse(definition.get_host(), cursor);
  BAIL_IF(!target_reference);
  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Library Alias definitions require one terminating `;`."_view);
  BAIL_IF(!terminator);

  BAIL_IF(!definition.complete(alias_token, terminator));

  Allocator::Arena& domain = cursor.get_arena();
  Alias& alias = domain.construct_from<Alias>(
      [&]() -> Alias { return Alias(domain, definition, *target_reference); });
  return alias;
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

  auto target = selected->select<Model::Type>();
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

  if (selected && selected->is<Model::Type>()) {
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
