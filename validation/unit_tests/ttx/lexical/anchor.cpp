// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/lexical/anchor.hpp"

#include "validation/unit_test.hpp"

using namespace Ttx::Lexical;
using namespace Validation;

static Harness TtxAnchor = {
  .name = "TTX::Lexical::Anchor"_view,
};

PERIMORTEM_UNIT_TEST(TtxAnchor, defaults_to_span_start) {
  Token first(0, 1, 1, 1, Code::Type::Numeric);
  Token last(4, 1, 5, 1, Code::Type::Numeric);
  Span span(first, last);
  Anchor anchor = Anchor::create(span);

  EXPECT(anchor.get_token().get_offset() == first.get_offset());
  EXPECT(anchor.get_token().get_code() == first.get_code());
  EXPECT(anchor.get_span().get_offset() == span.get_offset());
  EXPECT(anchor.get_span().get_size() == span.get_size());
}

PERIMORTEM_UNIT_TEST(TtxAnchor, retains_independent_focus) {
  Token first(0, 1, 1, 1, Code::Type::Numeric);
  Token operation(2, 1, 3, 2, Code::Type::CmpOp);
  Token last(7, 1, 8, 4, Code::Type::True);
  Anchor anchor = Anchor::create(operation, Span(first, last));
  Anchor composed = Anchor::create(operation, Span(first), Span(last));

  EXPECT(anchor.get_token().get_offset() == operation.get_offset());
  EXPECT(anchor.get_token().get_size() == operation.get_size());
  EXPECT(anchor.get_span().get_offset() == first.get_offset());
  EXPECT(anchor.get_span().get_size() == Count(11));
  EXPECT(composed.get_span().get_offset() == anchor.get_span().get_offset());
  EXPECT(composed.get_span().get_size() == anchor.get_span().get_size());
}

PERIMORTEM_UNIT_TEST(TtxAnchor, permits_empty_or_external_focus) {
  Token first(4, 2, 3, 2, Code::Type::Numeric);
  Token last(9, 2, 8, 1, Code::Type::Numeric);
  Token external(40, 8, 2, 1, Code::Type::AddOp);
  Span span(first, last);
  Anchor empty = Anchor::create(Token(), span);
  Anchor outside = Anchor::create(external, span);

  EXPECT_NOT(empty.get_token());
  EXPECT(empty.get_span());
  EXPECT(outside.get_token().get_offset() == external.get_offset());
  EXPECT(outside.get_span().get_offset() == span.get_offset());
}

PERIMORTEM_UNIT_TEST(TtxAnchor, composition_covers_both_spans) {
  Token first(0, 1, 1, 1, Code::Type::Numeric);
  Token middle(4, 1, 5, 1, Code::Type::Numeric);
  Token last(8, 1, 9, 1, Code::Type::Numeric);
  Span outer(first, last);
  Span inner(middle);

  Anchor reversed = Anchor::create(middle, Span(last), Span(first));
  Anchor nested = Anchor::create(middle, outer, inner);

  EXPECT(reversed.get_span().get_offset() == first.get_offset());
  EXPECT(reversed.get_span().get_size() == outer.get_size());
  EXPECT(nested.get_span().get_offset() == outer.get_offset());
  EXPECT(nested.get_span().get_size() == outer.get_size());
}
