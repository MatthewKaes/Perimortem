// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/lexical/tokenizer.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/lexical/cursor.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Validation;

static Harness TtxLexical = {
  .name = "TTX::Lexical"_view,
};

PERIMORTEM_UNIT_TEST(TtxLexical, access_operators) {
  Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(
      arena,
      "Perimortem.Graphics color.[r, g] color:[start, 2] "
      "layout[Type] value.member"_view,
      "Test.Package"_view);

  View::Vector<Ttx::Lexical::Token> tokens = tokenizer.get_tokens();
  View::Bytes source = tokenizer.get_source_text();
  ASSERT_EQ(tokens.get_size(), Count(23));

  EXPECT(tokens[0].get_class() == Ttx::Lexical::Class::Type::Type);
  EXPECT(tokens[1].get_class() == Ttx::Lexical::Class::Type::AddressOp);
  EXPECT_TEXT(tokens[1].caculate_text(source), "."_view);
  EXPECT(tokens[2].get_class() == Ttx::Lexical::Class::Type::Type);

  EXPECT_TEXT(tokens[4].caculate_text(source), ".["_view);
  EXPECT(tokens[4].get_class() == Ttx::Lexical::Class::Type::SwizzleOp);
  EXPECT_TEXT(tokens[10].caculate_text(source), ":["_view);
  EXPECT(tokens[10].get_class() == Ttx::Lexical::Class::Type::SliceOp);

  EXPECT_TEXT(tokens[16].caculate_text(source), "["_view);
  EXPECT(tokens[16].get_class() == Ttx::Lexical::Class::Type::IndexStart);
  EXPECT_TEXT(tokens[20].caculate_text(source), "."_view);
  EXPECT(tokens[20].get_class() == Ttx::Lexical::Class::Type::AddressOp);
}

PERIMORTEM_UNIT_TEST(TtxLexical, modifiers) {
  Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(
      arena, "public private expose state const @package_name @public"_view,
      "Test.Package"_view);

  View::Vector<Ttx::Lexical::Token> tokens = tokenizer.get_tokens();
  View::Bytes source = tokenizer.get_source_text();
  ASSERT_EQ(tokens.get_size(), Count(8));

  EXPECT(tokens[0].get_class() == Ttx::Lexical::Class::Type::Public);
  EXPECT(tokens[1].get_class() == Ttx::Lexical::Class::Type::Private);
  EXPECT(tokens[2].get_class() == Ttx::Lexical::Class::Type::Expose);
  EXPECT(tokens[3].get_class() == Ttx::Lexical::Class::Type::State);
  EXPECT(tokens[4].get_class() == Ttx::Lexical::Class::Type::Const);
  EXPECT(tokens[5].get_class() == Ttx::Lexical::Class::Type::Attribute);
  EXPECT_TEXT(tokens[5].caculate_text(source), "package_name"_view);
  EXPECT(tokens[6].get_class() == Ttx::Lexical::Class::Type::Attribute);
  EXPECT_TEXT(tokens[6].caculate_text(source), "public"_view);
  EXPECT(tokens[7].get_class() == Ttx::Lexical::Class::Type::EndOfStream);
}

PERIMORTEM_UNIT_TEST(TtxLexical, fixed_text) {
  using Token = Ttx::Lexical::Class::Type;

  EXPECT_TEXT(
      Ttx::Lexical::Class::get_source_text(Token::TypeAccessOp), "::"_view);
  EXPECT_TEXT(
      Ttx::Lexical::Class::get_source_text(Token::SwizzleOp), ".["_view);
  EXPECT_TEXT(Ttx::Lexical::Class::get_source_text(Token::SliceOp), ":["_view);
  EXPECT_TEXT(Ttx::Lexical::Class::get_source_text(Token::CallOp), "->"_view);
  EXPECT_TEXT(
      Ttx::Lexical::Class::get_source_text(Token::Dialect), "dialect"_view);
  EXPECT_TEXT(
      Ttx::Lexical::Class::get_source_text(Token::Expose), "expose"_view);
}

PERIMORTEM_UNIT_TEST(TtxLexical, cursor_consume) {
  Allocator::Arena arena;
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Tokenizer tokenizer(arena, "value"_view, "test.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, errors);

  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "value"_view);
  cursor.consume();
  EXPECT(cursor.matches(Ttx::Lexical::Class::Type::EndOfStream));
  cursor.consume();
  EXPECT(cursor.matches(Ttx::Lexical::Class::Type::EndOfStream));
}

PERIMORTEM_UNIT_TEST(TtxLexical, require_success) {
  Allocator::Arena arena;
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Tokenizer tokenizer(arena, "value"_view, "test.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, errors);

  Ttx::Lexical::Token token = cursor.require(
      Ttx::Lexical::Class::Type::Addressable, "Expected address."_view);

  ASSERT(token.is_valid());
  EXPECT_TEXT(token.caculate_text(cursor.get_source_text()), "value"_view);
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Ttx::Lexical::Class::Type::EndOfStream));
}

PERIMORTEM_UNIT_TEST(TtxLexical, require_error) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Tokenizer tokenizer(arena, "value"_view, "test.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, errors);

  Ttx::Lexical::Token required = cursor.require(
      Ttx::Lexical::Class::Type::Type, "Expected type."_view);
  View::Bytes rendered = errors.render_message(render_arena, 0);

  EXPECT_NOT(required.is_valid());
  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(Algorithm::search(rendered, "Expected type."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "test.ttx"_view) != Count(-1));
  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "value"_view);
}

PERIMORTEM_UNIT_TEST(TtxLexical, range_error) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Tokenizer tokenizer(arena, "value = 1"_view, "test.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, errors);

  Ttx::Lexical::Token start = cursor.current();
  cursor.consume();
  cursor.consume();
  Ttx::Lexical::Token end = cursor.current();
  cursor.create_expression_error(
      start, end, "Bad expression."_view, "Use a value."_view);
  View::Bytes rendered = errors.render_message(render_arena, 0);

  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(Algorithm::search(rendered, "Bad expression."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Use a value."_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLexical, source_error) {
  Allocator::Arena render_arena;
  Ttx::Lexical::Errors errors;

  errors.set_source_context("test.ttx"_view, "source"_view);
  errors.create_general_error("Bad source."_view, "Try again."_view);
  View::Bytes rendered = errors.render_message(render_arena, 0);

  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(Algorithm::search(rendered, "test.ttx"_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Bad source."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Try again."_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLexical, token_error) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Tokenizer tokenizer(
      arena, "one\ntwo\nthree value"_view, "test.ttx"_view);
  Ttx::Lexical::Token token = tokenizer.get_tokens()[3];

  errors.set_source_context(
      tokenizer.get_source_path(), tokenizer.get_source_text());
  errors.create_token_error(token, "Bad token."_view);
  View::Bytes rendered = errors.render_message(render_arena, 0);

  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(Algorithm::search(rendered, "test.ttx:3:7"_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Bad token."_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLexical, recover_stmt) {
  Allocator::Arena arena;
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Tokenizer tokenizer(
      arena, "bad tokens ; next"_view, "test.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, errors);

  cursor.recover_to_statement();

  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "next"_view);
}

PERIMORTEM_UNIT_TEST(TtxLexical, token_projection) {
  Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(
      arena, "one two three"_view, "test.ttx"_view);
  Ttx::Lexical::Token token = tokenizer.get_tokens()[1];

  EXPECT_TEXT(token.caculate_text(tokenizer.get_source_text()), "two"_view);
  EXPECT_TEXT(
      token.caculate_text("red blue green"_view), "blue"_view);
}
