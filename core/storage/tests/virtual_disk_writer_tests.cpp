// Perimortem Engine
// Copyright © Matt Kaes

#include <gtest/gtest.h>

#include "storage/virtual_disk_writer.hpp"

using namespace Perimortem::Storage;

static std::string script;
static std::string script_path;
static ByteView script_view;
static std::string txt;
static std::string txt_path;
static ByteView txt_view;
static Bytes logo_svg;
static std::string logo_svg_path;
static ByteView logo_svg_view;

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
    test_disk_path = "core/storage/tests/disks/";
    test_disk_path /=
        ::testing::UnitTest::GetInstance()->current_test_info()->name();
    test_disk_path.replace_extension(virutal_disk_extension);
  }

  std::filesystem::path path;
  std::filesystem::path disk_path;
  std::filesystem::path test_disk_path;
};

TEST_F(DiskWriterTests, empty_disk) {
  VirtualDiskWriter writer;
  writer.create(DiskType::Standard);

  ASSERT_TRUE(writer.write_disk(path));
  ASSERT_TRUE(std::filesystem::exists(disk_path));
  compare_disks(disk_path, test_disk_path);
}

TEST_F(DiskWriterTests, simple_disk) {
  VirtualDiskWriter writer;
  writer.create(DiskType::Standard);

  std::string script = "# Test Script Data";
  writer.write_resource("test/test_script.ttx", (Byte *)script.data(),
                        script.size(), StorageOptions::Virtualized);

  auto logo_svg = read_all_bytes(".data/icon.svg");
  writer.write_resource(".data/icon.svg", logo_svg.data(), logo_svg.size(),
                        StorageOptions::Virtualized);

  ASSERT_TRUE(writer.write_disk(path));
  ASSERT_TRUE(std::filesystem::exists(disk_path));
  compare_disks(disk_path, test_disk_path);
}

TEST_F(DiskWriterTests, partial_disk) {
  VirtualDiskWriter writer;
  writer.create(DiskType::Standard);

  std::string script = "# Test Script Data";
  writer.write_resource("test/test_script.ttx", (Byte *)script.data(),
                        script.size(), StorageOptions::Virtualized);

  // Don't virtualize the icon.svg but keep it registered in the disk as an
  // external resoruce.
  auto logo_svg = read_all_bytes(".data/icon.svg");
  writer.write_resource(".data/icon.svg", logo_svg.data(), logo_svg.size(),
                        StorageOptions::None);

  ASSERT_TRUE(writer.write_disk(path));
  ASSERT_TRUE(std::filesystem::exists(disk_path));
  compare_disks(disk_path, test_disk_path);
}

TEST_F(DiskWriterTests, partial_preload_disk) {
  VirtualDiskWriter writer;
  writer.create(DiskType::Standard);

  std::string script = "# Test Script Data";
  writer.write_resource("test/test_script.ttx", (Byte *)script.data(),
                        script.size(), StorageOptions::Preload);

  // Don't virtualize the icon.svg but keep it registered in the disk as an
  // external resoruce.
  auto logo_svg = read_all_bytes(".data/icon.svg");
  writer.write_resource(".data/icon.svg", logo_svg.data(), logo_svg.size(),
                        StorageOptions::Preload);

  ASSERT_TRUE(writer.write_disk(path));
  ASSERT_TRUE(std::filesystem::exists(disk_path));
  compare_disks(disk_path, test_disk_path);
}

TEST_F(DiskWriterTests, simple_disk_compressed) {
  VirtualDiskWriter writer;
  writer.create(DiskType::Compressed);

  std::string script = "# Test Script Data";
  writer.write_resource("test/test_script.ttx", (Byte *)script.data(),
                        script.size(), StorageOptions::Virtualized);

  auto logo_svg = read_all_bytes(".data/icon.svg");
  writer.write_resource(".data/icon.svg", logo_svg.data(), logo_svg.size(),
                        StorageOptions::Virtualized);

  ASSERT_TRUE(writer.write_disk(path));
  ASSERT_TRUE(std::filesystem::exists(disk_path));
  compare_disks(disk_path, test_disk_path);
}

TEST_F(DiskWriterTests, simple_disk_max_compressed) {
  VirtualDiskWriter writer;
  writer.create(DiskType::Compressed, CompressionLevels::Smallest);

  std::string script = "# Test Script Data";
  writer.write_resource("test/test_script.ttx", (Byte *)script.data(),
                        script.size(), StorageOptions::Virtualized);

  auto logo_svg = read_all_bytes(".data/icon.svg");
  writer.write_resource(".data/icon.svg", logo_svg.data(), logo_svg.size(),
                        StorageOptions::Virtualized);

  ASSERT_TRUE(writer.write_disk(path));
  ASSERT_TRUE(std::filesystem::exists(disk_path));
  compare_disks(disk_path, test_disk_path);
}

TEST_F(DiskWriterTests, memory_disk) {
  VirtualDiskWriter writer;
  writer.create(DiskType::Memory, CompressionLevels::Smallest);

  std::string script = "# Test Script Data";
  writer.write_resource("test/test_script.ttx", (Byte *)script.data(),
                        script.size(), StorageOptions::Virtualized);

  script = "A bunch of test information which you might want to have streamed!";
  writer.write_resource("test_data/nested/example.txt", (Byte *)script.data(),
                        script.size(), StorageOptions::Virtualized);

  auto logo_svg = read_all_bytes(".data/icon.svg");
  writer.write_resource(".data/icon.svg", logo_svg.data(), logo_svg.size(),
                        StorageOptions::Virtualized);

  ASSERT_TRUE(writer.write_disk(path));
  ASSERT_TRUE(std::filesystem::exists(disk_path));
  compare_disks(disk_path, test_disk_path);
}

TEST_F(DiskWriterTests, stream_disk) {
  VirtualDiskWriter writer;
  writer.create(DiskType::Streamed, CompressionLevels::Smallest);

  std::string script = "# Test Script Data";
  writer.write_resource("test/test_script.ttx", (Byte *)script.data(),
                        script.size(), StorageOptions::Virtualized);

  script = "A bunch of test information which you might want to have streamed!";
  writer.write_resource("test_data/nested/example.txt", (Byte *)script.data(),
                        script.size(), StorageOptions::Virtualized);

  auto logo_svg = read_all_bytes(".data/icon.svg");
  writer.write_resource(".data/icon.svg", logo_svg.data(), logo_svg.size(),
                        StorageOptions::Virtualized);

  ASSERT_TRUE(writer.write_disk(path));
  ASSERT_TRUE(std::filesystem::exists(disk_path));
  compare_disks(disk_path, test_disk_path);
}

TEST_F(DiskWriterTests, stream_compressed_disk) {
  VirtualDiskWriter writer;
  writer.create(DiskType::StreamedCompressed, CompressionLevels::Smallest);

  std::string script = "# Test Script Data";
  writer.write_resource("test/test_script.ttx", (Byte *)script.data(),
                        script.size(), StorageOptions::Virtualized);

  script = "A bunch of test information which you might want to have streamed!";
  writer.write_resource("test_data/nested/example.txt", (Byte *)script.data(),
                        script.size(), StorageOptions::Virtualized);

  auto logo_svg = read_all_bytes(".data/icon.svg");
  writer.write_resource(".data/icon.svg", logo_svg.data(), logo_svg.size(),
                        StorageOptions::Virtualized);

  ASSERT_TRUE(writer.write_disk(path));
  ASSERT_TRUE(std::filesystem::exists(disk_path));
  compare_disks(disk_path, test_disk_path);
}