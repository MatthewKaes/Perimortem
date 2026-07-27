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

static Harness LibraryFixtureTokens = {
  .name = "Ttx::Lexical::LibraryFixtures"_view,
};

PERIMORTEM_UNIT_TEST(LibraryFixtureTokens, tokenize_completely) {
  static constexpr Static::Vector<View::Bytes, 13> paths = {{
    "validation/data/ttx/library/broad.ttx"_view,
    "validation/data/ttx/library/native.ttx"_view,
    "validation/data/ttx/library/foreign_triad.ttx"_view,
    "validation/data/ttx/library/public_parameter_private_type.ttx"_view,
    "validation/data/ttx/library/public_result_private_type.ttx"_view,
    "validation/data/ttx/library/ordinary_bodyless.ttx"_view,
    "validation/data/ttx/library/foreign_with_body.ttx"_view,
    "validation/data/ttx/library/foreign_named_scope.ttx"_view,
    "validation/data/ttx/library/foreign_undeclared_symbol.ttx"_view,
    "validation/data/ttx/library/duplicate_name.ttx"_view,
    "validation/data/ttx/library/new_without_expected_type.ttx"_view,
    "validation/data/ttx/library/bare_return.ttx"_view,
    "validation/data/ttx/library/dialect_led_callable.ttx"_view,
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
