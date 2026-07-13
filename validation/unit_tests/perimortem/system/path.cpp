// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/path.hpp"

#include "validation/unit_test.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Validation;

static Harness SystemPath = {
  .name = "System::Path"_view,
};

PERIMORTEM_UNIT_TEST(SystemPath, normalize) {
  Path path("unit\\./folder//file.ttx"_view);

  EXPECT_NOT(path.get_view().is_empty());
  EXPECT_TEXT(path.get_view(), "unit/folder/file.ttx"_view);
  EXPECT_NOT(path.is_rooted());
}

PERIMORTEM_UNIT_TEST(SystemPath, relative) {
  Path path("unit/source/main.ttx"_view, "../shared/a.ttx"_view);

  EXPECT_NOT(path.get_view().is_empty());
  EXPECT_TEXT(path.get_view(), "unit/shared/a.ttx"_view);
}

PERIMORTEM_UNIT_TEST(SystemPath, rooted) {
  Path path("/usr/local/bin"_view);

  EXPECT_NOT(path.get_view().is_empty());
  EXPECT_TEXT(path.get_view(), "/usr/local/bin"_view);
  EXPECT(path.is_rooted());
}

PERIMORTEM_UNIT_TEST(SystemPath, escape) {
  Path path("../file.ttx"_view);
  Path relative_path("unit/main.ttx"_view, "../../file.ttx"_view);

  EXPECT(path.get_view().is_empty());
  EXPECT(relative_path.get_view().is_empty());
}

PERIMORTEM_UNIT_TEST(SystemPath, root_parent) {
  Path path("/usr/.."_view);

  EXPECT_NOT(path.get_view().is_empty());
  EXPECT_TEXT(path.get_view(), "/"_view);
  EXPECT(path.is_rooted());
}

PERIMORTEM_UNIT_TEST(SystemPath, construct_view) {
  Path path("unit\\source/./main.ttx"_view);
  Path copy(path);

  EXPECT_NOT(copy.get_view().is_empty());
  EXPECT_TEXT(copy.get_view(), "unit/source/main.ttx"_view);
}

PERIMORTEM_UNIT_TEST(SystemPath, file) {
  Path path("unit/source/main.ttx"_view);

  EXPECT_TEXT(path.get_file(), "main.ttx"_view);
}

PERIMORTEM_UNIT_TEST(SystemPath, directory) {
  Path path("unit/source/main.ttx"_view);
  Path root_child("/main.ttx"_view);

  EXPECT_TEXT(path.get_directory(), "unit/source"_view);
  EXPECT_TEXT(root_child.get_directory(), "/"_view);
}

PERIMORTEM_UNIT_TEST(SystemPath, extension) {
  Path path("unit/source/main.ttx"_view);
  Path no_extension("unit/source/main"_view);
  Path hidden("unit/source/.main"_view);

  EXPECT_TEXT(path.get_extension(), ".ttx"_view);
  EXPECT(no_extension.get_extension().is_empty());
  EXPECT(hidden.get_extension().is_empty());
}
