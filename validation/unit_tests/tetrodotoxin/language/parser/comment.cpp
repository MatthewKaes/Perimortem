// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/language/parser/comment.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Language;
using namespace Ttx;
using namespace Validation;

static Harness ParserCommentTests = {
  .name = "Tetrodotoxin::Language::Parser::Comment"_view,
};

PERIMORTEM_UNIT_TEST(ParserCommentTests, greedy) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  Lexical::Tokenizer tokenizer(
      arena, "// First line\n// Second line\nValue"_view,
      "<greedy comments>"_view);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Lexical::Cursor cursor(tokenizer, errors, associations);

  Option<const Concept::Documentation&> documentation =
      Parser::Comment::parse(cursor);
  Bool correct = documentation.visit(
      []() { return False; },
      [](const Concept::Documentation& selected) -> Bool {
        return Bool(
            selected.line_count() == 2 &&
            selected.get_line(0) == "First line"_view &&
            selected.get_line(1) == "Second line"_view);
      });

  EXPECT(correct);
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Lexical::Code::Type::Type));
  EXPECT_TEXT(
      cursor.current().caculate_text(cursor.get_source_text()), "Value"_view);
}

PERIMORTEM_UNIT_TEST(ParserCommentTests, preserves_empty) {
  Allocator::Arena arena;
  Lexical::Errors errors;
  Lexical::Tokenizer tokenizer(
      arena, "// First line\n//\n// \n//  Indented line\nValue"_view,
      "<empty comment lines>"_view);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Lexical::Cursor cursor(tokenizer, errors, associations);

  Option<const Concept::Documentation&> documentation =
      Parser::Comment::parse(cursor);
  Bool correct = documentation.visit(
      []() { return False; },
      [](const Concept::Documentation& selected) -> Bool {
        return Bool(
            selected.line_count() == 4 &&
            selected.get_line(0) == "First line"_view &&
            selected.get_line(1).is_empty() &&
            selected.get_line(2).is_empty() &&
            selected.get_line(3) == " Indented line"_view);
      });

  EXPECT(correct);
  EXPECT(errors.is_empty());
  EXPECT(cursor.matches(Lexical::Code::Type::Type));
}
