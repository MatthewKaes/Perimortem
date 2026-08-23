// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/archive/foreign.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/archive/declaration.hpp"
#include "tetrodotoxin/library/archive/reference.hpp"
#include "ttx/concept/reference.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

auto Archive::write(Writer& writer, const Language::Foreign::State& state)
    -> Bool {
  auto record = writer.begin(Tag::ForeignState);
  Declaration declaration(state.get_definition());
  return declaration.write(writer) && writer.write(state.get_abi()) &&
         Archive::write(writer, state.get_type_reference()) &&
         writer.finish(record);
}

auto Archive::read_foreign_state(
    Reader& reader,
    Allocator::Arena& arena,
    Language::Foreign& host) -> Option<Language::Foreign::State&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::ForeignState) ||
      record->is_optional());

  Reader contents(record->get_payload());
  auto declaration = Declaration::read(contents, arena);
  auto abi = contents.read_bytes();
  auto type = read_type_reference(contents, arena, host);
  BAIL_IF(
      !declaration || !abi || abi->is_empty() || !type ||
      !contents.is_complete());

  auto& definition = declaration->create_definition(arena, host);
  return Language::Foreign::State::create(
      arena, definition, *type, arena.proxy(*abi));
}

auto Archive::write(Writer& writer, const Language::Foreign::Function& function)
    -> Bool {
  auto record = writer.begin(Tag::ForeignFunction);
  Declaration declaration(function.get_definition());
  return declaration.write(writer) && writer.write(function.get_abi()) &&
         Archive::write(writer, function.get_signature()) &&
         writer.finish(record);
}

auto Archive::read_foreign_function(
    Reader& reader,
    Allocator::Arena& arena,
    Language::Foreign& host) -> Option<Language::Foreign::Function&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::ForeignFunction) ||
      record->is_optional());

  Reader contents(record->get_payload());
  auto declaration = Declaration::read(contents, arena);
  auto abi = contents.read_bytes();
  auto signature = read_signature(contents, arena, host);
  BAIL_IF(
      !declaration || !abi || abi->is_empty() || !signature ||
      !contents.is_complete());

  auto& definition = declaration->create_definition(arena, host);
  return Language::Foreign::Function::create(
      arena, definition, *signature, arena.proxy(*abi));
}

auto Archive::write(Writer& writer, const Language::Foreign& foreign) -> Bool {
  auto record = writer.begin(Tag::Foreign);
  auto abi = foreign.get_abi();
  auto declarations = foreign.get_declarations();
  BAIL_IF(
      !writer.write(foreign.get_documentation()) || !abi ||
      !writer.write(*abi) || declarations.get_size() > U32(-1));

  writer.write(U32(declarations.get_size()));
  for (const Reference<Abstract>& declaration : declarations) {
    auto state = declaration.get().select<Language::Foreign::State>();
    if (state) {
      BAIL_IF(!Archive::write(writer, *state));
      continue;
    }
    auto function = declaration.get().select<Language::Foreign::Function>();
    BAIL_IF(!function || !Archive::write(writer, *function));
  }
  return writer.finish(record);
}

auto Archive::read_foreign(
    Reader& reader,
    Allocator::Arena& arena,
    Language::Foreign& foreign) -> Bool {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::Foreign) ||
      record->is_optional());

  Reader contents(record->get_payload());
  auto documentation = contents.read_documentation(arena);
  auto abi = contents.read_bytes();
  auto count = contents.read_u32();
  BAIL_IF(
      !documentation || !abi || abi->is_empty() || !count ||
      Count(*count) > record->get_payload().get_size());

  Managed::Vector<Reference<Language::Foreign::State>> states(arena);
  Managed::Vector<Reference<Language::Foreign::Function>> functions(arena);
  Managed::Vector<Reference<Abstract>> declarations(arena);
  for (Count index = 0; index < *count; index++) {
    Reader probe = contents;
    auto declaration = probe.read_record();
    BAIL_IF(!declaration || declaration->is_optional());

    switch (Tag(declaration->get_tag())) {
    case Tag::ForeignState: {
      auto state = read_foreign_state(contents, arena, foreign);
      BAIL_IF(!state || state->get_abi() != *abi);
      states.insert(*state);
      declarations.insert(*state);
      break;
    }
    case Tag::ForeignFunction: {
      auto function = read_foreign_function(contents, arena, foreign);
      BAIL_IF(!function || function->get_abi() != *abi);
      functions.insert(*function);
      declarations.insert(*function);
      break;
    }
    default:
      return False;
    }
  }
  BAIL_IF(!contents.is_complete());
  return foreign.retain_block(
      *documentation, arena.proxy(*abi), states.get_view(),
      functions.get_view(), declarations.get_view());
}
