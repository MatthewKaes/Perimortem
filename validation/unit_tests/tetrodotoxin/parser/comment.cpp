// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/parser/comment.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Ttx;
using namespace Validation;

static Harness ParserCommentTests = {
  .name = "Tetrodotoxin::Parser::Comment"_view,
};

PERIMORTEM_UNIT_TEST(ParserCommentTests, leaves_cursor_when_comment_is_absent) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  Lexical::Tokenizer tokenizer(arena, "Value"_view, "<no comment>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  const Concept::Documentation& documentation = Parser::Comment::parse(cursor);

  EXPECT(documentation.is_empty());
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Lexical::Code::Type::Type));
  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "Value"_view);
}

PERIMORTEM_UNIT_TEST(ParserCommentTests, consumes_comment_prefix_greedily) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  Lexical::Tokenizer tokenizer(
      arena, "// First line\n// Second line\nValue"_view,
      "<greedy comments>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  const Concept::Documentation& documentation = Parser::Comment::parse(cursor);
  Bool correct = documentation.line_count() == 2 &&
                 documentation.get_line(0) == "First line"_view &&
                 documentation.get_line(1) == "Second line"_view;

  EXPECT(correct);
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Lexical::Code::Type::Type));
  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "Value"_view);
}

PERIMORTEM_UNIT_TEST(ParserCommentTests, preserves_empty_comment_lines) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  Lexical::Tokenizer tokenizer(
      arena, "// First line\n//\n// \n//  Indented line\nValue"_view,
      "<empty comment lines>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  const Concept::Documentation& documentation = Parser::Comment::parse(cursor);
  Bool correct = documentation.line_count() == 4 &&
                 documentation.get_line(0) == "First line"_view &&
                 documentation.get_line(1).is_empty() &&
                 documentation.get_line(2).is_empty() &&
                 documentation.get_line(3) == " Indented line"_view;

  EXPECT(correct);
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Lexical::Code::Type::Type));
}
