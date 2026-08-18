// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/lexical/tokenizer.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/lexicon.hpp"
#include "ttx/lexical/span.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness TtxLexical = {
  .name = "TTX::Lexical"_view,
};

PERIMORTEM_UNIT_TEST(TtxLexical, access_operators) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "Perimortem.Graphics color.[r, g] color:[start, 2] "
      "layout[Type] value.member"_view,
      "Test.Package"_view);

  View::Vector<Token> tokens = tokenizer.get_tokens();
  View::Bytes source = tokenizer.get_source_text();
  const auto* token_data = tokens.get_data();
  ASSERT_EQ(tokens.get_size(), Count(23));

  EXPECT(token_data[0].get_code() == Code::Type::Type);
  EXPECT(token_data[1].get_code() == Code::Type::AddressOp);
  EXPECT_TEXT(token_data[1].caculate_text(source), "."_view);
  EXPECT(token_data[2].get_code() == Code::Type::Type);

  EXPECT_TEXT(token_data[4].caculate_text(source), ".["_view);
  EXPECT(token_data[4].get_code() == Code::Type::SwizzleOp);
  EXPECT_TEXT(token_data[10].caculate_text(source), ":["_view);
  EXPECT(token_data[10].get_code() == Code::Type::ValueAccessOp);

  EXPECT_TEXT(token_data[16].caculate_text(source), "["_view);
  EXPECT(token_data[16].get_code() == Code::Type::BracketStart);
  EXPECT_TEXT(token_data[20].caculate_text(source), "."_view);
  EXPECT(token_data[20].get_code() == Code::Type::AddressOp);
}

PERIMORTEM_UNIT_TEST(TtxLexical, assignment_operators_are_complete_tokens) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena, "left = right += add -= subtract"_view, "Test.ttx"_view);

  View::Vector<Token> tokens = tokenizer.get_tokens();
  View::Bytes source = tokenizer.get_source_text();
  ASSERT_EQ(tokens.get_size(), Count(8));

  EXPECT(tokens.get_data()[1].get_code() == Code::Type::Assign);
  EXPECT_TEXT(tokens.get_data()[1].caculate_text(source), "="_view);
  EXPECT(tokens.get_data()[3].get_code() == Code::Type::AddAssign);
  EXPECT_TEXT(tokens.get_data()[3].caculate_text(source), "+="_view);
  EXPECT(tokens.get_data()[5].get_code() == Code::Type::SubAssign);
  EXPECT_TEXT(tokens.get_data()[5].caculate_text(source), "-="_view);
}

PERIMORTEM_UNIT_TEST(TtxLexical, divide_before_greater) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "/> // retained comment\nleft / right -> call value > other >= floor"_view,
      "Test.Package"_view);

  View::Vector<Token> tokens = tokenizer.get_tokens();
  View::Bytes source = tokenizer.get_source_text();
  const auto* token_data = tokens.get_data();
  ASSERT_EQ(tokens.get_size(), Count(14));

  EXPECT(token_data[0].get_code() == Code::Type::DivOp);
  EXPECT_TEXT(token_data[0].caculate_text(source), "/"_view);
  EXPECT(token_data[1].get_code() == Code::Type::GreaterOp);
  EXPECT_TEXT(token_data[1].caculate_text(source), ">"_view);
  EXPECT(token_data[2].get_code() == Code::Type::Comment);
  EXPECT(token_data[4].get_code() == Code::Type::DivOp);
  EXPECT(token_data[6].get_code() == Code::Type::CallOp);
  EXPECT(token_data[9].get_code() == Code::Type::GreaterOp);
  EXPECT(token_data[11].get_code() == Code::Type::GreaterEqOp);
  EXPECT(token_data[13].get_code() == Code::Type::Terminal);
}

PERIMORTEM_UNIT_TEST(TtxLexical, propagation_operator) {
  Allocator::Arena arena;
  Tokenizer tokenizer(arena, "value?\nnext?"_view, "Test.Package"_view);

  View::Vector<Token> tokens = tokenizer.get_tokens();
  View::Bytes source = tokenizer.get_source_text();
  const auto* token_data = tokens.get_data();
  ASSERT_EQ(tokens.get_size(), Count(5));
  EXPECT(token_data[0].get_code() == Code::Type::Addressable);
  EXPECT(token_data[1].get_code() == Code::Type::QuestionOp);
  EXPECT_TEXT(token_data[1].caculate_text(source), "?"_view);
  EXPECT_EQ(token_data[1].get_offset(), Unsigned_32(5));
  EXPECT_EQ(token_data[1].get_line(), Unsigned_32(1));
  EXPECT_EQ(token_data[1].get_column(), Unsigned_32(6));
  EXPECT_EQ(token_data[1].get_size(), Unsigned_32(1));
  EXPECT(token_data[2].get_code() == Code::Type::Addressable);
  EXPECT(token_data[3].get_code() == Code::Type::QuestionOp);
  EXPECT_TEXT(token_data[3].caculate_text(source), "?"_view);
  EXPECT_EQ(token_data[3].get_offset(), Unsigned_32(11));
  EXPECT_EQ(token_data[3].get_line(), Unsigned_32(2));
  EXPECT_EQ(token_data[3].get_column(), Unsigned_32(5));
  EXPECT_EQ(token_data[3].get_size(), Unsigned_32(1));
  EXPECT(token_data[4].get_code() == Code::Type::Terminal);
}

PERIMORTEM_UNIT_TEST(TtxLexical, reserved_keywords) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "public private expose state const enum struct object using new from "
      "emit emitter @package_name @public"_view,
      "Test.Package"_view);

  View::Vector<Token> tokens = tokenizer.get_tokens();
  View::Bytes source = tokenizer.get_source_text();
  const auto* token_data = tokens.get_data();
  static constexpr Code::Type expected[] = {
    Code::Type::Public,      Code::Type::Private, Code::Type::Expose,
    Code::Type::State,       Code::Type::Const,   Code::Type::Enum,
    Code::Type::Struct,      Code::Type::Object,  Code::Type::Using,
    Code::Type::New,         Code::Type::From,    Code::Type::Emit,
    Code::Type::Addressable,
  };
  static constexpr Count expected_size = sizeof(expected) / sizeof(*expected);
  ASSERT_EQ(tokens.get_size(), expected_size + 3);
  for (Count i = 0; i < expected_size; i++) {
    EXPECT(token_data[i].get_code() == expected[i]);
  }

  EXPECT(token_data[expected_size].get_code() == Code::Type::Attribute);
  EXPECT_TEXT(
      token_data[expected_size].caculate_text(source), "package_name"_view);
  EXPECT(token_data[expected_size + 1].get_code() == Code::Type::Attribute);
  EXPECT_TEXT(
      token_data[expected_size + 1].caculate_text(source), "public"_view);
  EXPECT(token_data[expected_size + 2].get_code() == Code::Type::Terminal);
}

PERIMORTEM_UNIT_TEST(TtxLexical, lexicon) {
  using Token = Code::Type;

  EXPECT_TEXT(Lexicon::get_spelling(Token::TypeAccessOp), "::"_view);
  EXPECT_TEXT(Lexicon::get_spelling(Token::SwizzleOp), ".["_view);
  EXPECT_TEXT(Lexicon::get_spelling(Token::ValueAccessOp), ":["_view);
  EXPECT_TEXT(Lexicon::get_spelling(Token::CallOp), "->"_view);
  EXPECT_TEXT(Lexicon::get_spelling(Token::QuestionOp), "?"_view);
  EXPECT_TEXT(Lexicon::get_spelling(Token::Dialect), "dialect"_view);
  EXPECT_TEXT(Lexicon::get_spelling(Token::Resolve), "resolve"_view);
  EXPECT_TEXT(Lexicon::get_spelling(Token::Source), "source"_view);
  EXPECT_TEXT(Lexicon::get_spelling(Token::Emit), "emit"_view);
  EXPECT_TEXT(Lexicon::get_spelling(Token::Expose), "expose"_view);
  for (Unsigned_8 value = 0; value <= static_cast<Unsigned_8>(Token::Const);
       value++) {
    EXPECT_NOT(Lexicon::get_spelling(Token(value)) == "/>"_view);
  }
  EXPECT(Lexicon::get_spelling(Token::Addressable).is_empty());
  EXPECT(Lexicon::get_spelling(Token::Numeric).is_empty());
  EXPECT(
      Lexicon::get_keyword("public"_view, Token::Addressable) == Token::Public);
  EXPECT(Lexicon::get_keyword("emit"_view, Token::Addressable) == Token::Emit);
  EXPECT(
      Lexicon::get_keyword("emitter"_view, Token::Addressable) ==
      Token::Addressable);
  EXPECT(
      Lexicon::get_keyword("value"_view, Token::Addressable) ==
      Token::Addressable);
  EXPECT(Lexicon::is_whitespace(' '));
  EXPECT(Lexicon::is_whitespace('\n'));
  EXPECT(Lexicon::is_whitespace('\r'));
  EXPECT(Lexicon::is_whitespace('\t'));
  EXPECT_NOT(Lexicon::is_whitespace('\0'));
  EXPECT_EQ(Lexicon::get_hex_value('0'), Unsigned_8(0));
  EXPECT_EQ(Lexicon::get_hex_value('9'), Unsigned_8(9));
  EXPECT_EQ(Lexicon::get_hex_value('A'), Unsigned_8(10));
  EXPECT_EQ(Lexicon::get_hex_value('F'), Unsigned_8(15));
  EXPECT_EQ(Lexicon::get_hex_value('a'), Unsigned_8(10));
  EXPECT_EQ(Lexicon::get_hex_value('f'), Unsigned_8(15));
}

PERIMORTEM_UNIT_TEST(TtxLexical, spelling_validation) {
  using Token = Code::Type;

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

  EXPECT(Lexicon::validate(Token::Type, "Type"_view));
  EXPECT(Lexicon::validate(Token::Type, "Type_Name2"_view));
  EXPECT_NOT(Lexicon::validate(Token::Type, View::Bytes()));
  EXPECT_NOT(Lexicon::validate(Token::Type, "type"_view));
  EXPECT_NOT(Lexicon::validate(Token::Type, "Type.Name"_view));

  EXPECT(Lexicon::validate(Token::Type, "Scenes::Splash"_view, type_access));
  EXPECT(Lexicon::validate(Token::Type, "Perimortem.Graphics"_view, address));
  EXPECT(Lexicon::validate(Token::Type, "Root::Child.Leaf"_view, qualified));
  EXPECT_NOT(Lexicon::validate(Token::Type, "::Splash"_view, type_access));
  EXPECT_NOT(Lexicon::validate(Token::Type, "Scenes::"_view, type_access));
  EXPECT_NOT(
      Lexicon::validate(Token::Type, "Scenes::::Splash"_view, type_access));
  EXPECT_NOT(
      Lexicon::validate(Token::Type, "Scenes :: Splash"_view, type_access));

  EXPECT(Lexicon::validate(Token::Addressable, "local_name"_view));
  EXPECT_NOT(Lexicon::validate(Token::Addressable, "public"_view));
  EXPECT(Lexicon::validate(Token::Public, "public"_view));
  EXPECT_NOT(Lexicon::validate(Token::Addressable, "emit"_view));
  EXPECT(Lexicon::validate(Token::Emit, "emit"_view));
  EXPECT(Lexicon::validate(Token::Numeric, "123"_view));
  EXPECT_NOT(Lexicon::validate(Token::Numeric, "1.2"_view));
  EXPECT(Lexicon::validate(Token::Float, "1.25"_view));
  EXPECT_NOT(Lexicon::validate(Token::Float, "1.2.5"_view));
  EXPECT(Lexicon::validate(Token::Hex, "0xAB"_view));
  EXPECT_NOT(Lexicon::validate(Token::Hex, "0x"_view));

  EXPECT(Lexicon::validate(Token::String, "\"1.0\""_view));
  EXPECT(Lexicon::validate(Token::String, "\"escaped \\\" quote\""_view));
  EXPECT_NOT(Lexicon::validate(Token::String, "\"unterminated"_view));
  EXPECT_NOT(Lexicon::validate(Token::String, "\"escaped terminal\\\""_view));
  EXPECT(Lexicon::validate(Token::Bytes, "0x[]"_view));
  EXPECT(Lexicon::validate(Token::Embedded, "$[resources/table.bin]"_view));
  EXPECT(Lexicon::validate(Token::Comment, "// text"_view));
  EXPECT_NOT(Lexicon::validate(Token::Comment, "// line\n"_view));
  EXPECT(Lexicon::validate(Token::TypeAccessOp, "::"_view));
  EXPECT(Lexicon::validate(Token::QuestionOp, "?"_view));
  EXPECT_NOT(Lexicon::validate(Token::QuestionOp, "!"_view));
  EXPECT_NOT(Lexicon::validate(Token::Unknown, "?"_view));
}

PERIMORTEM_UNIT_TEST(TtxLexical, code_semantics) {
  using Code = Code;

  EXPECT_TEXT(
      Code(Code::Type::Public).get_semantics(),
      "public publication modifier"_view);
  EXPECT_TEXT(
      Code(Code::Type::Assign).get_semantics(), "assignment operator"_view);
  EXPECT_TEXT(
      Code(Code::Type::QuestionOp).get_semantics(),
      "propagation operator"_view);
  EXPECT_TEXT(Code(Code::Type::Emit).get_semantics(), "emission keyword"_view);
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
  Tokenizer tokenizer(arena, "0xAB"_view, "test.ttx"_view);

  View::Vector<Token> tokens = tokenizer.get_tokens();
  ASSERT_EQ(tokens.get_size(), Count(2));
  EXPECT(tokens.get_data()[0].get_code() == Code::Type::Hex);
  EXPECT_TEXT(
      tokens.get_data()[0].caculate_text(tokenizer.get_source_text()),
      "0xAB"_view);
  EXPECT(tokens.get_data()[1].get_code() == Code::Type::Terminal);
}

PERIMORTEM_UNIT_TEST(TtxLexical, terminal_boundary) {
  Allocator::Arena arena;
  Token empty;
  Tokenizer tokenizer(arena, "one two"_view, "test.ttx"_view);
  View::Vector<Token> tokens = tokenizer.get_tokens();
  Token terminal = tokens.get_data()[tokens.get_size() - 1];

  EXPECT(empty.get_code() == Code::Type::Terminal);
  EXPECT(empty.get_offset() == 0);
  EXPECT(empty.get_line() == 0);
  EXPECT(empty.get_column() == 0);
  EXPECT(empty.get_size() == 0);
  EXPECT(terminal.get_code() == Code::Type::Terminal);
  EXPECT(terminal.get_offset() == tokenizer.get_source_text().get_size());
  EXPECT(terminal.get_size() == 0);
}

PERIMORTEM_UNIT_TEST(TtxLexical, cursor_consume) {
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, "value"_view, "test.ttx"_view);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "value"_view);
  cursor.consume();
  EXPECT(cursor.matches(Code::Type::Terminal));
  cursor.consume();
  EXPECT(cursor.matches(Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(TtxLexical, cursor_completed_span) {
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, "one two"_view, "test.ttx"_view);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  EXPECT(!cursor.peek(-1));
  EXPECT_TEXT(
      cursor.peek(1).caculate_text(cursor.get_source_text()), "two"_view);
  EXPECT(!cursor.peek(3));

  Token opening = cursor.consume();
  Span first(opening, cursor.peek(-1));
  cursor.consume();
  Span complete(opening, cursor.peek(-1));
  cursor.consume();
  Span at_terminal(opening, cursor.peek(-1));

  EXPECT_TEXT(first.caculate_text(cursor.get_source_text()), "one"_view);
  EXPECT_TEXT(complete.caculate_text(cursor.get_source_text()), "one two"_view);
  EXPECT_TEXT(
      at_terminal.caculate_text(cursor.get_source_text()), "one two"_view);
  EXPECT(complete.get_end().get_code() == Code::Type::Addressable);
  EXPECT(at_terminal.get_end().get_code() == Code::Type::Addressable);
  EXPECT(!cursor.peek(-3));
  EXPECT(!cursor.peek(1));
}

PERIMORTEM_UNIT_TEST(TtxLexical, require_success) {
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, "value"_view, "test.ttx"_view);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  Token token =
      cursor.require(Code::Type::Addressable, "Expected address."_view);

  ASSERT(token.is_valid());
  EXPECT_TEXT(token.caculate_text(cursor.get_source_text()), "value"_view);
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Code::Type::Terminal));
}

PERIMORTEM_UNIT_TEST(TtxLexical, require_error) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Errors errors;
  Tokenizer tokenizer(arena, "value"_view, "test.ttx"_view);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  Token required = cursor.require(Code::Type::Type, "Expected type."_view);
  View::Bytes rendered = errors.render_message(render_arena, 0);

  EXPECT_NOT(required.is_valid());
  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT_EQ(cursor.get_error_count(), Count(1));
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

PERIMORTEM_UNIT_TEST(TtxLexical, span_ordering) {
  Token early(4, 2, 3, 2, Code::Type::Type);
  Token late(304, 8, 12, 4, Code::Type::Type);
  Span span(late, early);
  Span invalid;

  EXPECT_NOT(invalid);
  EXPECT(span);
  EXPECT(span.get_start().get_offset() == early.get_offset());
  EXPECT(span.get_end().get_offset() == late.get_offset());
  EXPECT_EQ(span.get_line_count(), Unsigned_16(7));
  EXPECT_EQ(span.get_size(), Count(304));
}

PERIMORTEM_UNIT_TEST(TtxLexical, range_error) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Errors errors;
  Tokenizer tokenizer(arena, "value = 1"_view, "test.ttx"_view);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  Token start = cursor.current();
  cursor.consume();
  cursor.consume();
  Token end = cursor.current();
  cursor.create_expression_error(
      Span(start, end), "Bad expression."_view, "Use a value."_view);
  View::Bytes rendered = errors.render_message(render_arena, 0);

  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(Algorithm::search(rendered, "Bad expression."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Use a value."_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLexical, source_error) {
  Allocator::Arena render_arena;
  Errors errors;

  {
    Errors::Report report(
        errors, "test.ttx"_view, "source"_view, Anchor::create(Span()));
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
  Errors errors;

  {
    Errors::Report empty(
        errors, "empty.ttx"_view, View::Bytes(), Anchor::create(Span()));
  }
  EXPECT(errors.is_empty());

  {
    Allocator::Arena source_arena;
    Tokenizer tokenizer(source_arena, "report source"_view, "report.ttx"_view);
    Token token = tokenizer.get_tokens().get_data()[0];
    Errors::Report report(
        errors, tokenizer.get_source_path(), tokenizer.get_source_text(),
        Anchor::create(Span(token)));

    report << "Scoped report "_view << Count(7) << "."_view;
    report.get_hint() << "Use the retained message."_view;
  }

  {
    Allocator::Arena replacement_arena;
    Tokenizer replacement(
        replacement_arena, "replacement source"_view, "report.ttx"_view);
    Errors::Report report(
        errors, replacement.get_source_path(), replacement.get_source_text(),
        Anchor::create(Span(replacement.get_tokens().get_data()[0])));
    report << "Repeated report."_view;
  }

  {
    Errors::Report report(
        errors, "outer.ttx"_view, "outer source"_view, Anchor::create(Span()));
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
  Errors errors;

  {
    Allocator::Arena outer_arena;
    Tokenizer outer_tokenizer(
        outer_arena, "outer start finish"_view, "outer.ttx"_view);
    Ttx::Lexical::Associations outer_associations(outer_tokenizer.get_arena());
    Cursor outer(outer_tokenizer, errors, outer_associations);

    {
      Allocator::Arena inner_arena;
      Tokenizer inner_tokenizer(
          inner_arena, "inner source"_view, "inner.ttx"_view);
      Ttx::Lexical::Associations inner_associations(
          inner_tokenizer.get_arena());
      Cursor inner(inner_tokenizer, errors, inner_associations);

      outer.create_token_error(
          "Outer while inner is live."_view, "Outer token hint."_view);
      inner.create_token_error(
          "Inner while nested."_view, "Inner token hint."_view);
    }

    Token start = outer.current();
    outer.consume();
    outer.consume();
    Token end = outer.current();
    outer.create_expression_error(
        Span(start, end), "Outer after inner."_view, "Outer range hint."_view);
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
  EXPECT(Algorithm::search(first, "^----\n"_view) != Count(-1));
  EXPECT(Algorithm::search(first, "inner.ttx"_view) == Count(-1));

  EXPECT(Algorithm::search(second, "inner.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(second, "inner source"_view) != Count(-1));
  EXPECT(Algorithm::search(second, "Inner while nested."_view) != Count(-1));
  EXPECT(Algorithm::search(second, "Inner token hint."_view) != Count(-1));
  EXPECT(Algorithm::search(second, "^----\n"_view) != Count(-1));
  EXPECT(Algorithm::search(second, "outer.ttx"_view) == Count(-1));

  EXPECT(Algorithm::search(third, "outer.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(third, "outer start finish"_view) != Count(-1));
  EXPECT(Algorithm::search(third, "Outer after inner."_view) != Count(-1));
  EXPECT(Algorithm::search(third, "Outer range hint."_view) != Count(-1));
  EXPECT(Algorithm::search(third, "^----\n"_view) != Count(-1));
  EXPECT(Algorithm::search(third, "inner.ttx"_view) == Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLexical, wrapper_messages) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Errors errors;
  Tokenizer tokenizer(arena, "first second third"_view, "wrappers.ttx"_view);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  cursor.create_error("General wrapper."_view, "General hint."_view);
  cursor.create_token_error("Token wrapper."_view, "Token hint."_view);
  Token start = cursor.current();
  cursor.consume();
  Token end = cursor.current();
  cursor.create_expression_error(
      Span(start, end), "Expression wrapper."_view, "Expression hint."_view);

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

PERIMORTEM_UNIT_TEST(TtxLexical, cursor_report) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Errors errors;
  Tokenizer tokenizer(arena, "first second"_view, "cursor-report.ttx"_view);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  Token first = cursor.consume();
  Token second = cursor.current();
  {
    auto report = cursor.create_report(Span(first, second));
    report << "Semantic report."_view;
  }

  View::Bytes rendered = errors.render_message(render_arena, 0);

  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(
      Algorithm::search(rendered, "cursor-report.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "first second"_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Semantic report."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "^----\n"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLexical, reversed_range) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Errors errors;
  Tokenizer tokenizer(arena, "one two three"_view, "reversed.ttx"_view);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  Token early = cursor.current();
  cursor.consume();
  cursor.consume();
  Token late = cursor.current();
  Span span(late, early);
  cursor.create_expression_error(
      span, "Reversed range."_view, "Order the range."_view);
  View::Bytes rendered = errors.render_message(render_arena, 0);

  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(span.get_start().get_offset() == early.get_offset());
  EXPECT(span.get_end().get_offset() == late.get_offset());
  EXPECT(Algorithm::search(rendered, "reversed.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Reversed range."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Order the range."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "^--\n"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLexical, anchor_selects_operator_caret) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Errors errors;
  Tokenizer tokenizer(arena, "5 == true"_view, "anchor.ttx"_view);
  const Token* tokens = tokenizer.get_tokens().get_data();
  Span expression(tokens[0], tokens[2]);

  {
    Errors::Report report(
        errors, tokenizer.get_source_path(), tokenizer.get_source_text(),
        Anchor::create(tokens[1], expression));
    report << "Equal rejects the right operand."_view;
  }

  View::Bytes rendered = errors.render_message(render_arena, 0);

  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(Algorithm::search(rendered, "anchor.ttx:1:3"_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "5 == true"_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "^-\n"_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "^--------\n"_view) == Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLexical, external_anchor_omits_caret) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Errors errors;
  Tokenizer tokenizer(arena, "5 + true"_view, "anchor.ttx"_view);
  const Token* tokens = tokenizer.get_tokens().get_data();
  Span expression(tokens[0], tokens[2]);
  Token external(40, 7, 4, 1, Code::Type::AddOp);

  {
    Errors::Report report(
        errors, tokenizer.get_source_path(), tokenizer.get_source_text(),
        Anchor::create(Token(), expression));
    report << "Empty focus."_view;
  }

  {
    Errors::Report report(
        errors, tokenizer.get_source_path(), tokenizer.get_source_text(),
        Anchor::create(external, expression));
    report << "External focus."_view;
  }

  View::Bytes empty = errors.render_message(render_arena, 0);
  View::Bytes outside = errors.render_message(render_arena, 1);

  ASSERT_EQ(errors.get_size(), Count(2));
  EXPECT(Algorithm::search(empty, "anchor.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(empty, "5 + true"_view) != Count(-1));
  EXPECT(Algorithm::search(empty, "^"_view) == Count(-1));
  EXPECT(Algorithm::search(outside, "anchor.ttx:1:1"_view) != Count(-1));
  EXPECT(Algorithm::search(outside, "5 + true"_view) != Count(-1));
  EXPECT(Algorithm::search(outside, "^"_view) == Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLexical, retained_source_name) {
  Allocator::Arena render_arena;
  Errors errors;

  {
    Allocator::Arena source_arena;
    Tokenizer tokenizer(source_arena, "first value"_view, "test.ttx"_view);
    Ttx::Lexical::Associations associations(tokenizer.get_arena());
    Cursor cursor(tokenizer, errors, associations);
    cursor.create_token_error("First failure."_view);
  }

  {
    Allocator::Arena source_arena;
    Tokenizer tokenizer(source_arena, "second value"_view, "test.ttx"_view);
    Ttx::Lexical::Associations associations(tokenizer.get_arena());
    Cursor cursor(tokenizer, errors, associations);
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
  Errors errors;
  Tokenizer tokenizer(arena, "one\ntwo\nthree value"_view, "test.ttx"_view);
  Token token = tokenizer.get_tokens().get_data()[3];

  {
    Errors::Report report(
        errors, tokenizer.get_source_path(), tokenizer.get_source_text(),
        Anchor::create(Span(token)));
    report << "Bad token."_view;
  }

  View::Bytes rendered = errors.render_message(render_arena, 0);

  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(Algorithm::search(rendered, "test.ttx:3:7"_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Bad token."_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLexical, recover_stmt) {
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, "bad tokens ; next"_view, "test.ttx"_view);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  cursor.recover_to_statement();

  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "next"_view);
}

PERIMORTEM_UNIT_TEST(TtxLexical, token_projection) {
  Allocator::Arena arena;
  Tokenizer tokenizer(arena, "one two three"_view, "test.ttx"_view);
  Token token = tokenizer.get_tokens().get_data()[1];

  EXPECT_TEXT(token.caculate_text(tokenizer.get_source_text()), "two"_view);
  EXPECT_TEXT(token.caculate_text("red sky green"_view), "sky"_view);
}
