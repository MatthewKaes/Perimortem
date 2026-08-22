// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/lsp/position_encoding.hpp"

#include "validation/unit_test.hpp"

using namespace Perimortem::Core;
using namespace Puffer::Lsp;
using namespace Validation;

static Harness PufferLspPositionEncoding = {
  .name = "Puffer::Lsp::PositionEncoding"_view,
};

static constexpr U8 utf_8_source[] = {
  'a', 0xF0, 0x9F, 0x98, 0x80, 'b', '\n', 0xC3, 0xA9, 0xE2, 0x82, 0xAC, 'z',
};

static constexpr View::Bytes source(utf_8_source, sizeof(utf_8_source));

static auto matches_position(
    const PositionEncoding& encoding,
    Count offset,
    Count line,
    Count character) -> Bool {
  auto position = encoding.locate(source, offset);
  BAIL_IF(
      !position || position->get_line() != line ||
      position->get_character() != character);
  auto restored = encoding.find_offset(source, *position);
  return Bool(restored && *restored == offset);
}

PERIMORTEM_UNIT_TEST(PufferLspPositionEncoding, utf_8_uses_byte_coordinates) {
  PositionEncoding encoding(PositionEncoding::Kind::Utf8);
  EXPECT(encoding.get_name() == "utf-8"_view);
  EXPECT(matches_position(encoding, 0, 0, 0));
  EXPECT(matches_position(encoding, 1, 0, 1));
  EXPECT(matches_position(encoding, 5, 0, 5));
  EXPECT(matches_position(encoding, 6, 0, 6));
  EXPECT(matches_position(encoding, 7, 1, 0));
  EXPECT(matches_position(encoding, 9, 1, 2));
  EXPECT(matches_position(encoding, 12, 1, 5));
  EXPECT(matches_position(encoding, 13, 1, 6));
}

PERIMORTEM_UNIT_TEST(PufferLspPositionEncoding, utf_16_uses_code_units) {
  PositionEncoding encoding;
  EXPECT(encoding.get_name() == "utf-16"_view);
  EXPECT(matches_position(encoding, 0, 0, 0));
  EXPECT(matches_position(encoding, 1, 0, 1));
  EXPECT(matches_position(encoding, 5, 0, 3));
  EXPECT(matches_position(encoding, 6, 0, 4));
  EXPECT(matches_position(encoding, 7, 1, 0));
  EXPECT(matches_position(encoding, 9, 1, 1));
  EXPECT(matches_position(encoding, 12, 1, 2));
  EXPECT(matches_position(encoding, 13, 1, 3));
}

PERIMORTEM_UNIT_TEST(PufferLspPositionEncoding, rejects_partial_codepoints) {
  PositionEncoding utf_8(PositionEncoding::Kind::Utf8);
  PositionEncoding utf_16;
  EXPECT(!utf_8.locate(source, 2));
  EXPECT(!utf_8.locate(source, 3));
  EXPECT(!utf_8.locate(source, 4));
  EXPECT(!utf_8.locate(source, 8));
  EXPECT(!utf_8.locate(source, 10));
  EXPECT(!utf_8.locate(source, 11));
  EXPECT(!utf_16.locate(source, 2));
  EXPECT(!utf_16.locate(source, 3));
  EXPECT(!utf_16.locate(source, 4));
  EXPECT(!utf_16.locate(source, 8));
  EXPECT(!utf_16.locate(source, 10));
  EXPECT(!utf_16.locate(source, 11));
  EXPECT(!utf_8.find_offset(source, PositionEncoding::Position(0, 2)));
  EXPECT(!utf_16.find_offset(source, PositionEncoding::Position(0, 2)));
  EXPECT(!utf_16.find_offset(source, PositionEncoding::Position(2, 0)));
}
