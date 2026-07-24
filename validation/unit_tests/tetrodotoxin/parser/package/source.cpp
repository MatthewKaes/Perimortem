// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/parser/package/source.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/version.hpp"

#include "perimortem/utility/option.hpp"

#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness TetrodotoxinParserPackageTests = {
  .name = "Tetrodotoxin::Parser::Package::Source"_view,
};

static auto parse_package(
    Allocator::Arena& arena,
    View::Bytes source,
    View::Bytes path,
    Errors& errors) -> Option<const Model::Package::Source&> {
  Tokenizer tokenizer(arena, source, path);
  Cursor cursor(tokenizer, errors);
  return Parser::Package::Source::parse(cursor);
}

static auto has_value(const Option<const Model::Package::Source&>& parsed)
    -> Bool {
  return parsed.visit(
      [](const None&) { return False; },
      [](const Model::Package::Source&) { return True; });
}

static auto error_contains(
    Errors& errors,
    Allocator::Arena& render_arena,
    View::Bytes text) -> Bool {
  if (errors.get_size() == 0) {
    return False;
  }

  View::Bytes message = errors.render_message(render_arena, 0);
  return Algorithm::search(message, text) != Count(-1);
}

PERIMORTEM_UNIT_TEST(
    TetrodotoxinParserPackageTests,
    parses_documented_ordered_package) {
  constexpr View::Bytes source =
      "// Package documentation.\n"
      "//\n"
      "// Preserves authored order.\n"
      "dialect : Package;\n"
      "resolve Math : Perimortem.Math = \"1.0\";\n"
      "resolve Graphics : Perimortem.Graphics = \"12.34\";\n"
      "source \"scenes/./splash.ttx\";\n"
      "source \"scenes/../main.ttx\";\n"_view;
  Allocator::Arena arena;
  Errors errors;

  Option<const Model::Package::Source&> parsed =
      parse_package(arena, source, "package.ttx"_view, errors);
  Bool selected = parsed.visit(
      [](const None&) { return False; },
      [&](const Model::Package::Source& package) {
        EXPECT_EQ(package.get_documentation().line_count(), Count(3));
        EXPECT_TEXT(
            package.get_documentation().get_line(0),
            "Package documentation."_view);
        EXPECT(package.get_documentation().get_line(1).is_empty());
        EXPECT_TEXT(
            package.get_documentation().get_line(2),
            "Preserves authored order."_view);
        EXPECT_EQ(package.get_resolutions().get_size(), Count(2));
        EXPECT_TEXT(package.get_resolutions()[0].get_local_name(), "Math"_view);
        EXPECT_TEXT(
            package.get_resolutions()[0].get_package_name(),
            "Perimortem.Math"_view);
        EXPECT(package.get_resolutions()[0].get_version() == Version(1, 0));
        EXPECT_TEXT(
            package.get_resolutions()[1].get_local_name(), "Graphics"_view);
        EXPECT(package.get_resolutions()[1].get_version() == Version(12, 34));
        EXPECT_EQ(package.get_members().get_size(), Count(2));
        EXPECT_TEXT(package.get_members()[0], "scenes/splash.ttx"_view);
        EXPECT_TEXT(package.get_members()[1], "main.ttx"_view);
        return True;
      });

  ASSERT(selected);
  ASSERT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(
    TetrodotoxinParserPackageTests,
    rejects_an_unprocessed_package_body) {
  constexpr View::Bytes source =
      "// Package documentation.\n"
      "dialect : Package;\n"
      "{\n"
      "  public Types;\n"
      "}\n"_view;
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Errors errors;

  Option<const Model::Package::Source&> parsed =
      parse_package(arena, source, "package.ttx"_view, errors);

  EXPECT_NOT(has_value(parsed));
  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(error_contains(
      errors, render_arena,
      "require direct Package-Dialect semantic evaluation"_view));
}

PERIMORTEM_UNIT_TEST(
    TetrodotoxinParserPackageTests,
    parses_frozen_package_fixtures) {
  constexpr Static::Vector<View::Bytes, 2> paths = {{
    "validation/data/ttx/package_resources/package.ttx"_view,
    "apps/ttx/scene_lifetime/package.ttx"_view,
  }};
  constexpr Static::Vector<Count, 2> resolution_counts = {{0, 3}};
  constexpr Static::Vector<Count, 2> member_counts = {{2, 3}};

  for (Count i = 0; i < 2; i++) {
    Dynamic::Bytes source = File::read(paths[i]);
    Allocator::Arena arena;
    Errors errors;
    Option<const Model::Package::Source&> parsed =
        parse_package(arena, source, paths[i], errors);
    Bool selected = parsed.visit(
        [](const None&) { return False; },
        [&](const Model::Package::Source& package) {
          EXPECT_EQ(package.get_resolutions().get_size(), resolution_counts[i]);
          EXPECT_EQ(package.get_members().get_size(), member_counts[i]);
          return True;
        });

    EXPECT(selected);
    EXPECT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(
    TetrodotoxinParserPackageTests,
    requires_an_opening_documentation_comment) {
  constexpr View::Bytes source = "dialect : Package;\n"_view;
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Errors errors;

  Option<const Model::Package::Source&> parsed =
      parse_package(arena, source, "package.ttx"_view, errors);

  EXPECT_NOT(has_value(parsed));
  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(error_contains(
      errors, render_arena, "require an opening documentation comment"_view));
  EXPECT(error_contains(
      errors, render_arena,
      "Expected lexical token source comment but got source Dialect selection"_view));
}

PERIMORTEM_UNIT_TEST(
    TetrodotoxinParserPackageTests,
    rejects_duplicate_and_out_of_order_inputs) {
  constexpr Static::Vector<View::Bytes, 3> sources = {{
    "// Test package.\n"
    "dialect : Package;\n"
    "resolve Math : Perimortem.Math = \"1.0\";\n"
    "resolve Math : Other.Math = \"2.0\";\n"_view,
    "// Test package.\n"
    "dialect : Package;\n"
    "source \"nested/../member.ttx\";\n"
    "source \"member.ttx\";\n"_view,
    "// Test package.\n"
    "dialect : Package;\n"
    "source \"member.ttx\";\n"
    "resolve Math : Perimortem.Math = \"1.0\";\n"_view,
  }};
  constexpr Static::Vector<View::Bytes, 3> messages = {{
    "aliases must be unique"_view,
    "routes must be unique"_view,
    "must precede member Source"_view,
  }};

  for (Count i = 0; i < 3; i++) {
    Allocator::Arena arena;
    Allocator::Arena render_arena;
    Errors errors;
    Option<const Model::Package::Source&> parsed =
        parse_package(arena, sources[i], "package.ttx"_view, errors);

    EXPECT_NOT(has_value(parsed));
    EXPECT_EQ(errors.get_size(), Count(1));
    EXPECT(error_contains(errors, render_arena, messages[i]));
  }
}

PERIMORTEM_UNIT_TEST(
    TetrodotoxinParserPackageTests,
    rejects_invalid_routes_versions_and_dialects) {
  constexpr Static::Vector<View::Bytes, 5> sources = {{
    "// Test package.\n"
    "dialect : Package;\nsource \"/absolute.ttx\";\n"_view,
    "// Test package.\n"
    "dialect : Package;\nsource \"../../outside.ttx\";\n"_view,
    "// Test package.\n"
    "dialect : Package;\n"
    "resolve Math : Perimortem.Math = 1.2;\n"_view,
    "// Test package.\n"
    "dialect : Package;\n"
    "resolve Math : Perimortem.Math = \"01.2\";\n"_view,
    "// Test package.\n"
    "dialect : Library;\nsource \"member.ttx\";\n"_view,
  }};
  constexpr Static::Vector<View::Bytes, 5> messages = {{
    "stay beneath the package root"_view,
    "stay beneath the package root"_view,
    "quoted canonical Major.Minor"_view,
    "canonical non-null package version"_view,
    "must select the Package Dialect"_view,
  }};

  for (Count i = 0; i < 5; i++) {
    Allocator::Arena arena;
    Allocator::Arena render_arena;
    Errors errors;
    Option<const Model::Package::Source&> parsed =
        parse_package(arena, sources[i], "package.ttx"_view, errors);

    EXPECT_NOT(has_value(parsed));
    EXPECT_EQ(errors.get_size(), Count(1));
    EXPECT(error_contains(errors, render_arena, messages[i]));
  }
}
