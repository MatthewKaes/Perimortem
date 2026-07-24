// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/lexical/tokenizer.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/lexicon.hpp"

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

  EXPECT(tokens[0].get_code() == Ttx::Lexical::Code::Type::Type);
  EXPECT(tokens[1].get_code() == Ttx::Lexical::Code::Type::AddressOp);
  EXPECT_TEXT(tokens[1].caculate_text(source), "."_view);
  EXPECT(tokens[2].get_code() == Ttx::Lexical::Code::Type::Type);

  EXPECT_TEXT(tokens[4].caculate_text(source), ".["_view);
  EXPECT(tokens[4].get_code() == Ttx::Lexical::Code::Type::SwizzleOp);
  EXPECT_TEXT(tokens[10].caculate_text(source), ":["_view);
  EXPECT(tokens[10].get_code() == Ttx::Lexical::Code::Type::SliceOp);

  EXPECT_TEXT(tokens[16].caculate_text(source), "["_view);
  EXPECT(tokens[16].get_code() == Ttx::Lexical::Code::Type::LayoutStart);
  EXPECT_TEXT(tokens[20].caculate_text(source), "."_view);
  EXPECT(tokens[20].get_code() == Ttx::Lexical::Code::Type::AddressOp);
}

PERIMORTEM_UNIT_TEST(TtxLexical, modifiers) {
  Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(
      arena, "public private expose state const @package_name @public"_view,
      "Test.Package"_view);

  View::Vector<Ttx::Lexical::Token> tokens = tokenizer.get_tokens();
  View::Bytes source = tokenizer.get_source_text();
  ASSERT_EQ(tokens.get_size(), Count(8));

  EXPECT(tokens[0].get_code() == Ttx::Lexical::Code::Type::Public);
  EXPECT(tokens[1].get_code() == Ttx::Lexical::Code::Type::Private);
  EXPECT(tokens[2].get_code() == Ttx::Lexical::Code::Type::Expose);
  EXPECT(tokens[3].get_code() == Ttx::Lexical::Code::Type::State);
  EXPECT(tokens[4].get_code() == Ttx::Lexical::Code::Type::Const);
  EXPECT(tokens[5].get_code() == Ttx::Lexical::Code::Type::Attribute);
  EXPECT_TEXT(tokens[5].caculate_text(source), "package_name"_view);
  EXPECT(tokens[6].get_code() == Ttx::Lexical::Code::Type::Attribute);
  EXPECT_TEXT(tokens[6].caculate_text(source), "public"_view);
  EXPECT(tokens[7].get_code() == Ttx::Lexical::Code::Type::Terminal);
}

PERIMORTEM_UNIT_TEST(TtxLexical, lexicon) {
  using Token = Ttx::Lexical::Code::Type;

  EXPECT_TEXT(
      Ttx::Lexical::Lexicon::get_spelling(Token::TypeAccessOp), "::"_view);
  EXPECT_TEXT(Ttx::Lexical::Lexicon::get_spelling(Token::SwizzleOp), ".["_view);
  EXPECT_TEXT(Ttx::Lexical::Lexicon::get_spelling(Token::SliceOp), ":["_view);
  EXPECT_TEXT(Ttx::Lexical::Lexicon::get_spelling(Token::CallOp), "->"_view);
  EXPECT_TEXT(
      Ttx::Lexical::Lexicon::get_spelling(Token::Dialect), "dialect"_view);
  EXPECT_TEXT(
      Ttx::Lexical::Lexicon::get_spelling(Token::Resolve), "resolve"_view);
  EXPECT_TEXT(
      Ttx::Lexical::Lexicon::get_spelling(Token::Source), "source"_view);
  EXPECT_TEXT(
      Ttx::Lexical::Lexicon::get_spelling(Token::Expose), "expose"_view);
  EXPECT(Ttx::Lexical::Lexicon::get_spelling(Token::Addressable).is_empty());
  EXPECT(Ttx::Lexical::Lexicon::get_spelling(Token::Numeric).is_empty());
  EXPECT(
      Ttx::Lexical::Lexicon::get_keyword("public"_view, Token::Addressable) ==
      Token::Public);
  EXPECT(
      Ttx::Lexical::Lexicon::get_keyword("value"_view, Token::Addressable) ==
      Token::Addressable);
}

PERIMORTEM_UNIT_TEST(TtxLexical, code_semantics) {
  using Code = Ttx::Lexical::Code;

  EXPECT_EQ(static_cast<Unsigned_8>(Code::Type::Terminal), Unsigned_8(0x00));
  EXPECT_EQ(static_cast<Unsigned_8>(Code::Type::Unknown), Unsigned_8(0xFF));
  EXPECT_TEXT(
      Code(Code::Type::Public).get_semantics(),
      "public publication modifier"_view);
  EXPECT_TEXT(
      Code(Code::Type::Assign).get_semantics(), "assignment operator"_view);
  EXPECT_TEXT(
      Code(Code::Type::Addressable).get_semantics(),
      "Addressable space name"_view);
  EXPECT_TEXT(
      Code(Code::Type::Hex).get_semantics(),
      "Unsigned_64 hexadecimal literal"_view);
  EXPECT_TEXT(Code(Code::Type::Terminal).get_semantics(), "terminal Code"_view);
  EXPECT_TEXT(
      Code(Code::Type::Unknown).get_semantics(), "unknown source Code"_view);
  EXPECT_NEQ(
      Code(Code::Type::Public).get_semantics(),
      Code(Code::Type::Private).get_semantics());
}

PERIMORTEM_UNIT_TEST(TtxLexical, hexadecimal_code) {
  Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(arena, "0xAB"_view, "test.ttx"_view);

  View::Vector<Ttx::Lexical::Token> tokens = tokenizer.get_tokens();
  ASSERT_EQ(tokens.get_size(), Count(2));
  EXPECT(tokens[0].get_code() == Ttx::Lexical::Code::Type::Hex);
  EXPECT_TEXT(
      tokens[0].caculate_text(tokenizer.get_source_text()), "0xAB"_view);
  EXPECT(tokens[1].get_code() == Ttx::Lexical::Code::Type::Terminal);
}

PERIMORTEM_UNIT_TEST(TtxLexical, cursor_consume) {
  Allocator::Arena arena;
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Tokenizer tokenizer(arena, "value"_view, "test.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, errors);

  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "value"_view);
  cursor.consume();
  EXPECT(cursor.matches(Ttx::Lexical::Code::Type::Terminal));
  cursor.consume();
  EXPECT(cursor.matches(Ttx::Lexical::Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(TtxLexical, require_success) {
  Allocator::Arena arena;
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Tokenizer tokenizer(arena, "value"_view, "test.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, errors);

  Ttx::Lexical::Token token = cursor.require(
      Ttx::Lexical::Code::Type::Addressable, "Expected address."_view);

  ASSERT(token.is_valid());
  EXPECT_TEXT(token.caculate_text(cursor.get_source_text()), "value"_view);
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Ttx::Lexical::Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(TtxLexical, require_error) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Tokenizer tokenizer(arena, "value"_view, "test.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, errors);

  Ttx::Lexical::Token required =
      cursor.require(Ttx::Lexical::Code::Type::Type, "Expected type."_view);
  View::Bytes rendered = errors.render_message(render_arena, 0);

  EXPECT_NOT(required.is_valid());
  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(Algorithm::search(rendered, "Expected type."_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          rendered,
          "Expected lexical token Type space name but got Addressable space name."_view) !=
      Count(-1));
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
  EXPECT_TEXT(token.caculate_text("red sky green"_view), "sky"_view);
}
