// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/serialization/escaped_text.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/allocator/arena.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Validation;

static Harness SerializationEscapedText = {
  .name = "Serialization::EscapedText"_view,
};

PERIMORTEM_UNIT_TEST(SerializationEscapedText, unescaped_views) {
  Allocator::Arena arena;
  View::Bytes source = "plain source text"_view;
  View::Bytes decoded = EscapedText::decode(arena, source);

  EXPECT_TEXT(decoded, source);
  EXPECT(decoded.get_data() == source.get_data());
}

PERIMORTEM_UNIT_TEST(SerializationEscapedText, common_sequences) {
  Allocator::Arena arena;
  View::Bytes decoded =
      EscapedText::decode(arena, "line\\n\\\"title\\\"\\\\end"_view);

  EXPECT_TEXT(decoded, "line\n\"title\"\\end"_view);
}

PERIMORTEM_UNIT_TEST(SerializationEscapedText, unicode_sequences) {
  Allocator::Arena arena;
  View::Bytes decoded =
      EscapedText::decode(arena, "\\u0041\\u0020\\u0042"_view);

  EXPECT_TEXT(decoded, "A B"_view);
}

PERIMORTEM_UNIT_TEST(SerializationEscapedText, json_payloads) {
  Static::Bytes<20> encoded;
  Writer::Textual output(encoded.get_access());
  EscapedText::encode(output, "line\n\"title\"\\end"_view);

  View::Bytes output_view = output;
  EXPECT_TEXT(output_view, "line\\n\\\"title\\\"\\\\end"_view);
  EXPECT_EQ(EscapedText::encoded_size("line\n\"title\"\\end"_view), 20);
}
