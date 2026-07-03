// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/file.hpp"

#include "validation/unit_test.hpp"

#include <stdio.h>

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/random.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;

using namespace Validation;

constexpr auto test_directory = "validation/data"_view;
constexpr auto test_file = ".bin/bin/validation/system_file_test.json"_view;
constexpr auto test_output =
    ".bin/bin/validation/system_file_test_out.json"_view;
constexpr auto test_file_c_string = ".bin/bin/validation/system_file_test.json";
constexpr auto test_output_c_string =
    ".bin/bin/validation/system_file_test_out.json";
constexpr auto json_contents =
    "{\"object\":{\"a\":2,\"b\":\"3\"},\"array\":[1,2,true,\"test\"],"
    "\"member\":\"value\"}"_view;

static Harness SystemFile = {
  // Harness to try and make file management at least some what "hermetic"
  .name = "System::File"_view,
  .setup =
      []() {
        auto file_source = fopen(test_file_c_string, "wb");
        if (!file_source) {
          return;
        }

        fwrite(json_contents.get_data(), json_contents.get_size(), 1, file_source);
        fclose(file_source);
      },
  .teardown =
      []() {
        remove(test_file_c_string);
        remove(test_output_c_string);
      },
};

PERIMORTEM_UNIT_TEST(SystemFile, empty_file) {
  auto start_requests = Bibliotheca::check_out_requests();
  File file;

  EXPECT_EQ(file.get_size(), 0);
  EXPECT_TEXT(file.get_view(), ""_view);
  EXPECT_NOT(file.is_valid());

  // Creating an empty file should perform zero allocations.
  EXPECT_EQ(Bibliotheca::check_out_requests(), start_requests);
}

PERIMORTEM_UNIT_TEST(SystemFile, memory_file) {
  auto start_requests = Bibliotheca::check_out_requests();
  File file;
  EXPECT_EQ(file.get_size(), 0);

  constexpr Count buffer_size = 1 << 12;
  constexpr Bits_8 buffer_value = 4;
  Dynamic::Bytes buffer;
  buffer.resize(buffer_size);
  buffer.set(buffer_value);

  // Move the contents into the file.
  file.update_contents(Data::take(buffer));

  Count incorrect_bytes = 0;
  EXPECT_EQ(file.get_size(), buffer_size);
  for (Count i = 0; i < file.get_size(); i++) {
    incorrect_bytes += file.get_view()[i] != buffer_value;
  }
  EXPECT_EQ(incorrect_bytes, 0);
  EXPECT(file.is_valid());

  // Only creating the Dynamic::Bytes should allocate memory.
  EXPECT_EQ(Bibliotheca::check_out_requests(), start_requests + 1);
}

PERIMORTEM_UNIT_TEST(SystemFile, check_existence) {
  auto start_requests = Bibliotheca::check_out_requests();

  EXPECT(File::exists("validation/data/ttx/png.ttx"_view));
  EXPECT_NOT(File::exists("validation/data/ttx/png2.ttx"_view));
  EXPECT(File::exists("perimortem/system/file.cpp"_view));

  // Directories don't count as files.
  EXPECT_NOT(File::exists("perimortem/"_view));
  EXPECT_NOT(File::exists("perimortem"_view));

  // Checking files should perform zero allocations.
  EXPECT_EQ(Bibliotheca::check_out_requests(), start_requests);
}

PERIMORTEM_UNIT_TEST(SystemFile, file_contents) {
  auto start_requests = Bibliotheca::check_out_requests();
  File file;

  ASSERT(file.read(test_file));

  // Reading the file should require an allocation.
  EXPECT_EQ(Bibliotheca::check_out_requests(), start_requests + 1);

  EXPECT_EQ(file.get_size(), json_contents.get_size());
  EXPECT_TEXT(file.get_view(), json_contents);

  // File should round trip without issue.
  auto test_output_location =
      ".bin/bin/validation/file_contents_temp.json"_view;
  ASSERT(file.write(test_output_location));

  ASSERT(file.read(test_output_location));

  EXPECT_TEXT(file.get_view(), json_contents);

  EXPECT(File::remove(test_output_location));
}

PERIMORTEM_UNIT_TEST(SystemFile, sync_empty_file) {
  File empty_file;
  File file;

  // Syncing an empty file should start as stale and move to original
  ASSERT_EQ((Bits_8)file.sync_status(test_file), (Bits_8)File::State::Stale);
  ASSERT_EQ((Bits_8)file.sync(test_file), (Bits_8)File::State::Original);
  ASSERT_EQ((Bits_8)file.sync_status(test_file), (Bits_8)File::State::Original);

  // Data should be valid after
  EXPECT_TEXT(file.get_view(), json_contents);
}

PERIMORTEM_UNIT_TEST(SystemFile, sync_empty_empty) {
  File empty_file;
  File file;

  // Syncing invalid files to an empty location is a no-op.
  ASSERT_EQ(
      (Bits_8)file.sync_status(test_output), (Bits_8)File::State::Original);
  EXPECT_EQ((Bits_8)file.sync(test_output), (Bits_8)File::State::Original);
  ASSERT_EQ(
      (Bits_8)file.sync_status(test_output), (Bits_8)File::State::Original);

  // Shouldn't be a file to remove
  EXPECT_NOT(File::remove(test_output));

  // Data should be invalid after.
  EXPECT_NOT(file.is_valid());
  EXPECT_TEXT(file.get_view(), ""_view);
}

PERIMORTEM_UNIT_TEST(SystemFile, sync_memory_empty) {
  File empty_file;
  File file;
  constexpr auto test_content = "test"_view;
  file.update_contents(test_content);

  // Syncing new content to an empty location is always a fresh write.
  ASSERT_EQ((Bits_8)file.sync_status(test_output), (Bits_8)File::State::Create);
  EXPECT_EQ((Bits_8)file.sync(test_output), (Bits_8)File::State::Original);
  ASSERT_EQ(
      (Bits_8)file.sync_status(test_output), (Bits_8)File::State::Original);

  // Should create a file which can be removed.
  EXPECT(File::remove(test_file));
  ASSERT_EQ((Bits_8)file.sync_status(test_file), (Bits_8)File::State::Create);

  // Data should be valid after
  EXPECT_TEXT(file.get_view(), test_content);
}

PERIMORTEM_UNIT_TEST(SystemFile, sync_directory) {
  File empty_file;
  File file;

  // Syncing to a non file is always invalid.
  ASSERT_EQ(
      (Bits_8)file.sync_status(test_directory), (Bits_8)File::State::Invalid);
  EXPECT_EQ((Bits_8)file.sync(test_directory), (Bits_8)File::State::Invalid);
  ASSERT_EQ(
      (Bits_8)file.sync_status(test_directory), (Bits_8)File::State::Invalid);

  // Data should be invalid after.
  EXPECT_NOT(file.is_valid());
  EXPECT_TEXT(file.get_view(), ""_view);
}

PERIMORTEM_UNIT_TEST(SystemFile, sync_memory_existing) {
  File empty_file;
  File file;
  auto test_content = "test"_view;

  // Syncing a file from memory with a file on disk should force a conflict.
  ASSERT_EQ((Bits_8)file.sync_status(test_file), (Bits_8)File::State::Stale);
  file.update_contents(test_content);
  ASSERT_EQ((Bits_8)file.sync_status(test_file), (Bits_8)File::State::Conflict);
  ASSERT_EQ((Bits_8)file.sync(test_file), (Bits_8)File::State::Conflict);

  // Data should be original content
  EXPECT(file.is_valid());
  EXPECT_TEXT(file.get_view(), test_content);
}

PERIMORTEM_UNIT_TEST(SystemFile, sync_read_and_write) {
  File empty_file;
  File file;

  // Sync to current.
  ASSERT_EQ((Bits_8)file.sync_status(test_file), (Bits_8)File::State::Stale);
  ASSERT_EQ((Bits_8)file.sync(test_file), (Bits_8)File::State::Original);
  ASSERT_EQ((Bits_8)file.sync_status(test_file), (Bits_8)File::State::Original);

  // Now have data to sync to new location
  ASSERT_EQ((Bits_8)file.sync_status(test_output), (Bits_8)File::State::Create);
  ASSERT_EQ((Bits_8)file.sync(test_output), (Bits_8)File::State::Original);
  ASSERT_EQ(
      (Bits_8)file.sync_status(test_output), (Bits_8)File::State::Original);

  // File points to a new location so it's in conflict compared to original
  // source, even though the contents are the same.
  ASSERT_EQ((Bits_8)file.sync_status(test_file), (Bits_8)File::State::Conflict);

  // Should create a file which can be removed.
  EXPECT(File::remove(test_file));
  ASSERT_EQ((Bits_8)file.sync_status(test_file), (Bits_8)File::State::Create);

  // Data should still be the contents of the original file.
  EXPECT_TEXT(file.get_view(), json_contents);
}

PERIMORTEM_UNIT_TEST(SystemFile, sync_update_write) {
  auto start_requests = Bibliotheca::check_out_requests();
  constexpr auto new_content = "empty"_view;
  File empty_file;
  File file;

  // Sync to current.
  ASSERT_EQ((Bits_8)file.sync_status(test_file), (Bits_8)File::State::Stale);
  ASSERT_EQ((Bits_8)file.sync(test_file), (Bits_8)File::State::Original);
  ASSERT_EQ((Bits_8)file.sync_status(test_file), (Bits_8)File::State::Original);

  // First file loaded.
  EXPECT_EQ(Bibliotheca::check_out_requests(), start_requests + 1);

  // Make sure we loaded the right content.
  EXPECT_TEXT(file.get_view(), json_contents);
  file.update_contents(new_content);
  // "empty" (5 bytes) is smaller than the JSON content buffer, so
  // forgetful_resize remits the old block and checks out a smaller one.
  EXPECT_EQ(Bibliotheca::check_out_requests(), start_requests + 2);

  // Flush the new data to disk.
  ASSERT_EQ((Bits_8)file.sync_status(test_file), (Bits_8)File::State::Fresh);
  ASSERT_EQ((Bits_8)file.sync(test_file), (Bits_8)File::State::Original);
  ASSERT_EQ((Bits_8)file.sync_status(test_file), (Bits_8)File::State::Original);

  // Sync data via another file.
  File file_copy;
  ASSERT_EQ((Bits_8)file_copy.sync(test_file), (Bits_8)File::State::Original);

  // Should create a file which can be removed.
  EXPECT(File::remove(test_file));
  ASSERT_EQ((Bits_8)file.sync_status(test_file), (Bits_8)File::State::Create);

  // The two files should have the same data.
  EXPECT_TEXT(file.get_view(), new_content);
  EXPECT_TEXT(file_copy.get_view(), new_content);

  // file_copy.sync allocates one block to read "empty" (5 bytes).
  EXPECT_EQ(Bibliotheca::check_out_requests(), start_requests + 3);
}
