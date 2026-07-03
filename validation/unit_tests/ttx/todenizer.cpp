// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Validation;

static Harness TtxParse = {
  .name = "Ttx::Parse"_view,
};

PERIMORTEM_UNIT_TEST(TtxParse, access_operators) {
  Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(
      arena,
      "TTX::Graphics color.[r, g] color:[start, 2] layout[Type] value.member"_view,
      "Test::Package"_view);

  View::Vector<Ttx::Lexical::Token> tokens = tokenizer.get_tokens();
  ASSERT_EQ(tokens.get_size(), Count(23));

  EXPECT(tokens[0].get_class() == Ttx::Lexical::Class::Type::Type);
  EXPECT(tokens[1].get_class() == Ttx::Lexical::Class::Type::TypeAccessOp);
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
