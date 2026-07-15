// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/lexical/tokenizer.hpp"

#include "validation/unit_test.hpp"

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
  ASSERT_EQ(tokens.get_size(), Count(23));

  EXPECT(tokens[0].get_class() == Ttx::Lexical::Class::Type::Type);
  EXPECT(tokens[1].get_class() == Ttx::Lexical::Class::Type::AddressOp);
  EXPECT_TEXT(tokens[1].get_text(), "."_view);
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

PERIMORTEM_UNIT_TEST(TtxLexical, modifiers) {
  Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(
      arena, "public private expose state const @package_name @public"_view,
      "Test.Package"_view);

  View::Vector<Ttx::Lexical::Token> tokens = tokenizer.get_tokens();
  ASSERT_EQ(tokens.get_size(), Count(8));

  EXPECT(tokens[0].get_class() == Ttx::Lexical::Class::Type::Public);
  EXPECT(tokens[1].get_class() == Ttx::Lexical::Class::Type::Private);
  EXPECT(tokens[2].get_class() == Ttx::Lexical::Class::Type::Expose);
  EXPECT(tokens[3].get_class() == Ttx::Lexical::Class::Type::State);
  EXPECT(tokens[4].get_class() == Ttx::Lexical::Class::Type::Const);
  EXPECT(tokens[5].get_class() == Ttx::Lexical::Class::Type::Attribute);
  EXPECT_TEXT(tokens[5].get_text(), "@package_name"_view);
  EXPECT(tokens[6].get_class() == Ttx::Lexical::Class::Type::Attribute);
  EXPECT_TEXT(tokens[6].get_text(), "@public"_view);
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
  Allocator::Arena error_arena;
  Ttx::Lexical::Tokenizer tokenizer(arena, "value"_view, "test.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, error_arena);

  EXPECT_TEXT(cursor.current().get_text(), "value"_view);
  cursor.consume();
  EXPECT(cursor.matches(Ttx::Lexical::Class::Type::EndOfStream));
  cursor.consume();
  EXPECT(cursor.matches(Ttx::Lexical::Class::Type::EndOfStream));
}

PERIMORTEM_UNIT_TEST(TtxLexical, cursor_seek) {
  Allocator::Arena arena;
  Allocator::Arena error_arena;
  Ttx::Lexical::Tokenizer tokenizer(
      arena, "one two three"_view, "test.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, error_arena);

  cursor.seek_token(1);
  EXPECT_TEXT(cursor.current().get_text(), "two"_view);
  cursor.seek_token(99);
  EXPECT(cursor.matches(Ttx::Lexical::Class::Type::EndOfStream));
}

PERIMORTEM_UNIT_TEST(TtxLexical, require_success) {
  Allocator::Arena arena;
  Allocator::Arena error_arena;
  Ttx::Lexical::Tokenizer tokenizer(arena, "value"_view, "test.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, error_arena);

  const Ttx::Lexical::Token* token = cursor.require(
      Ttx::Lexical::Class::Type::Addressable, "Expected address."_view);

  ASSERT(token != nullptr);
  EXPECT_TEXT(token->get_text(), "value"_view);
  EXPECT_NOT(cursor.get_errors().has_errors());
  EXPECT(cursor.matches(Ttx::Lexical::Class::Type::EndOfStream));
}

PERIMORTEM_UNIT_TEST(TtxLexical, require_error) {
  Allocator::Arena arena;
  Allocator::Arena error_arena;
  Ttx::Lexical::Tokenizer tokenizer(arena, "value"_view, "test.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, error_arena);

  EXPECT(
      cursor.require(Ttx::Lexical::Class::Type::Type, "Expected type."_view) ==
      nullptr);

  ASSERT(cursor.get_errors().has_errors());
  EXPECT_TEXT(
      cursor.get_errors().get_view()[0].get_message(), "Expected type."_view);
  EXPECT_TEXT(
      cursor.get_errors().get_view()[0].get_source_path(), "test.ttx"_view);
  EXPECT_TEXT(cursor.current().get_text(), "value"_view);
}

PERIMORTEM_UNIT_TEST(TtxLexical, range_error) {
  Allocator::Arena arena;
  Allocator::Arena error_arena;
  Ttx::Lexical::Tokenizer tokenizer(arena, "value = 1"_view, "test.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, error_arena);

  const Ttx::Lexical::Token& start = cursor.current();
  cursor.seek_token(2);
  const Ttx::Lexical::Token& end = cursor.current();
  cursor.range_error(start, end, "Bad expression."_view, "Use a value."_view);

  ASSERT(cursor.get_errors().has_errors());
  const Ttx::Lexical::Errors::Error& error = cursor.get_errors().get_view()[0];
  EXPECT(&error.get_start_token() == &start);
  EXPECT(&error.get_end_token() == &end);
  EXPECT_TEXT(error.get_hint(), "Use a value."_view);
}

PERIMORTEM_UNIT_TEST(TtxLexical, source_error) {
  Allocator::Arena arena;
  Ttx::Lexical::Errors errors(arena);

  errors.insert(
      Ttx::Lexical::Source("test.ttx"_view, "source"_view), "Bad source."_view,
      "Try again."_view);

  ASSERT(errors.has_errors());
  EXPECT_TEXT(errors.get_view()[0].get_source_path(), "test.ttx"_view);
  EXPECT_TEXT(errors.get_view()[0].get_source(), "source"_view);
  EXPECT_TEXT(errors.get_view()[0].get_message(), "Bad source."_view);
  EXPECT_TEXT(errors.get_view()[0].get_hint(), "Try again."_view);
}

PERIMORTEM_UNIT_TEST(TtxLexical, token_error) {
  Allocator::Arena arena;
  Ttx::Lexical::Errors errors(arena);
  Ttx::Lexical::Token token(
      "value"_view, Ttx::Lexical::Class::Type::Addressable, 4, 7);

  errors.insert(
      token, Ttx::Lexical::Source("test.ttx"_view, "value"_view),
      "Bad token."_view);

  ASSERT(errors.has_errors());
  EXPECT(errors.get_view()[0].has_tokens());
  EXPECT(&errors.get_view()[0].get_start_token() == &token);
  EXPECT(&errors.get_view()[0].get_end_token() == &token);
  EXPECT_EQ(errors.get_view()[0].get_start_token().get_line(), Bits_32(4));
  EXPECT_EQ(errors.get_view()[0].get_start_token().get_column(), Bits_32(7));
}

PERIMORTEM_UNIT_TEST(TtxLexical, recover_stmt) {
  Allocator::Arena arena;
  Allocator::Arena error_arena;
  Ttx::Lexical::Tokenizer tokenizer(
      arena, "bad tokens ; next"_view, "test.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, error_arena);

  cursor.recover_to_statement();

  EXPECT_TEXT(cursor.current().get_text(), "next"_view);
}

PERIMORTEM_UNIT_TEST(TtxLexical, token_span) {
  Allocator::Arena arena;
  Allocator::Arena error_arena;
  Ttx::Lexical::Tokenizer tokenizer(
      arena, "one two three"_view, "test.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, error_arena);

  View::Vector<Ttx::Lexical::Token> span = cursor.get_token_span(1, 3);

  ASSERT_EQ(span.get_size(), Count(2));
  EXPECT_TEXT(span[0].get_text(), "two"_view);
  EXPECT_TEXT(span[1].get_text(), "three"_view);
  EXPECT(cursor.get_token_span(3, 1).is_empty());
}
