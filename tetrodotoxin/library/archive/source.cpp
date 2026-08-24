// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/archive/source.hpp"

#include "tetrodotoxin/library/archive/composite.hpp"
#include "tetrodotoxin/library/archive/foreign.hpp"
#include "tetrodotoxin/library/archive/reference.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

auto Archive::write(Writer& writer, const Language::Import& import) -> Bool {
  auto record = writer.begin(Tag::Import);
  return writer.write(import.get_documentation()) &&
         Archive::write(writer, import.get_type_reference()) &&
         writer.finish(record);
}

auto Archive::read_import(
    Reader& reader,
    Allocator::Arena& arena,
    const Abstract& context) -> Option<Language::Import> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::Import) ||
      record->is_optional());

  Reader contents(record->get_payload());
  auto documentation = contents.read_documentation(arena);
  auto type_reference = read_type_reference(contents, arena, context);
  BAIL_IF(!documentation || !type_reference || !contents.is_complete());
  return Language::Import(
      *documentation, *type_reference, Ttx::Lexical::Span());
}

auto Archive::write(Writer& writer, const Language::Types::Source& source)
    -> Bool {
  auto record = writer.begin(Tag::Source);
  auto imports = source.get_imports();
  BAIL_IF(
      !writer.write(source.get_documentation()) ||
      imports.get_size() > U32(-1));

  writer.write(U32(imports.get_size()));
  for (const Language::Import& import : imports) {
    BAIL_IF(!Archive::write(writer, import));
  }

  const Language::Foreign& foreign = source.get_foreign();
  writer.write(U8(foreign.is_authored() ? 1 : 0));
  BAIL_IF(foreign.is_authored() && !Archive::write(writer, foreign));

  Bool public_only = writer.get_profile() ==
                     Tetrodotoxin::Language::Persistence::Profile::Contract;
  return write_declarations(writer, source, public_only) &&
         writer.finish(record);
}

auto Archive::read_source(
    Reader& reader,
    Allocator::Arena& arena,
    Language::Types::Source& source,
    Tetrodotoxin::Language::Persistence::Profile profile) -> Bool {
  auto import_count = reader.read_u32();
  BAIL_IF(!import_count || Count(*import_count) > reader.get_remaining_size());
  for (Count index = 0; index < *import_count; index++) {
    auto import = read_import(reader, arena, source.get_host());
    BAIL_IF(!import || !source.retain_import_route(*import));
  }

  auto has_foreign = reader.read_u8();
  BAIL_IF(!has_foreign || *has_foreign > 1);
  BAIL_IF(
      *has_foreign == 1 && !read_foreign(reader, arena, source.get_foreign()));
  return read_declarations(reader, arena, source, profile);
}
