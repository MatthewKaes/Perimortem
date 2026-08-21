// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/archive/reader.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/reader/binary.hpp"

#include "ttx/model/documentations/block.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Tetrodotoxin;

using BinaryReader = Reader::Binary<Data::ByteOrder::Little>;

auto Library::Archive::Reader::open(
    View::Bytes payload,
    Language::Persistence::Profile profile) -> Option<Reader> {
  BAIL_IF(payload.get_size() < 8);

  BinaryReader reader(payload.slice(0, 8));
  View::Bytes magic = reader.read_bytes(4);
  Unsigned_16 format = reader.read_unsigned_16();
  Unsigned_8 encoded_profile = reader.read_unsigned_8();
  Unsigned_8 flags = reader.read_unsigned_8();
  BAIL_IF(
      magic != "TTXL"_view || format != 1 ||
      encoded_profile != Unsigned_8(profile) || flags != 0);

  return Reader(payload.slice(8));
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
  Unsigned_16 tag = reader.read_unsigned_16();
  Unsigned_16 flags = reader.read_unsigned_16();
  Unsigned_32 size = reader.read_unsigned_32();
  BAIL_IF(flags > 1);

  auto record_payload = take(size);
  BAIL_IF(!record_payload);
  return Record(tag, flags == 1, *record_payload);
}

auto Library::Archive::Reader::read_unsigned_8() -> Option<Unsigned_8> {
  auto bytes = take(sizeof(Unsigned_8));
  return bytes ? Option<Unsigned_8>(BinaryReader(*bytes).read_unsigned_8())
               : Option<Unsigned_8>();
}

auto Library::Archive::Reader::read_unsigned_16() -> Option<Unsigned_16> {
  auto bytes = take(sizeof(Unsigned_16));
  return bytes ? Option<Unsigned_16>(BinaryReader(*bytes).read_unsigned_16())
               : Option<Unsigned_16>();
}

auto Library::Archive::Reader::read_unsigned_32() -> Option<Unsigned_32> {
  auto bytes = take(sizeof(Unsigned_32));
  return bytes ? Option<Unsigned_32>(BinaryReader(*bytes).read_unsigned_32())
               : Option<Unsigned_32>();
}

auto Library::Archive::Reader::read_unsigned_64() -> Option<Unsigned_64> {
  auto bytes = take(sizeof(Unsigned_64));
  return bytes ? Option<Unsigned_64>(BinaryReader(*bytes).read_unsigned_64())
               : Option<Unsigned_64>();
}

auto Library::Archive::Reader::read_signed_64() -> Option<Signed_64> {
  auto bytes = take(sizeof(Signed_64));
  return bytes ? Option<Signed_64>(BinaryReader(*bytes).read_signed_64())
               : Option<Signed_64>();
}

auto Library::Archive::Reader::read_real_64() -> Option<Real_64> {
  auto bytes = take(sizeof(Real_64));
  return bytes ? Option<Real_64>(BinaryReader(*bytes).read_real_64())
               : Option<Real_64>();
}

auto Library::Archive::Reader::read_bytes() -> Option<View::Bytes> {
  auto size = read_unsigned_32();
  BAIL_IF(!size);
  return take(*size);
}

auto Library::Archive::Reader::read_documentation(Allocator::Arena& arena)
    -> Option<const Documentation&> {
  auto count = read_unsigned_32();
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
