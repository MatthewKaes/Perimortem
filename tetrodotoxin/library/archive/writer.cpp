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
  appender << Unsigned_16(1);
  appender << Unsigned_8(profile);
  appender << Unsigned_8(0);
}

auto Library::Archive::Writer::begin(Tag tag, Bool optional) -> Record {
  Count offset = bytes.get_size();
  Appender appender(bytes);
  appender << Unsigned_16(tag);
  appender << Unsigned_16(optional ? 1 : 0);
  appender << Unsigned_32(0);
  return Record(offset);
}

auto Library::Archive::Writer::finish(Record record) -> Bool {
  Count offset = record.get_offset();
  BAIL_IF(offset > bytes.get_size() || bytes.get_size() - offset < 8);

  Count payload_size = bytes.get_size() - offset - 8;
  BAIL_IF(payload_size > Unsigned_32(-1));

  Patcher patcher(bytes.get_access().slice(offset + 4, 4));
  patcher << Unsigned_32(payload_size);
  return patcher.is_valid();
}

auto Library::Archive::Writer::write(Unsigned_8 value) -> void {
  Appender(bytes) << value;
}

auto Library::Archive::Writer::write(Unsigned_16 value) -> void {
  Appender(bytes) << value;
}

auto Library::Archive::Writer::write(Unsigned_32 value) -> void {
  Appender(bytes) << value;
}

auto Library::Archive::Writer::write(Unsigned_64 value) -> void {
  Appender(bytes) << value;
}

auto Library::Archive::Writer::write(Signed_64 value) -> void {
  Appender(bytes) << value;
}

auto Library::Archive::Writer::write(Real_64 value) -> void {
  Appender(bytes) << value;
}

auto Library::Archive::Writer::write(View::Bytes value) -> Bool {
  BAIL_IF(value.get_size() > Unsigned_32(-1));

  Appender appender(bytes);
  appender << Unsigned_32(value.get_size());
  appender << value;
  return True;
}

auto Library::Archive::Writer::write(const Ttx::Concept::Documentation& value)
    -> Bool {
  BAIL_IF(value.line_count() > Unsigned_32(-1));

  write(Unsigned_32(value.line_count()));
  for (Count index = 0; index < value.line_count(); index++) {
    BAIL_IF(!write(value.get_line(index)));
  }
  return True;
}

auto Library::Archive::Writer::take() -> Dynamic::Bytes {
  return Data::take(bytes);
}
