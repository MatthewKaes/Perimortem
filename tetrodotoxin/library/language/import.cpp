// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/import.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

auto Library::Language::Import::persist(Archive::Writer& writer) const -> Bool {
  auto record = writer.begin(Library::Archive::Tag::Import);
  BAIL_IF(
      !writer.write(documentation) || !type_reference.persist(writer) ||
      !writer.finish(record));
  return True;
}

auto Library::Language::Import::restore(
    Library::Archive::Reader& reader,
    Perimortem::Memory::Allocator::Arena& arena,
    const Ttx::Concept::Abstract& context) -> Option<Import> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Library::Archive::Tag::Import) ||
      record->is_optional());

  Library::Archive::Reader contents(record->get_payload());
  auto documentation = contents.read_documentation(arena);
  auto type_reference = TypeReference::restore(contents, arena, context);
  BAIL_IF(!documentation || !type_reference || !contents.is_complete());
  return Import(*documentation, *type_reference, {});
}
