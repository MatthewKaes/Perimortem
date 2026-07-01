// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/dialect/dialect.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/parse/cursor.hpp"
#include "ttx/parse/source.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Validation;

static Harness TtxParse = {
  .name = "Ttx::Parse"_view,
};

PERIMORTEM_UNIT_TEST(TtxParse, access_operators) {
  Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(
      "TTX::Graphics color.[r, g] color:[start, 2] layout[Type] value.member"_view);

  View::Vector<Ttx::Lexical::Token> tokens = tokenizer.get_tokens();
  ASSERT_EQ(tokens.get_size(), Count(23));

  EXPECT(tokens[0].get_class() == Ttx::Lexical::Class::Type::Type);
  EXPECT(tokens[1].get_class() == Ttx::Lexical::Class::Type::TypeAccessOp);
  EXPECT(tokens[2].get_class() == Ttx::Lexical::Class::Type::Type);

  EXPECT_TEXT(tokens[4].get_text(), ".["_view);
  EXPECT(tokens[4].get_class() == Ttx::Lexical::Class::Type::SwizzleOp);
  EXPECT_TEXT(tokens[10].get_text(), ":["_view);
  EXPECT(tokens[10].get_class() == Ttx::Lexical::Class::Type::SliceOp);

  EXPECT_TEXT(tokens[16].get_text(), "["_view);
  EXPECT(tokens[16].get_class() == Ttx::Lexical::Class::Type::IndexStart);
  EXPECT_TEXT(tokens[20].get_text(), "."_view);
  EXPECT(tokens[20].get_class() == Ttx::Lexical::Class::Type::AddressOp);
}

PERIMORTEM_UNIT_TEST(TtxParse, source_envelope) {
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
  Ttx::Parse::Source source =
      Ttx::Parse::Source::parse(cursor, "unit.ttx"_view);

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

PERIMORTEM_UNIT_TEST(TtxParse, body_comments) {
  Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(
      "// Source docs.\n"
      "dialect : Library;\n"
      "// Body docs.\n"
      "@public func main[] -> [];\n"_view);

  Ttx::Parse::Cursor cursor(tokenizer);
  Ttx::Parse::Source source = Ttx::Parse::Source::parse(cursor);

  EXPECT(cursor.get_errors().is_empty());
  EXPECT_EQ(source.get_documentation().get_line_count(), Count(1));
  EXPECT_TEXT(source.get_documentation().line_at(0), "Source docs."_view);
  EXPECT(cursor.matches(Ttx::Lexical::Class::Type::Comment));
  EXPECT_TEXT(cursor.current().get_text(), "Body docs."_view);
}

PERIMORTEM_UNIT_TEST(TtxParse, bad_imports) {
  Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(
      "dialect : Library;\n"
      "import BadColon Library = TTX::Graphics;\n"
      "import BadField : Library = (.path = \"types.ttx\");\n"
      "import Graphics : Package = TTX::Graphics;\n"
      "@public func main[] -> [];\n"_view);

  Ttx::Parse::Cursor cursor(tokenizer);
  Ttx::Parse::Source source = Ttx::Parse::Source::parse(cursor);

  EXPECT_EQ(cursor.get_errors().get_size(), Count(2));
  ASSERT_EQ(source.get_imports().get_size(), Count(1));
  EXPECT_TEXT(source.get_imports()[0].get_local_name(), "Graphics"_view);
  EXPECT(cursor.matches(Ttx::Lexical::Class::Type::ConstPublic));
}

PERIMORTEM_UNIT_TEST(TtxParse, bad_dialect_header) {
  Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(
      "dialect : TTX.Graphics;\n"
      "import Graphics : Package = TTX::Graphics;\n"_view);

  Ttx::Parse::Cursor cursor(tokenizer);
  Ttx::Parse::Source source = Ttx::Parse::Source::parse(cursor);

  EXPECT(source.get_dialect_name().is_empty());
  EXPECT(source.get_imports().is_empty());
  EXPECT_EQ(cursor.get_errors().get_size(), Count(1));
}

PERIMORTEM_UNIT_TEST(TtxParse, missing_dialect) {
  Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse("import Graphics : Package = TTX::Graphics;\n"_view);

  Ttx::Parse::Cursor cursor(tokenizer);
  Ttx::Parse::Source source = Ttx::Parse::Source::parse(cursor);

  EXPECT(source.get_dialect_name().is_empty());
  EXPECT(source.get_imports().is_empty());
  EXPECT_EQ(cursor.get_errors().get_size(), Count(1));
  EXPECT(cursor.matches(Ttx::Lexical::Class::Type::Import));
}

PERIMORTEM_UNIT_TEST(TtxParse, dialect_registry) {
  class TestDialect : public Ttx::Dialect::Dialect {
   public:
    auto get_name() const -> View::Bytes override { return "UnitDialect"_view; }
    auto parse(Ttx::Parse::Cursor&) const -> void override {}
  };

  static TestDialect dialect;
  Count starting_count =
      Ttx::Dialect::Dialect::Registry::get_dialects().get_size();

  Ttx::Dialect::Dialect::Registry::add(dialect);
  Ttx::Dialect::Dialect::Registry::add(dialect);

  EXPECT(Ttx::Dialect::Dialect::Registry::find("UnitDialect"_view) ==
         &dialect);
  EXPECT_EQ(
      Ttx::Dialect::Dialect::Registry::get_dialects().get_size(),
      starting_count + 1);
}
