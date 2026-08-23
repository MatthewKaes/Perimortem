// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/lexical/anchor.hpp"

#include "validation/unit_test.hpp"

using namespace Ttx::Lexical;
using namespace Validation;

static Harness TtxAnchor = {
  .name = "TTX::Lexical::Anchor"_view,
};

PERIMORTEM_UNIT_TEST(TtxAnchor, focus_contract) {
  Token first(0, 1, 1, 1, Code::Type::Numeric);
  Token operation(4, 1, 5, 1, Code::Type::CmpOp);
  Token middle(4, 1, 5, 1, Code::Type::Numeric);
  Token last(8, 1, 9, 1, Code::Type::Numeric);
  Span outer(first, last);
  Span inner(middle);
  Anchor defaulted = Anchor::create(outer);
  Anchor focused = Anchor::create(operation, outer);
  Anchor empty = Anchor::create(Token(), outer);
  Token external(40, 8, 2, 1, Code::Type::AddOp);
  Anchor outside = Anchor::create(external, outer);

  Anchor reversed = Anchor::create(middle, Span(last), Span(first));
  Anchor nested = Anchor::create(middle, outer, inner);

  EXPECT(defaulted.get_token() == first);
  EXPECT(defaulted.get_span() == outer);
  EXPECT(focused.get_token() == operation);
  EXPECT(focused.get_span() == outer);
  EXPECT_NOT(empty.get_token());
  EXPECT(empty.get_span() == outer);
  EXPECT(outside.get_token() == external);
  EXPECT(outside.get_span() == outer);
  EXPECT(reversed.get_span().get_offset() == first.get_offset());
  EXPECT(reversed.get_span().get_size() == outer.get_size());
  EXPECT(nested.get_span().get_offset() == outer.get_offset());
  EXPECT(nested.get_span().get_size() == outer.get_size());
}
