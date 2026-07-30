// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/lexical/tokenizer.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
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

PERIMORTEM_UNIT_TEST(TtxLexical, spelling_validation) {
  using Token = Ttx::Lexical::Code::Type;

  static constexpr Static::Vector<Token, 1> type_access = {{
    Token::TypeAccessOp,
  }};
  static constexpr Static::Vector<Token, 1> address = {{
    Token::AddressOp,
  }};
  static constexpr Static::Vector<Token, 2> qualified = {{
    Token::TypeAccessOp,
    Token::AddressOp,
  }};

  EXPECT(Ttx::Lexical::Lexicon::validate(Token::Type, "Type"_view));
  EXPECT(Ttx::Lexical::Lexicon::validate(Token::Type, "Type_Name2"_view));
  EXPECT_NOT(Ttx::Lexical::Lexicon::validate(Token::Type, View::Bytes()));
  EXPECT_NOT(Ttx::Lexical::Lexicon::validate(Token::Type, "type"_view));
  EXPECT_NOT(Ttx::Lexical::Lexicon::validate(Token::Type, "Type.Name"_view));

  EXPECT(
      Ttx::Lexical::Lexicon::validate(
          Token::Type, "Scenes::Splash"_view, type_access));
  EXPECT(
      Ttx::Lexical::Lexicon::validate(
          Token::Type, "Perimortem.Graphics"_view, address));
  EXPECT(
      Ttx::Lexical::Lexicon::validate(
          Token::Type, "Root::Child.Leaf"_view, qualified));
  EXPECT_NOT(
      Ttx::Lexical::Lexicon::validate(
          Token::Type, "::Splash"_view, type_access));
  EXPECT_NOT(
      Ttx::Lexical::Lexicon::validate(
          Token::Type, "Scenes::"_view, type_access));
  EXPECT_NOT(
      Ttx::Lexical::Lexicon::validate(
          Token::Type, "Scenes::::Splash"_view, type_access));
  EXPECT_NOT(
      Ttx::Lexical::Lexicon::validate(
          Token::Type, "Scenes :: Splash"_view, type_access));

  EXPECT(
      Ttx::Lexical::Lexicon::validate(Token::Addressable, "local_name"_view));
  EXPECT_NOT(
      Ttx::Lexical::Lexicon::validate(Token::Addressable, "public"_view));
  EXPECT(Ttx::Lexical::Lexicon::validate(Token::Public, "public"_view));
  EXPECT(Ttx::Lexical::Lexicon::validate(Token::Numeric, "123"_view));
  EXPECT_NOT(Ttx::Lexical::Lexicon::validate(Token::Numeric, "1.2"_view));
  EXPECT(Ttx::Lexical::Lexicon::validate(Token::Float, "1.25"_view));
  EXPECT_NOT(Ttx::Lexical::Lexicon::validate(Token::Float, "1.2.5"_view));
  EXPECT(Ttx::Lexical::Lexicon::validate(Token::Hex, "0xAB"_view));
  EXPECT_NOT(Ttx::Lexical::Lexicon::validate(Token::Hex, "0x"_view));

  EXPECT(Ttx::Lexical::Lexicon::validate(Token::String, "\"1.0\""_view));
  EXPECT(
      Ttx::Lexical::Lexicon::validate(
          Token::String, "\"escaped \\\" quote\""_view));
  EXPECT_NOT(
      Ttx::Lexical::Lexicon::validate(Token::String, "\"unterminated"_view));
  EXPECT_NOT(
      Ttx::Lexical::Lexicon::validate(
          Token::String, "\"escaped terminal\\\""_view));
  EXPECT(Ttx::Lexical::Lexicon::validate(Token::Bytes, "0x[]"_view));
  EXPECT(
      Ttx::Lexical::Lexicon::validate(
          Token::Embedded, "$[resources/table.bin]"_view));
  EXPECT(Ttx::Lexical::Lexicon::validate(Token::Comment, "// text"_view));
  EXPECT_NOT(Ttx::Lexical::Lexicon::validate(Token::Comment, "// line\n"_view));
  EXPECT(Ttx::Lexical::Lexicon::validate(Token::TypeAccessOp, "::"_view));
  EXPECT_NOT(Ttx::Lexical::Lexicon::validate(Token::Unknown, "?"_view));
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

  {
    Ttx::Lexical::Errors::Report report(errors, "test.ttx"_view, "source"_view);
    report << "Bad source."_view;
    report.get_hint() << "Try again."_view;
  }

  View::Bytes rendered = errors.render_message(render_arena, 0);

  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(Algorithm::search(rendered, "test.ttx"_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Bad source."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Try again."_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLexical, scoped_report) {
  Allocator::Arena render_arena;
  Ttx::Lexical::Errors errors;

  {
    Ttx::Lexical::Errors::Report empty(errors, "empty.ttx"_view, View::Bytes());
  }
  EXPECT(errors.is_empty());

  {
    Allocator::Arena source_arena;
    Ttx::Lexical::Tokenizer tokenizer(
        source_arena, "report source"_view, "report.ttx"_view);
    Ttx::Lexical::Token token = tokenizer.get_tokens()[0];
    Ttx::Lexical::Errors::Report report(
        errors, tokenizer.get_source_path(), tokenizer.get_source_text(),
        token);

    report << "Scoped report "_view << Count(7) << "."_view;
    report.get_hint() << "Use the retained message."_view;
  }

  {
    Allocator::Arena replacement_arena;
    Ttx::Lexical::Tokenizer replacement(
        replacement_arena, "replacement source"_view, "report.ttx"_view);
    Ttx::Lexical::Errors::Report report(
        errors, replacement.get_source_path(), replacement.get_source_text(),
        replacement.get_tokens()[0]);
    report << "Repeated report."_view;
  }

  {
    Ttx::Lexical::Errors::Report report(
        errors, "outer.ttx"_view, "outer source"_view);
    report << "Outer report."_view;
  }

  View::Bytes scoped = errors.render_message(render_arena, 0);
  View::Bytes repeated = errors.render_message(render_arena, 1);
  View::Bytes outer = errors.render_message(render_arena, 2);

  ASSERT_EQ(errors.get_size(), Count(3));
  EXPECT(Algorithm::search(scoped, "report.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(scoped, "report source"_view) != Count(-1));
  EXPECT(Algorithm::search(scoped, "Scoped report 7."_view) != Count(-1));
  EXPECT(
      Algorithm::search(scoped, "Use the retained message."_view) != Count(-1));
  EXPECT(Algorithm::search(repeated, "report source"_view) != Count(-1));
  EXPECT(Algorithm::search(repeated, "replacement source"_view) == Count(-1));
  EXPECT(Algorithm::search(repeated, "Repeated report."_view) != Count(-1));
  EXPECT(Algorithm::search(outer, "outer.ttx:"_view) != Count(-1));
  EXPECT(Algorithm::search(outer, "Outer report."_view) != Count(-1));
  EXPECT(Algorithm::search(outer, "report.ttx"_view) == Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLexical, overlapping_cursors) {
  Allocator::Arena render_arena;
  Ttx::Lexical::Errors errors;

  {
    Allocator::Arena outer_arena;
    Ttx::Lexical::Tokenizer outer_tokenizer(
        outer_arena, "outer start finish"_view, "outer.ttx"_view);
    Ttx::Lexical::Cursor outer(outer_tokenizer, errors);

    {
      Allocator::Arena inner_arena;
      Ttx::Lexical::Tokenizer inner_tokenizer(
          inner_arena, "inner source"_view, "inner.ttx"_view);
      Ttx::Lexical::Cursor inner(inner_tokenizer, errors);

      outer.create_token_error(
          "Outer while inner is live."_view, "Outer token hint."_view);
      inner.create_token_error(
          "Inner while nested."_view, "Inner token hint."_view);
    }

    Ttx::Lexical::Token start = outer.current();
    outer.consume();
    outer.consume();
    Ttx::Lexical::Token end = outer.current();
    outer.create_expression_error(
        start, end, "Outer after inner."_view, "Outer range hint."_view);
  }

  View::Bytes first = errors.render_message(render_arena, 0);
  View::Bytes second = errors.render_message(render_arena, 1);
  View::Bytes third = errors.render_message(render_arena, 2);

  ASSERT_EQ(errors.get_size(), Count(3));
  EXPECT(Algorithm::search(first, "outer.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(first, "outer start finish"_view) != Count(-1));
  EXPECT(
      Algorithm::search(first, "Outer while inner is live."_view) != Count(-1));
  EXPECT(Algorithm::search(first, "Outer token hint."_view) != Count(-1));
  EXPECT(Algorithm::search(first, "^-----\n"_view) != Count(-1));
  EXPECT(Algorithm::search(first, "inner.ttx"_view) == Count(-1));

  EXPECT(Algorithm::search(second, "inner.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(second, "inner source"_view) != Count(-1));
  EXPECT(Algorithm::search(second, "Inner while nested."_view) != Count(-1));
  EXPECT(Algorithm::search(second, "Inner token hint."_view) != Count(-1));
  EXPECT(Algorithm::search(second, "^-----\n"_view) != Count(-1));
  EXPECT(Algorithm::search(second, "outer.ttx"_view) == Count(-1));

  EXPECT(Algorithm::search(third, "outer.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(third, "outer start finish"_view) != Count(-1));
  EXPECT(Algorithm::search(third, "Outer after inner."_view) != Count(-1));
  EXPECT(Algorithm::search(third, "Outer range hint."_view) != Count(-1));
  EXPECT(Algorithm::search(third, "^------------------\n"_view) != Count(-1));
  EXPECT(Algorithm::search(third, "inner.ttx"_view) == Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLexical, wrapper_messages) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Tokenizer tokenizer(
      arena, "first second third"_view, "wrappers.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, errors);

  cursor.create_error("General wrapper."_view, "General hint."_view);
  cursor.create_token_error("Token wrapper."_view, "Token hint."_view);
  Ttx::Lexical::Token start = cursor.current();
  cursor.consume();
  Ttx::Lexical::Token end = cursor.current();
  cursor.create_expression_error(
      start, end, "Expression wrapper."_view, "Expression hint."_view);

  View::Bytes general = errors.render_message(render_arena, 0);
  View::Bytes token = errors.render_message(render_arena, 1);
  View::Bytes expression = errors.render_message(render_arena, 2);

  ASSERT_EQ(errors.get_size(), Count(3));
  EXPECT(Algorithm::search(general, "wrappers.ttx:"_view) != Count(-1));
  EXPECT(Algorithm::search(general, "General wrapper."_view) != Count(-1));
  EXPECT(Algorithm::search(general, "General hint."_view) != Count(-1));
  EXPECT(Algorithm::search(token, "wrappers.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(token, "Token wrapper."_view) != Count(-1));
  EXPECT(Algorithm::search(token, "Token hint."_view) != Count(-1));
  EXPECT(Algorithm::search(expression, "wrappers.ttx:1:1"_view) != Count(-1));
  EXPECT(
      Algorithm::search(expression, "Expression wrapper."_view) != Count(-1));
  EXPECT(Algorithm::search(expression, "Expression hint."_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLexical, reversed_range) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Tokenizer tokenizer(
      arena, "one two three"_view, "reversed.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, errors);

  Ttx::Lexical::Token end = cursor.current();
  cursor.consume();
  cursor.consume();
  Ttx::Lexical::Token start = cursor.current();
  cursor.create_expression_error(
      start, end, "Reversed range."_view, "Clamp to start."_view);
  View::Bytes rendered = errors.render_message(render_arena, 0);

  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(Algorithm::search(rendered, "reversed.ttx:1:9"_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Reversed range."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Clamp to start."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "^-----\n"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLexical, keyword_bail) {
  static constexpr View::Bytes generated =
      "Expected `expected` but got `actual`."_view;
  Allocator::Arena render_arena;
  Ttx::Lexical::Errors errors;

  {
    Allocator::Arena arena;
    Ttx::Lexical::Tokenizer tokenizer(
        arena, "actual;"_view, "default.ttx"_view);
    Ttx::Lexical::Cursor cursor(tokenizer, errors);
    EXPECT(cursor.bail("expected"_view));
  }

  {
    Allocator::Arena arena;
    Ttx::Lexical::Tokenizer tokenizer(arena, "actual;"_view, "custom.ttx"_view);
    Ttx::Lexical::Cursor cursor(tokenizer, errors);
    EXPECT(cursor.bail("expected"_view, "Custom keyword failure."_view));
  }

  View::Bytes default_message = errors.render_message(render_arena, 0);
  View::Bytes custom_message = errors.render_message(render_arena, 1);

  ASSERT_EQ(errors.get_size(), Count(2));
  EXPECT(
      Algorithm::search(default_message, "default.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(default_message, generated) != Count(-1));
  EXPECT(Algorithm::search(default_message, "Note: "_view) == Count(-1));
  EXPECT(Algorithm::search(custom_message, "custom.ttx:1:1"_view) != Count(-1));
  EXPECT(
      Algorithm::search(custom_message, "Custom keyword failure."_view) !=
      Count(-1));
  EXPECT(Algorithm::search(custom_message, "Note: "_view) != Count(-1));
  EXPECT(Algorithm::search(custom_message, generated) != Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLexical, retained_source_name) {
  Allocator::Arena render_arena;
  Ttx::Lexical::Errors errors;

  {
    Allocator::Arena source_arena;
    Ttx::Lexical::Tokenizer tokenizer(
        source_arena, "first value"_view, "test.ttx"_view);
    Ttx::Lexical::Cursor cursor(tokenizer, errors);
    cursor.create_token_error("First failure."_view);
  }

  {
    Allocator::Arena source_arena;
    Ttx::Lexical::Tokenizer tokenizer(
        source_arena, "second value"_view, "test.ttx"_view);
    Ttx::Lexical::Cursor cursor(tokenizer, errors);
    cursor.create_token_error("Second failure."_view);
  }

  View::Bytes first = errors.render_message(render_arena, 0);
  View::Bytes second = errors.render_message(render_arena, 1);

  ASSERT_EQ(errors.get_size(), Count(2));
  EXPECT(Algorithm::search(first, "first value"_view) != Count(-1));
  EXPECT(Algorithm::search(first, "second value"_view) == Count(-1));
  EXPECT(Algorithm::search(second, "first value"_view) != Count(-1));
  EXPECT(Algorithm::search(second, "second value"_view) == Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLexical, token_error) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Tokenizer tokenizer(
      arena, "one\ntwo\nthree value"_view, "test.ttx"_view);
  Ttx::Lexical::Token token = tokenizer.get_tokens()[3];

  {
    Ttx::Lexical::Errors::Report report(
        errors, tokenizer.get_source_path(), tokenizer.get_source_text(), token,
        token);
    report << "Bad token."_view;
  }

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
