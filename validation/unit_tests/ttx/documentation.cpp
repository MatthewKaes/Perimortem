// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/concept/documentation.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness TtxDocumentation = {
  .name = "TTX::Documentation"_view,
};

PERIMORTEM_UNIT_TEST(TtxDocumentation, empty) {
  Ttx::Concept::Documentation documentation;

  EXPECT(documentation.is_empty());
  EXPECT_EQ(documentation.get_line_count(), Count(0));
  EXPECT(documentation.line_at(0).is_empty());
}

PERIMORTEM_UNIT_TEST(TtxDocumentation, lines) {
  static constexpr Static::Vector<View::Bytes, 2> lines = {{
    "First"_view,
    "Second"_view,
  }};

  Ttx::Concept::Documentation documentation(lines);

  EXPECT_NOT(documentation.is_empty());
  EXPECT_EQ(documentation.get_line_count(), Count(2));
  EXPECT_TEXT(documentation.line_at(0), "First"_view);
  EXPECT_TEXT(documentation.line_at(1), "Second"_view);
  EXPECT(documentation.line_at(2).is_empty());
}

PERIMORTEM_UNIT_TEST(TtxDocumentation, empty_line) {
  static constexpr Static::Vector<View::Bytes, 2> lines = {{
    View::Bytes(),
    "Second"_view,
  }};

  Ttx::Concept::Documentation documentation(lines);

  EXPECT_NOT(documentation.is_empty());
  EXPECT(documentation.line_at(0).is_empty());
  EXPECT_TEXT(documentation.line_at(1), "Second"_view);
}

PERIMORTEM_UNIT_TEST(TtxDocumentation, view_identity) {
  static constexpr Static::Vector<View::Bytes, 1> lines = {{
    "Stable"_view,
  }};

  Ttx::Concept::Documentation documentation(lines);

  EXPECT(documentation.get_lines().get_data() == lines.get_data());
  EXPECT_EQ(documentation.get_lines().get_size(), Count(1));
}

PERIMORTEM_UNIT_TEST(TtxDocumentation, late_bound) {
  Static::Vector<View::Bytes, 2> lines = {{
    "Initial"_view,
    "Second"_view,
  }};

  Ttx::Concept::Documentation documentation(lines);
  lines[0] = "Updated"_view;

  EXPECT_TEXT(documentation.line_at(0), "Updated"_view);
  EXPECT_TEXT(documentation.line_at(1), "Second"_view);
}
