// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/lexical/tokenizer.hpp"
#include "ttx/parse/cursor.hpp"
#include "ttx/source.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Validation;

static Harness TtxSource = {
  .name = "Ttx::Source"_view,
};

PERIMORTEM_UNIT_TEST(TtxSource, source_envelope) {
  Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(
      "// Package docs.\n"
      "// More docs.\n"
      "dialect : Library;\n"
      "import Graphics : Package = TTX::Graphics;\n"
      "import Types : Library = (.source = \"library/types.ttx\");\n"
      "@public func main[] -> [];\n"_view);

  Ttx::Parse::Cursor cursor(tokenizer);
  Ttx::Source source = Ttx::Source::parse(cursor, "unit.ttx"_view);

  EXPECT(cursor.get_errors().is_empty());
  EXPECT_TEXT(source.get_source_path(), "unit.ttx"_view);
  EXPECT_EQ(source.get_documentation().get_line_count(), Count(2));
  EXPECT_TEXT(source.get_documentation().line_at(0), "Package docs."_view);
  EXPECT_TEXT(source.get_documentation().line_at(1), "More docs."_view);

  EXPECT_TEXT(source.get_dialect_name().get_name(), "Library"_view);
  ASSERT_EQ(source.get_imports().get_size(), Count(2));

  const Ttx::Parse::Import& graphics = source.get_imports()[0];
  EXPECT_TEXT(graphics.get_local_name(), "Graphics"_view);
  EXPECT_TEXT(graphics.get_dialect_name().get_name(), "Package"_view);
  EXPECT(graphics.is_package_source());
  EXPECT_NOT(graphics.is_file_source());
  ASSERT_EQ(graphics.get_package_path().get_segment_count(), Count(2));
  EXPECT_TEXT(graphics.get_package_path().get_segments()[0], "TTX"_view);
  EXPECT_TEXT(graphics.get_package_path().get_segments()[1], "Graphics"_view);

  const Ttx::Parse::Import& types = source.get_imports()[1];
  EXPECT_TEXT(types.get_local_name(), "Types"_view);
  EXPECT_TEXT(types.get_dialect_name().get_name(), "Library"_view);
  EXPECT(types.is_file_source());
  EXPECT_NOT(types.is_package_source());
  EXPECT_TEXT(types.get_file_path(), "library/types.ttx"_view);

  EXPECT(cursor.matches(Ttx::Lexical::Class::Type::ConstPublic));
}

PERIMORTEM_UNIT_TEST(TtxSource, body_comments) {
  Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(
      "// Source docs.\n"
      "dialect : Library;\n"
      "// Body docs.\n"
      "@public func main[] -> [];\n"_view);

  Ttx::Parse::Cursor cursor(tokenizer);
  Ttx::Source source = Ttx::Source::parse(cursor);

  EXPECT(cursor.get_errors().is_empty());
  EXPECT_EQ(source.get_documentation().get_line_count(), Count(1));
  EXPECT_TEXT(source.get_documentation().line_at(0), "Source docs."_view);
  EXPECT(cursor.matches(Ttx::Lexical::Class::Type::Comment));
  EXPECT_TEXT(cursor.current().get_text(), "Body docs."_view);
}

PERIMORTEM_UNIT_TEST(TtxSource, package_directive) {
  Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(
      "dialect : Package;\n"
      "@package_name = TTX::Graphics;\n"_view);

  Ttx::Parse::Cursor cursor(tokenizer);
  Ttx::Source source = Ttx::Source::parse(cursor);

  EXPECT(cursor.get_errors().is_empty());
  EXPECT_TEXT(source.get_dialect_name().get_name(), "Package"_view);
  EXPECT(cursor.matches(Ttx::Lexical::Class::Type::Attribute));
  EXPECT_TEXT(cursor.current().get_text(), "@package_name"_view);
  cursor.consume();
  EXPECT(cursor.matches(Ttx::Lexical::Class::Type::Assign));
  cursor.consume();
  EXPECT(cursor.matches(Ttx::Lexical::Class::Type::Type));
}

PERIMORTEM_UNIT_TEST(TtxSource, bad_imports) {
  Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(
      "dialect : Library;\n"
      "import BadColon Library = TTX::Graphics;\n"
      "import BadField : Library = (.path = \"types.ttx\");\n"
      "import Graphics : Package = TTX::Graphics;\n"
      "@public func main[] -> [];\n"_view);

  Ttx::Parse::Cursor cursor(tokenizer);
  Ttx::Source source = Ttx::Source::parse(cursor);

  EXPECT_EQ(cursor.get_errors().get_size(), Count(2));
  ASSERT_EQ(source.get_imports().get_size(), Count(1));
  EXPECT_TEXT(source.get_imports()[0].get_local_name(), "Graphics"_view);
  EXPECT(cursor.matches(Ttx::Lexical::Class::Type::ConstPublic));
}

PERIMORTEM_UNIT_TEST(TtxSource, bad_dialect_header) {
  Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(
      "dialect : TTX.Graphics;\n"
      "import Graphics : Package = TTX::Graphics;\n"_view);

  Ttx::Parse::Cursor cursor(tokenizer);
  Ttx::Source source = Ttx::Source::parse(cursor);

  EXPECT(source.get_dialect_name().is_empty());
  EXPECT(source.get_imports().is_empty());
  EXPECT_EQ(cursor.get_errors().get_size(), Count(1));
}

PERIMORTEM_UNIT_TEST(TtxSource, missing_dialect) {
  Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse("import Graphics : Package = TTX::Graphics;\n"_view);

  Ttx::Parse::Cursor cursor(tokenizer);
  Ttx::Source source = Ttx::Source::parse(cursor);

  EXPECT(source.get_dialect_name().is_empty());
  EXPECT(source.get_imports().is_empty());
  EXPECT_EQ(cursor.get_errors().get_size(), Count(1));
  EXPECT(cursor.matches(Ttx::Lexical::Class::Type::Import));
}
