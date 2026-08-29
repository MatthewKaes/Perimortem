// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/archive/writer.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/binary.hpp"

#include "perimortem/serialization/stream/binary.hpp"

#include "tetrodotoxin/library/archive/composite.hpp"
#include "tetrodotoxin/library/archive/reference.hpp"
#include "tetrodotoxin/library/archive/source.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin;

enum class LibraryWriterAttributeValue : U8 {
  Empty,
  Bytes,
  Unsigned,
  Signed,
  Real,
  Flag,
};

Library::Archive::Writer::Writer() {
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes> appender(bytes);
  appender << "TTXL"_view;
  appender << U16(2);
  appender << U16(0);
}

auto Library::Archive::Writer::write(
    const Library::Language::Monograph& monograph) -> Option<Dynamic::Bytes> {
  BAIL_IF(!monograph.get_source().is_finalized());
  Writer writer;
  BAIL_IF(!Archive::write(writer, monograph.get_source()));
  return writer.take();
}

auto Library::Archive::Writer::encode_declarations(
    const Library::Language::Types::Composite& composite)
    -> Option<Dynamic::Bytes> {
  BAIL_IF(!composite.is_finalized());

  Writer writer;
  BAIL_IF(!Archive::write_declarations(writer, composite));
  return writer.take();
}

auto Library::Archive::Writer::encode_type_reference(
    const Library::Language::TypeReference& reference)
    -> Option<Dynamic::Bytes> {
  Writer writer;
  BAIL_IF(!Archive::write(writer, reference));
  return writer.take();
}

auto Library::Archive::Writer::begin(Tag tag, Bool optional) -> Record {
  Count offset = bytes.get_size();
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes> appender(bytes);
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

  Perimortem::Core::Writer::Binary<Data::ByteOrder::Little> patcher(
      bytes.get_access().slice(offset + 4, 4));
  patcher << U32(payload_size);
  return patcher.is_valid();
}

auto Library::Archive::Writer::write(U8 value) -> void {
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(bytes) << value;
}

auto Library::Archive::Writer::write(U16 value) -> void {
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(bytes) << value;
}

auto Library::Archive::Writer::write(U32 value) -> void {
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(bytes) << value;
}

auto Library::Archive::Writer::write(U64 value) -> void {
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(bytes) << value;
}

auto Library::Archive::Writer::write(S64 value) -> void {
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(bytes) << value;
}

auto Library::Archive::Writer::write(R64 value) -> void {
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(bytes) << value;
}

auto Library::Archive::Writer::write(View::Bytes value) -> Bool {
  BAIL_IF(value.get_size() > U32(-1));

  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes> appender(bytes);
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

auto Library::Archive::Writer::write(
    const Tetrodotoxin::Language::Attribute& attribute) -> Bool {
  BAIL_IF(!write(attribute.get_key()));
  return attribute.get_value().visit(
      [&]() -> Bool {
        write(U8(LibraryWriterAttributeValue::Empty));
        return True;
      },
      [&](View::Bytes selected) -> Bool {
        write(U8(LibraryWriterAttributeValue::Bytes));
        return write(selected);
      },
      [&](U64 selected) -> Bool {
        write(U8(LibraryWriterAttributeValue::Unsigned));
        write(selected);
        return True;
      },
      [&](S64 selected) -> Bool {
        write(U8(LibraryWriterAttributeValue::Signed));
        write(selected);
        return True;
      },
      [&](R64 selected) -> Bool {
        write(U8(LibraryWriterAttributeValue::Real));
        write(selected);
        return True;
      },
      [&](Bool selected) -> Bool {
        write(U8(LibraryWriterAttributeValue::Flag));
        write(U8(selected ? 1 : 0));
        return True;
      });
}

auto Library::Archive::Writer::take() -> Dynamic::Bytes {
  return Data::take(bytes);
}
