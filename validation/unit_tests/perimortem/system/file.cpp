// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/file.hpp"

#include "validation/unit_test.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Validation;

constexpr auto test_file = ".bin/bin/validation/system_file_test.json"_view;
constexpr auto test_output =
    ".bin/bin/validation/system_file_test_out.json"_view;
constexpr auto test_contents = "{\"value\":42}"_view;

static Harness SystemFile = {
  .name = "System::File"_view,
  .setup = []() { File::write(test_contents, test_file); },
  .teardown =
      []() {
        File::remove(test_file);
        File::remove(test_output);
      },
};

PERIMORTEM_UNIT_TEST(SystemFile, read) {
  EXPECT_TEXT(File::read(test_file), test_contents);
}

PERIMORTEM_UNIT_TEST(SystemFile, write) {
  ASSERT(File::write(test_contents, test_output));
  EXPECT_TEXT(File::read(test_output), test_contents);
}

PERIMORTEM_UNIT_TEST(SystemFile, empty) {
  ASSERT(File::write(View::Bytes(), test_output));
  EXPECT(File::exists(test_output));
  EXPECT(File::read(test_output).is_empty());
}

PERIMORTEM_UNIT_TEST(SystemFile, missing) {
  File::remove(test_output);

  EXPECT_NOT(File::exists(test_output));
  EXPECT(File::read(test_output).is_empty());
  EXPECT_NOT(File::remove(test_output));
}

PERIMORTEM_UNIT_TEST(SystemFile, exists) {
  EXPECT(File::exists(test_file));
  EXPECT_NOT(File::exists("perimortem/"_view));
  EXPECT_NOT(File::exists("perimortem"_view));
}

PERIMORTEM_UNIT_TEST(SystemFile, remove) {
  ASSERT(File::write(test_contents, test_output));
  ASSERT(File::remove(test_output));
  EXPECT_NOT(File::exists(test_output));
}
