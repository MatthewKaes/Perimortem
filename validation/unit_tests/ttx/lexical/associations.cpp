// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/lexical/associations.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx;
using namespace Validation;

static Harness TtxLexicalAssociations = {
  .name = "TTX::Lexical::Associations"_view,
};

PERIMORTEM_UNIT_TEST(
    TtxLexicalAssociations,
    selects_the_most_precise_semantic_identity) {
  Allocator::Arena arena;
  Lexical::Tokenizer tokenizer(
      arena, "outer inner end"_view, "association.ttx"_view);
  Lexical::Associations associations(arena);
  const Lexical::Token* tokens = tokenizer.get_tokens().get_data();
  Model::Alias outer("Outer"_view, Concept::Invalid::get_invalid());
  Model::Alias inner("Inner"_view, Concept::Invalid::get_invalid());

  associations.create(
      Lexical::Anchor::create(tokens[0], Lexical::Span(tokens[0], tokens[2])),
      outer);
  associations.create(Lexical::Anchor::create(Lexical::Span(tokens[1])), inner);

  auto focused = associations.find_at(tokens[1].get_offset());
  auto containing = associations.find_at(tokens[2].get_offset());
  auto missing = associations.find_at(tokenizer.get_source_text().get_size());
  auto outer_anchor = associations.find(outer);
  auto inner_anchor = associations.find(inner);

  ASSERT(focused);
  ASSERT(containing);
  ASSERT(outer_anchor);
  ASSERT(inner_anchor);
  EXPECT(&*focused == &inner);
  EXPECT(&*containing == &outer);
  EXPECT(outer_anchor->get_token() == tokens[0]);
  EXPECT(inner_anchor->get_token() == tokens[1]);
  EXPECT_NOT(missing);
  EXPECT_NOT(associations.find(Concept::Invalid::get_invalid()));
}
