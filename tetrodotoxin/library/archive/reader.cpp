// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/archive/reader.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/reader/binary.hpp"

#include "tetrodotoxin/library/archive/source.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/model/documentations/block.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Tetrodotoxin;

using BinaryReader = Reader::Binary<Data::ByteOrder::Little>;

auto Library::Archive::Reader::open(
    View::Bytes payload,
    Tetrodotoxin::Language::Persistence::Profile profile) -> Option<Reader> {
  BAIL_IF(payload.get_size() < 8);

  BinaryReader reader(payload.slice(0, 8));
  View::Bytes magic = reader.read_bytes(4);
  U16 format = reader.read_u16();
  U8 encoded_profile = reader.read_u8();
  U8 flags = reader.read_u8();
  BAIL_IF(
      magic != "TTXL"_view || format != 1 || encoded_profile != U8(profile) ||
      flags != 0);

  return Reader(payload.slice(8));
}

auto Library::Archive::Reader::read(
    Allocator::Arena& arena,
    View::Bytes payload,
    Tetrodotoxin::Language::Persistence::Profile profile,
    const Abstract& language,
    Abstract& context) -> Option<Library::Language::Monograph&> {
  auto opened = open(payload, profile);
  BAIL_IF(!opened);

  auto record = opened->read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::Source) ||
      record->is_optional() || !opened->is_complete());

  Reader contents(record->get_payload());
  auto documentation = contents.read_documentation(arena);
  BAIL_IF(!documentation);
  auto& monograph = Library::Language::Monograph::create(
      arena, *documentation, Ttx::Lexical::Anchor::create(Ttx::Lexical::Span()),
      language, context);
  BAIL_IF(
      !read_source(contents, arena, monograph.get_source(), profile) ||
      !contents.is_complete());
  return monograph;
}

auto Library::Archive::Reader::take(Count size) -> Option<View::Bytes> {
  BAIL_IF(
      location > payload.get_size() || size > payload.get_size() - location);

  View::Bytes selected = payload.slice(location, size);
  location += size;
  return selected;
}

auto Library::Archive::Reader::read_record() -> Option<Record> {
  auto header = take(8);
  BAIL_IF(!header);

  BinaryReader reader(*header);
  U16 tag = reader.read_u16();
  U16 flags = reader.read_u16();
  U32 size = reader.read_u32();
  BAIL_IF(flags > 1);

  auto record_payload = take(size);
  BAIL_IF(!record_payload);
  return Record(tag, flags == 1, *record_payload);
}

auto Library::Archive::Reader::read_u8() -> Option<U8> {
  auto bytes = take(sizeof(U8));
  return bytes ? Option<U8>(BinaryReader(*bytes).read_u8()) : Option<U8>();
}

auto Library::Archive::Reader::read_u16() -> Option<U16> {
  auto bytes = take(sizeof(U16));
  return bytes ? Option<U16>(BinaryReader(*bytes).read_u16()) : Option<U16>();
}

auto Library::Archive::Reader::read_u32() -> Option<U32> {
  auto bytes = take(sizeof(U32));
  return bytes ? Option<U32>(BinaryReader(*bytes).read_u32()) : Option<U32>();
}

auto Library::Archive::Reader::read_u64() -> Option<U64> {
  auto bytes = take(sizeof(U64));
  return bytes ? Option<U64>(BinaryReader(*bytes).read_u64()) : Option<U64>();
}

auto Library::Archive::Reader::read_s64() -> Option<S64> {
  auto bytes = take(sizeof(S64));
  return bytes ? Option<S64>(BinaryReader(*bytes).read_s64()) : Option<S64>();
}

auto Library::Archive::Reader::read_r64() -> Option<R64> {
  auto bytes = take(sizeof(R64));
  return bytes ? Option<R64>(BinaryReader(*bytes).read_r64()) : Option<R64>();
}

auto Library::Archive::Reader::read_bytes() -> Option<View::Bytes> {
  auto size = read_u32();
  BAIL_IF(!size);
  return take(*size);
}

auto Library::Archive::Reader::read_documentation(Allocator::Arena& arena)
    -> Option<const Documentation&> {
  auto count = read_u32();
  BAIL_IF(!count || Count(*count) > payload.get_size());

  auto lines = arena.reserve<View::Bytes>(*count);
  for (Count index = 0; index < *count; index++) {
    auto line = read_bytes();
    BAIL_IF(!line);
    lines.get_data()[index] = arena.proxy(*line);
  }

  auto& documentation = arena.construct<Ttx::Model::Documentations::Block>(
      View::Vector<View::Bytes>(lines.get_data(), lines.get_size()));
  return documentation;
}
