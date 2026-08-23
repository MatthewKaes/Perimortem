// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/formatting/terminal.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness FormattingTerminal = {
  .name = "Tetrodotoxin::Formatting::Terminal"_view,
};

PERIMORTEM_UNIT_TEST(FormattingTerminal, documentation_reflow) {
  static constexpr View::Bytes source =
      "// This source documentation was intentionally split\n"
      "// across two short authored lines.\n"
      "//\n"
      "// A second paragraph remains separate.\n"
      "dialect : Library;\n"
      "public value : U64;\n"_view;
  auto toolchain = create_library_toolchain();
  Environment::Workspace workspace(*toolchain);
  Errors errors;
  auto monograph = workspace.interpret_source(
      errors, "Formatting"_view, "formatting.ttx"_view, source);
  ASSERT(monograph);
  ASSERT(errors.is_empty());

  Allocator::Arena arena;
  Tokenizer tokenizer(arena, source, "formatting.ttx"_view);
  Dynamic::Bytes formatted =
      Formatting::Terminal::format(*monograph, tokenizer);
  EXPECT(
      Algorithm::search(
          formatted.get_view(),
          "// This source documentation was intentionally split across two "
          "short authored lines.\n"
          "//\n"
          "// A second paragraph remains separate."_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(FormattingTerminal, markdown_heading_boundary) {
  static constexpr View::Bytes source =
      "// # Tetrodotoxin\n"
      "// Copyright (c) 2023-present Matt Kaes and contributors\n"
      "//\n"
      "// Describes one source while leaving its legal notice untouched.\n"
      "//\n"
      "dialect : Library;\n"
      "public value : U64;\n"_view;
  auto toolchain = create_library_toolchain();
  Environment::Workspace workspace(*toolchain);
  Errors errors;
  auto monograph = workspace.interpret_source(
      errors, "MarkdownHeadingFormatting"_view, "heading.ttx"_view, source);
  ASSERT(monograph);
  ASSERT(errors.is_empty());

  Allocator::Arena arena;
  Tokenizer tokenizer(arena, source, "heading.ttx"_view);
  Dynamic::Bytes formatted =
      Formatting::Terminal::format(*monograph, tokenizer);
  static constexpr View::Bytes header =
      "// # Tetrodotoxin\n"
      "// Copyright (c) 2023-present Matt Kaes and contributors\n"
      "//\n"_view;
  EXPECT(formatted.get_view().slice(0, header.get_size()) == header);
}
