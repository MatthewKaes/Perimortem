// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/system/path.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Validation;

static Harness SystemPath = {
  .name = "System::Path"_view,
};

PERIMORTEM_UNIT_TEST(SystemPath, normalize) {
  Path path;

  ASSERT(path.set("unit\\./folder//file.ttx"_view));
  EXPECT_TEXT(path.get_view(), "unit/folder/file.ttx"_view);
  EXPECT_NOT(path.is_rooted());
}

PERIMORTEM_UNIT_TEST(SystemPath, relative) {
  Path path;

  ASSERT(path.set_relative("unit/source/main.ttx"_view, "../shared/a.ttx"_view));
  EXPECT_TEXT(path.get_view(), "unit/shared/a.ttx"_view);
}

PERIMORTEM_UNIT_TEST(SystemPath, rooted) {
  Path path;

  ASSERT(path.set("/usr/local/bin"_view));
  EXPECT_TEXT(path.get_view(), "/usr/local/bin"_view);
  EXPECT(path.is_rooted());
}

PERIMORTEM_UNIT_TEST(SystemPath, escape) {
  Path path;

  EXPECT_NOT(path.set("../file.ttx"_view));
  EXPECT_NOT(path.set_relative("unit/main.ttx"_view, "../../file.ttx"_view));
  EXPECT(path.is_empty());
}

PERIMORTEM_UNIT_TEST(SystemPath, root_parent) {
  Path path;

  ASSERT(path.set("/usr/.."_view));
  EXPECT_TEXT(path.get_view(), "/"_view);
  EXPECT(path.is_rooted());
}

PERIMORTEM_UNIT_TEST(SystemPath, self_assign) {
  Path path("unit\\source/./main.ttx"_view);

  ASSERT(path.set(path.get_view()));
  EXPECT_TEXT(path.get_view(), "unit/source/main.ttx"_view);
}
