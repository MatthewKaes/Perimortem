// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/system/file.hpp"
#include "perimortem/system/random.hpp"
#include "perimortem/utility/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;

using namespace Validation;

Test::Harness SystemFile = {.name = "System::File"};

PERIMORTEM_UNIT_TEST(SystemFile, empty_file) {
  auto start_requests = Allocator::Bibliotheca::check_out_requests();
  File file;

  EXPECT_EQ(file.get_size(), 0);
  EXPECT_TEXT(file.get_view(), ""_view);
  EXPECT_EQ(file.is_valid(), false);

  // Creating an empty file should perform zero allocations.
  EXPECT_EQ(Allocator::Bibliotheca::check_out_requests(), start_requests);
}

PERIMORTEM_UNIT_TEST(SystemFile, memory_file) {
  auto start_requests = Allocator::Bibliotheca::check_out_requests();
  constexpr Count file_size = 1 << 12;
  File file(file_size);

  Count non_zero_bytes = 0;
  EXPECT_EQ(file.get_size(), file_size);
  for (Count i = 0; i < file.get_size(); i++) {
    non_zero_bytes += file.get_view()[i] != 0;
  }
  EXPECT_EQ(non_zero_bytes, 0);
  EXPECT_EQ(file.is_valid(), true);

  // Creating an empty file should perform zero allocations.
  EXPECT_EQ(Allocator::Bibliotheca::check_out_requests(), start_requests + 1);
}

PERIMORTEM_UNIT_TEST(SystemFile, check_existence) {
  auto start_requests = Allocator::Bibliotheca::check_out_requests();

  EXPECT_EQ(File::exists("tests/perimortem/data/ttx/source.ttx"_view), true);
  EXPECT_EQ(File::exists("tests/perimortem/data/ttx/source2.ttx"_view), false);
  EXPECT_EQ(File::exists("perimortem/system/file.cpp"_view), true);

  // Directories don't count as files.
  EXPECT_EQ(File::exists("perimortem/"_view), false);
  EXPECT_EQ(File::exists("perimortem"_view), false);

  // Checking files should perform zero allocations.
  EXPECT_EQ(Allocator::Bibliotheca::check_out_requests(), start_requests);
}

PERIMORTEM_UNIT_TEST(SystemFile, file_contents) {
  File test_file = File::read("tests/perimortem/data/json/test.json"_view);
  auto json =
      "{\"object\":{\"a\":2,\"b\":\"3\"},\"array\":[1,2,true,\"test\"],"
      "\"member\":\"value\"}"_view;

  EXPECT_EQ(test_file.get_size(), 69);
  EXPECT_TEXT(test_file.get_view(), json);

  // File should round trip without issue.
  ASSERT(test_file.write(".bin/bin/tests/file_contents.json"_view));

  File temp_file = File::read(".bin/bin/tests/file_contents.json"_view);

  EXPECT_EQ(test_file.get_size(), 69);
  EXPECT_TEXT(test_file.get_view(), json);
}
