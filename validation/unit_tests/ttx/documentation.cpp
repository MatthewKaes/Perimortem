// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/comment.hpp"
#include "ttx/concept/comments.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Validation;

static Harness TtxDocumentation = {
  .name = "TTX::Documentation"_view,
};

PERIMORTEM_UNIT_TEST(TtxDocumentation, empty) {
  const Comment& empty = Comment::get_empty();
  const Documentation& documentation = empty;

  EXPECT(&empty == &Comment::get_empty());
  EXPECT(documentation.is_empty());
  EXPECT_EQ(documentation.line_count(), Count(0));
  EXPECT(documentation.get_line(0).is_empty());
}

PERIMORTEM_UNIT_TEST(TtxDocumentation, lines) {
  static constexpr Static::Vector<View::Bytes, 2> lines = {{
    "First"_view,
    "Second"_view,
  }};

  Comments documentation(lines);

  EXPECT_NOT(documentation.is_empty());
  EXPECT_EQ(documentation.line_count(), Count(2));
  EXPECT_TEXT(documentation.get_line(0), "First"_view);
  EXPECT_TEXT(documentation.get_line(1), "Second"_view);
  EXPECT(documentation.get_line(2).is_empty());
}

PERIMORTEM_UNIT_TEST(TtxDocumentation, empty_line) {
  static constexpr Static::Vector<View::Bytes, 2> lines = {{
    View::Bytes(),
    "Second"_view,
  }};

  Comments documentation(lines);

  EXPECT_NOT(documentation.is_empty());
  EXPECT(documentation.get_line(0).is_empty());
  EXPECT_TEXT(documentation.get_line(1), "Second"_view);
}

PERIMORTEM_UNIT_TEST(TtxDocumentation, comment) {
  Comment documentation("Stable"_view);

  EXPECT_EQ(documentation.line_count(), Count(1));
  EXPECT_TEXT(documentation.get_line(0), "Stable"_view);
  EXPECT(documentation.get_line(1).is_empty());
}

PERIMORTEM_UNIT_TEST(TtxDocumentation, late_bound) {
  Static::Vector<View::Bytes, 2> lines = {{
    "Initial"_view,
    "Second"_view,
  }};

  Comments documentation(lines);
  lines[0] = "Updated"_view;

  EXPECT_TEXT(documentation.get_line(0), "Updated"_view);
  EXPECT_TEXT(documentation.get_line(1), "Second"_view);
}
