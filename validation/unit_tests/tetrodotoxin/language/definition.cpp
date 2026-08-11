// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/language/definition.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Language;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness DefinitionTests = {
  .name = "Tetrodotoxin::Language::Definition"_view,
};

PERIMORTEM_UNIT_TEST(DefinitionTests, complete_prefix) {
  static constexpr View::Bytes source =
      "// Shared definition.\n"
      "@first @first(2) public public state worker : func = [] -> [] {}"_view;
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, source, "<definition>"_view);
  Cursor cursor(tokenizer, errors);

  auto definition = Definition::parse(cursor);

  ASSERT(definition);
  EXPECT_TEXT(definition->get_name(), "worker"_view);
  EXPECT(definition->get_qualifier().get_code() == Code::Type::Func);
  ASSERT_EQ(definition->get_modifiers().get_size(), Count(3));
  EXPECT(
      definition->get_modifiers().get_data()[0].get_code() ==
      Code::Type::Public);
  EXPECT(
      definition->get_modifiers().get_data()[1].get_code() ==
      Code::Type::Public);
  EXPECT(
      definition->get_modifiers().get_data()[2].get_code() ==
      Code::Type::State);
  ASSERT_EQ(definition->get_attributes().get_size(), Count(2));
  EXPECT_TEXT(
      definition->get_attributes().get_data()[0].get_key(), "first"_view);
  EXPECT_TEXT(
      definition->get_attributes().get_data()[1].get_key(), "first"_view);
  EXPECT_TEXT(
      definition->get_documentation().get_line(0), "Shared definition."_view);
  EXPECT_TEXT(
      definition->get_name_anchor().get_span().caculate_text(source),
      "worker"_view);
  EXPECT_TEXT(
      definition->get_anchor().get_span().caculate_text(source),
      "first @first(2) public public state worker : func"_view);
  EXPECT(cursor.matches(Code::Type::Func));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DefinitionTests, type_qualifier) {
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(
      arena, "private value : Unsigned_64 = 1;"_view,
      "<typed definition>"_view);
  Cursor cursor(tokenizer, errors);

  auto definition = Definition::parse(cursor);

  ASSERT(definition);
  EXPECT_TEXT(definition->get_name(), "value"_view);
  EXPECT(definition->get_qualifier().get_code() == Code::Type::Type);
  EXPECT(cursor.matches(Code::Type::Type));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DefinitionTests, malformed_is_atomic) {
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(
      arena, "@note public value Unsigned_64 = 1;"_view,
      "<malformed definition>"_view);
  Cursor cursor(tokenizer, errors);

  auto definition = Definition::parse(cursor);

  EXPECT(!definition);
  EXPECT(cursor.matches(Code::Type::Attribute));
  EXPECT(!errors.is_empty());
}
