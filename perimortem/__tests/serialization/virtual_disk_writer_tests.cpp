// Perimortem Engine
// Copyright © Matt Kaes

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcharacter-conversion"
#include <gtest/gtest.h>
#pragma clang diagnostic pop

#include "perimortem/serialization/archive/writer.hpp"

using namespace Perimortem::Storage;
using namespace Perimortem::Memory;

extern auto read_all_bytes(const std::filesystem::path &p) -> Bytes;

void compare_disks(const std::filesystem::path &source_path,
                   const std::filesystem::path &target_path) {
  ASSERT_TRUE(std::filesystem::exists(target_path));

  auto result = read_all_bytes(source_path);
  auto target = read_all_bytes(target_path);
  ASSERT_EQ(result.size(), target.size()) << "Disks are of unequal length";

  for (int i = 0; i < target.size(); ++i) {
    EXPECT_EQ(result[i], target[i]) << "Disks differ at byte " << std::hex << i;
  }
}

struct DiskWriterTests : public ::testing::Test {
protected:
  static void SetUpTestSuite() {
    script = "# Test Script Data";
    script_path = "test/test_script.ttx";
    script_view = {(Byte *)script.data(), script.size()};

    txt = "A bunch of test information which you might want to have streamed!";
    txt_path = "test_data/nested/example.txt";
    txt_view = {(Byte *)txt.data(), txt.size()};

    logo_svg = read_all_bytes(".data/icon.svg");
    logo_svg_path = ".data/icon.svg";
    logo_svg_view = {logo_svg.data(), logo_svg.size()};
  }

  virtual void SetUp() {
    path = ".bin/";
    path /= ::testing::UnitTest::GetInstance()->current_test_info()->name();
    disk_path = path;
    disk_path.replace_extension(virutal_disk_extension);
    test_disk_path = "perimortem/tests/disks/";
    test_disk_path /=
        ::testing::UnitTest::GetInstance()->current_test_info()->name();
    test_disk_path.replace_extension(virutal_disk_extension);
  }

  std::filesystem::path path;
  std::filesystem::path disk_path;
  std::filesystem::path test_disk_path;
};

TEST_F(DiskWriterTests, empty_disk) {
  Writer writer;
  writer.create(Type::Standard);

  ASSERT_TRUE(writer.write_disk(path));
  ASSERT_TRUE(std::filesystem::exists(disk_path));
  compare_disks(disk_path, test_disk_path);
}

TEST_F(DiskWriterTests, simple_disk) {
  Writer writer;
  writer.create(Type::Standard);

  std::string script = "# Test Script Data";
  writer.stage_resource("test/test_script.ttx", (Byte *)script.data(),
                        script.size(), FileStorage::Virtualized);

  auto logo_svg = read_all_bytes(".data/icon.svg");
  writer.stage_resource(".data/icon.svg", logo_svg.data(), logo_svg.size(),
                        FileStorage::Virtualized);

  ASSERT_TRUE(writer.write_disk(path));
  ASSERT_TRUE(std::filesystem::exists(disk_path));
  compare_disks(disk_path, test_disk_path);
}

TEST_F(DiskWriterTests, partial_disk) {
  Writer writer;
  writer.create(Type::Standard);

  std::string script = "# Test Script Data";
  writer.stage_resource("test/test_script.ttx", (Byte *)script.data(),
                        script.size(), FileStorage::Virtualized);

  // Don't virtualize the icon.svg but keep it registered in the disk as an
  // external resoruce.
  auto logo_svg = read_all_bytes(".data/icon.svg");
  writer.stage_resource(".data/icon.svg", logo_svg.data(), logo_svg.size(),
                        FileStorage::None);

  ASSERT_TRUE(writer.write_disk(path));
  ASSERT_TRUE(std::filesystem::exists(disk_path));
  compare_disks(disk_path, test_disk_path);
}

TEST_F(DiskWriterTests, partial_preload_disk) {
  Writer writer;
  writer.create(Type::Standard);

  std::string script = "# Test Script Data";
  writer.stage_resource("test/test_script.ttx", (Byte *)script.data(),
                        script.size(), FileStorage::Preload);

  // Don't virtualize the icon.svg but keep it registered in the disk as an
  // external resoruce.
  auto logo_svg = read_all_bytes(".data/icon.svg");
  writer.stage_resource(".data/icon.svg", logo_svg.data(), logo_svg.size(),
                        FileStorage::Preload);

  ASSERT_TRUE(writer.write_disk(path));
  ASSERT_TRUE(std::filesystem::exists(disk_path));
  compare_disks(disk_path, test_disk_path);
}

TEST_F(DiskWriterTests, simple_disk_compressed) {
  Writer writer;
  writer.create(Type::Compressed);

  std::string script = "# Test Script Data";
  writer.stage_resource("test/test_script.ttx", (Byte *)script.data(),
                        script.size(), FileStorage::Virtualized);

  auto logo_svg = read_all_bytes(".data/icon.svg");
  writer.stage_resource(".data/icon.svg", logo_svg.data(), logo_svg.size(),
                        FileStorage::Virtualized);

  ASSERT_TRUE(writer.write_disk(path));
  ASSERT_TRUE(std::filesystem::exists(disk_path));
  compare_disks(disk_path, test_disk_path);
}

TEST_F(DiskWriterTests, simple_disk_max_compressed) {
  Writer writer;
  writer.create(Type::Compressed, CompressionLevel::Smallest);

  std::string script = "# Test Script Data";
  writer.stage_resource("test/test_script.ttx", (Byte *)script.data(),
                        script.size(), FileStorage::Virtualized);

  auto logo_svg = read_all_bytes(".data/icon.svg");
  writer.stage_resource(".data/icon.svg", logo_svg.data(), logo_svg.size(),
                        FileStorage::Virtualized);

  ASSERT_TRUE(writer.write_disk(path));
  ASSERT_TRUE(std::filesystem::exists(disk_path));
  compare_disks(disk_path, test_disk_path);
}

TEST_F(DiskWriterTests, memory_disk) {
  Writer writer;
  writer.create(Type::Memory, CompressionLevel::Smallest);

  std::string script = "# Test Script Data";
  writer.stage_resource("test/test_script.ttx", (Byte *)script.data(),
                        script.size(), FileStorage::Virtualized);

  script = "A bunch of test information which you might want to have streamed!";
  writer.stage_resource("test_data/nested/example.txt", (Byte *)script.data(),
                        script.size(), FileStorage::Virtualized);

  auto logo_svg = read_all_bytes(".data/icon.svg");
  writer.stage_resource(".data/icon.svg", logo_svg.data(), logo_svg.size(),
                        FileStorage::Virtualized);

  ASSERT_TRUE(writer.write_disk(path));
  ASSERT_TRUE(std::filesystem::exists(disk_path));
  compare_disks(disk_path, test_disk_path);
}

TEST_F(DiskWriterTests, stream_disk) {
  Writer writer;
  writer.create(Type::Streamed, CompressionLevel::Smallest);

  std::string script = "# Test Script Data";
  writer.stage_resource("test/test_script.ttx", (Byte *)script.data(),
                        script.size(), FileStorage::Virtualized);

  script = "A bunch of test information which you might want to have streamed!";
  writer.stage_resource("test_data/nested/example.txt", (Byte *)script.data(),
                        script.size(), FileStorage::Virtualized);

  auto logo_svg = read_all_bytes(".data/icon.svg");
  writer.stage_resource(".data/icon.svg", logo_svg.data(), logo_svg.size(),
                        FileStorage::Virtualized);

  ASSERT_TRUE(writer.write_disk(path));
  ASSERT_TRUE(std::filesystem::exists(disk_path));
  compare_disks(disk_path, test_disk_path);
}

TEST_F(DiskWriterTests, stream_compressed_disk) {
  Writer writer;
  writer.create(Type::StreamedCompressed, CompressionLevel::Smallest);

  std::string script = "# Test Script Data";
  writer.stage_resource("test/test_script.ttx", (Byte *)script.data(),
                        script.size(), FileStorage::Virtualized);

  script = "A bunch of test information which you might want to have streamed!";
  writer.stage_resource("test_data/nested/example.txt", (Byte *)script.data(),
                        script.size(), FileStorage::Virtualized);

  auto logo_svg = read_all_bytes(".data/icon.svg");
  writer.stage_resource(".data/icon.svg", logo_svg.data(), logo_svg.size(),
                        FileStorage::Virtualized);

  ASSERT_TRUE(writer.write_disk(path));
  ASSERT_TRUE(std::filesystem::exists(disk_path));
  compare_disks(disk_path, test_disk_path);
}