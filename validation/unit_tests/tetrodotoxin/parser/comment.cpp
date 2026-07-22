// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/parser/comment.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/option.hpp"

#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/documentations/block.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Ttx;
using namespace Validation;

static Harness ParserCommentTests = {
  .name = "Tetrodotoxin::Parser::Comment"_view,
};

using ParsedComment = Option<const Ttx::Model::Documentations::Block&>;

static auto is_none(const ParsedComment& parsed) -> Bool {
  return parsed.visit(
      [](const None&) { return True; },
      [](const Ttx::Model::Documentations::Block&) { return False; });
}

PERIMORTEM_UNIT_TEST(ParserCommentTests, leaves_cursor_when_comment_is_absent) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  Lexical::Tokenizer tokenizer(arena, "Value"_view, "<no comment>"_view);
  Lexical::Cursor cursor(tokenizer, errors);

  ParsedComment parsed = Parser::Comment::parse(cursor);

  EXPECT(is_none(parsed));
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

  ParsedComment parsed = Parser::Comment::parse(cursor);
  Bool correct = parsed.visit(
      [](const None&) { return False; },
      [](const Ttx::Model::Documentations::Block& document) {
        return Bool(
            document.line_count() == 2 &&
            document.get_line(0) == "First line"_view &&
            document.get_line(1) == "Second line"_view);
      });

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

  ParsedComment parsed = Parser::Comment::parse(cursor);
  Bool correct = parsed.visit(
      [](const None&) { return False; },
      [](const Ttx::Model::Documentations::Block& document) {
        return Bool(
            document.line_count() == 4 &&
            document.get_line(0) == "First line"_view &&
            document.get_line(1).is_empty() &&
            document.get_line(2).is_empty() &&
            document.get_line(3) == " Indented line"_view);
      });

  EXPECT(correct);
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Lexical::Code::Type::Type));
}
