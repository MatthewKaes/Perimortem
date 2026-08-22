// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/archive/writer.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/binary.hpp"

#include "perimortem/serialization/stream/binary.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin;

using Appender = Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>;
using Patcher = Perimortem::Core::Writer::Binary<Data::ByteOrder::Little>;

Library::Archive::Writer::Writer(Language::Persistence::Profile profile)
    : profile(profile) {
  Appender appender(bytes);
  appender << "TTXL"_view;
  appender << U16(1);
  appender << U8(profile);
  appender << U8(0);
}

auto Library::Archive::Writer::begin(Tag tag, Bool optional) -> Record {
  Count offset = bytes.get_size();
  Appender appender(bytes);
  appender << U16(tag);
  appender << U16(optional ? 1 : 0);
  appender << U32(0);
  return Record(offset);
}

auto Library::Archive::Writer::finish(Record record) -> Bool {
  Count offset = record.get_offset();
  BAIL_IF(offset > bytes.get_size() || bytes.get_size() - offset < 8);

  Count payload_size = bytes.get_size() - offset - 8;
  BAIL_IF(payload_size > U32(-1));

  Patcher patcher(bytes.get_access().slice(offset + 4, 4));
  patcher << U32(payload_size);
  return patcher.is_valid();
}

auto Library::Archive::Writer::write(U8 value) -> void {
  Appender(bytes) << value;
}

auto Library::Archive::Writer::write(U16 value) -> void {
  Appender(bytes) << value;
}

auto Library::Archive::Writer::write(U32 value) -> void {
  Appender(bytes) << value;
}

auto Library::Archive::Writer::write(U64 value) -> void {
  Appender(bytes) << value;
}

auto Library::Archive::Writer::write(S64 value) -> void {
  Appender(bytes) << value;
}

auto Library::Archive::Writer::write(R64 value) -> void {
  Appender(bytes) << value;
}

auto Library::Archive::Writer::write(View::Bytes value) -> Bool {
  BAIL_IF(value.get_size() > U32(-1));

  Appender appender(bytes);
  appender << U32(value.get_size());
  appender << value;
  return True;
}

auto Library::Archive::Writer::write(const Ttx::Concept::Documentation& value)
    -> Bool {
  BAIL_IF(value.line_count() > U32(-1));

  write(U32(value.line_count()));
  for (Count index = 0; index < value.line_count(); index++) {
    BAIL_IF(!write(value.get_line(index)));
  }
  return True;
}

auto Library::Archive::Writer::take() -> Dynamic::Bytes {
  return Data::take(bytes);
}
