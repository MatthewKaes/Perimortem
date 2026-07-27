// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/file.hpp"

#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness PackageFixtureTokens = {
  .name = "Ttx::Lexical::PackageFixtures"_view,
};

PERIMORTEM_UNIT_TEST(PackageFixtureTokens, tokenize_completely) {
  static constexpr Static::Vector<View::Bytes, 10> paths = {{
    "apps/ttx/scene_lifetime/package.ttx"_view,
    "apps/ttx/scene_lifetime/main.ttx"_view,
    "apps/ttx/scene_lifetime/scenes/splash.ttx"_view,
    "apps/ttx/scene_lifetime/scenes/title.ttx"_view,
    "validation/data/ttx/package/duplicate_source.ttx"_view,
    "validation/data/ttx/package/float_version.ttx"_view,
    "validation/data/ttx/package/noncanonical_version.ttx"_view,
    "validation/data/ttx/package_resources/package.ttx"_view,
    "validation/data/ttx/package_resources/shared_a.ttx"_view,
    "validation/data/ttx/package_resources/shared_b.ttx"_view,
  }};

  for (Count path_index = 0; path_index < paths.get_size(); path_index++) {
    View::Bytes path = paths[path_index];
    auto source = File::read(path);
    ASSERT(!source.is_empty());

    Allocator::Arena arena;
    Tokenizer tokenizer(arena, source, path);
    View::Vector<Token> tokens = tokenizer.get_tokens();
    ASSERT(tokens.get_size() > 1);
    EXPECT(tokens[tokens.get_size() - 1].get_code() == Code::Type::Terminal);

    for (Count index = 0; index + 1 < tokens.get_size(); index++) {
      EXPECT(tokens[index].get_code() != Code::Type::Terminal);
      EXPECT(tokens[index].get_code() != Code::Type::Unknown);
    }
  }
}

PERIMORTEM_UNIT_TEST(PackageFixtureTokens, resource_runfiles) {
  static constexpr View::Bytes table_path =
      "validation/data/ttx/package_resources/resources/table.bin"_view;
  static constexpr View::Bytes empty_path =
      "validation/data/ttx/package_resources/resources/empty.bin"_view;
  static constexpr View::Bytes logo_path =
      "apps/ttx/scene_lifetime/resources/logo.png"_view;
  static constexpr View::Bytes icon_path =
      "apps/ttx/scene_lifetime/resources/icon.png"_view;
  static constexpr View::Bytes expected_header =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/"_view;

  auto table = File::read(table_path);
  ASSERT(File::exists(table_path));
  ASSERT(table.get_size() >= expected_header.get_size());
  EXPECT(
      table.get_view().slice(0, expected_header.get_size()) == expected_header);

  EXPECT(File::exists(empty_path));
  EXPECT(File::read(empty_path).is_empty());
  EXPECT(File::exists(logo_path));
  EXPECT_NOT(File::read(logo_path).is_empty());
  EXPECT(File::exists(icon_path));
  EXPECT_NOT(File::read(icon_path).is_empty());
}
