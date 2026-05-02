// Perimortem Engine
// Copyright © Matt Kaes

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcharacter-conversion"
#include <gtest/gtest.h>
#pragma clang diagnostic pop

#include "perimortem/serialization/archive/reader.hpp"

#include <bitset>
#include <memory>

using namespace Perimortem::Memory;

static std::string script;
static std::string script_path;
static Core::View::Bytes script_view;
static std::string txt;
static std::string txt_path;
static Core::View::Bytes txt_view;
static Bytes logo_svg;
static std::string logo_svg_path;
static Core::View::Bytes logo_svg_view;

auto read_all_bytes(const std::filesystem::path &p) -> Bytes {
  std::ifstream ifs(p, std::ios::binary | std::ios::ate);
  std::ifstream::pos_type pos = ifs.tellg();

  if (pos == 0) {
    return Bytes();
  }

  Bytes data(pos);
  ifs.seekg(0, std::ios::beg);
  ifs.read((char *)data.data(), pos);
  return data;
}

void compare_buffers(const View::Byte &result, const View::Byte &target) {
  ASSERT_EQ(result.size(), target.size()) << "Buffers are of unequal length";

  for (int i = 0; i < target.size(); ++i) {
    EXPECT_EQ(result[i], target[i])
        << "Buffers differ at byte " << std::hex << i;
  }
}

struct DiskReaderTests : public ::testing::Test {
protected:
  static void SetUpTestSuite() {
    script = "# Test Script Data";
    script_path = "test/test_script.ttx";
    script_view = {(Byte*)script.data(), script.size()};

    txt = "A bunch of test information which you might want to have streamed!";
    txt_path = "test_data/nested/example.txt";
    txt_view = {(Byte*)txt.data(), txt.size()};

    logo_svg = read_all_bytes(".data/icon.svg");
    logo_svg_path = ".data/icon.svg";
    logo_svg_view = {logo_svg.data(), logo_svg.size()};
  }


  virtual void SetUp() {
    std::filesystem::path p = "perimortem/__tests/__data/disks/";
    p /= ::testing::UnitTest::GetInstance()->current_test_info()->name();
    p.replace_extension(Archive::virutal_extension);
    reader = Archive::Reader::mount_archive(p);
  }

  virtual void TearDown() {
    if (::testing::Test::HasFailure()) {
      std::cout << reader->dump_info();
    }
  }

  Archive::Reader reader;
};

TEST_F(DiskReaderTests, empty_disk) {
  EXPECT_EQ(reader->get_format(), Type::Standard);
  EXPECT_EQ(reader->get_blocks().size(), 2);
  EXPECT_EQ(reader->get_blocks()[0].location, 0x10);
  EXPECT_EQ(reader->get_blocks()[1].location, 0x18);

  ASSERT_EQ(reader->get_files().size(), 0);
}

TEST_F(DiskReaderTests, simple_disk) {
  EXPECT_EQ(reader->get_format(), Type::Standard);
  EXPECT_EQ(reader->get_blocks().size(), 2);
  EXPECT_EQ(reader->get_blocks()[0].location, 0x10);
  EXPECT_EQ(reader->get_blocks()[1].location, 0x44);

  ASSERT_EQ(reader->get_files().size(), 2);

  EXPECT_EQ(reader->get_files()[0].path, script_path);
  EXPECT_EQ(reader->get_files()[0].options, FileStorage::Virtualized);
  compare_buffers(reader->get_files()[0].data, script_view);

  EXPECT_EQ(reader->get_files()[1].path, logo_svg_path);
  EXPECT_EQ(reader->get_files()[1].options, FileStorage::Virtualized);
  compare_buffers(reader->get_files()[1].data, logo_svg_view);
}

TEST_F(DiskReaderTests, partial_disk) {
  EXPECT_EQ(reader->get_format(), Type::Standard);
  EXPECT_EQ(reader->get_blocks().size(), 2);
  EXPECT_EQ(reader->get_blocks()[0].location, 0x10);
  EXPECT_EQ(reader->get_blocks()[1].location, 0x42);

  ASSERT_EQ(reader->get_files().size(), 2);

  EXPECT_EQ(reader->get_files()[0].path, script_path);
  EXPECT_EQ(reader->get_files()[0].options, FileStorage::Virtualized);
  compare_buffers(reader->get_files()[0].data, script_view);

  EXPECT_EQ(reader->get_files()[1].path, logo_svg_path);
  EXPECT_EQ(reader->get_files()[1].options, FileStorage::None);
  EXPECT_EQ(reader->get_files()[1].data.size(), 0);
}

TEST_F(DiskReaderTests, partial_preload_disk) {
  EXPECT_EQ(reader->get_format(), Type::Standard);
  EXPECT_EQ(reader->get_blocks().size(), 2);
  EXPECT_EQ(reader->get_blocks()[0].location, 0x10);
  EXPECT_EQ(reader->get_blocks()[1].location, 0x40);

  ASSERT_EQ(reader->get_files().size(), 2);

  EXPECT_EQ(reader->get_files()[0].path, script_path);
  EXPECT_EQ(reader->get_files()[0].options, FileStorage::Preload);
  EXPECT_EQ(reader->get_files()[0].data.size(), 0);

  EXPECT_EQ(reader->get_files()[1].path, logo_svg_path);
  EXPECT_EQ(reader->get_files()[1].options, FileStorage::Preload);
  EXPECT_EQ(reader->get_files()[1].data.size(), 0);
}

TEST_F(DiskReaderTests, simple_disk_max_compressed) {
  EXPECT_EQ(reader->get_format(), Type::Compressed);
  EXPECT_EQ(reader->get_blocks().size(), 2);
  EXPECT_EQ(reader->get_blocks()[0].location, 0x10);
  EXPECT_EQ(reader->get_blocks()[1].location, 0x4D);

  ASSERT_EQ(reader->get_files().size(), 2);

  std::string script = "# Test Script Data";
  EXPECT_EQ(reader->get_files()[0].path, script_path);
  EXPECT_EQ(reader->get_files()[0].options, FileStorage::Virtualized);
  compare_buffers(reader->get_files()[0].data, script_view);

  EXPECT_EQ(reader->get_files()[1].path, logo_svg_path);
  EXPECT_EQ(reader->get_files()[1].options, FileStorage::Virtualized);
  compare_buffers(reader->get_files()[1].data, logo_svg_view);
}

TEST_F(DiskReaderTests, memory_disk) {
  EXPECT_EQ(reader->get_format(), Type::Memory);
  EXPECT_EQ(reader->get_blocks().size(), 1);
  EXPECT_EQ(reader->get_blocks()[0].location, 0x10);

  ASSERT_EQ(reader->get_files().size(), 3);

  std::string script = "# Test Script Data";
  EXPECT_EQ(reader->get_files()[0].path, script_path);
  EXPECT_EQ(reader->get_files()[0].options, FileStorage::Virtualized);
  compare_buffers(reader->get_files()[0].data, script_view);

  EXPECT_EQ(reader->get_files()[1].path, txt_path);
  EXPECT_EQ(reader->get_files()[1].options, FileStorage::Virtualized);
  compare_buffers(reader->get_files()[1].data, txt_view);

  EXPECT_EQ(reader->get_files()[2].path, logo_svg_path);
  EXPECT_EQ(reader->get_files()[2].options, FileStorage::Virtualized);
  compare_buffers(reader->get_files()[2].data, logo_svg_view);
}

TEST_F(DiskReaderTests, stream_disk) {
  EXPECT_EQ(reader->get_format(), Type::Streamed);
  EXPECT_EQ(reader->get_blocks().size(), 2);
  EXPECT_EQ(reader->get_blocks()[0].location, 0x10);
  EXPECT_EQ(reader->get_blocks()[1].location, 0x65);

  ASSERT_EQ(reader->get_files().size(), 3);

  EXPECT_EQ(reader->get_files()[0].path, script_path);
  EXPECT_EQ(reader->get_files()[0].data.size(), 0);
  EXPECT_EQ(reader->get_files()[0].options, FileStorage::Virtualized | FileStorage::Streamed);

  EXPECT_EQ(reader->get_files()[1].path, txt_path);
  EXPECT_EQ(reader->get_files()[1].data.size(), 0);
  EXPECT_EQ(reader->get_files()[1].options, FileStorage::Virtualized | FileStorage::Streamed);
  
  EXPECT_EQ(reader->get_files()[2].path, logo_svg_path);
  EXPECT_EQ(reader->get_files()[2].data.size(), 0);
  EXPECT_EQ(reader->get_files()[2].options, FileStorage::Virtualized | FileStorage::Streamed);

  Bytes data;
  ASSERT_TRUE(reader->stream_from_disk(script_path, data));
  compare_buffers(data, script_view);

  ASSERT_TRUE(reader->stream_from_disk(txt_path, data));
  compare_buffers(data, txt_view);

  ASSERT_TRUE(reader->stream_from_disk(logo_svg_path, data));
  compare_buffers(data, logo_svg_view);
}

TEST_F(DiskReaderTests, stream_compressed_disk) {
  EXPECT_EQ(reader->get_format(), Type::StreamedCompressed);
  EXPECT_EQ(reader->get_blocks().size(), 2);
  EXPECT_EQ(reader->get_blocks()[0].location, 0x10);
  EXPECT_EQ(reader->get_blocks()[1].location, 0x65);

  ASSERT_EQ(reader->get_files().size(), 3);

  EXPECT_EQ(reader->get_files()[0].path, script_path);
  EXPECT_EQ(reader->get_files()[0].data.size(), 0);
  EXPECT_EQ(reader->get_files()[0].options, FileStorage::Virtualized | FileStorage::Streamed);

  EXPECT_EQ(reader->get_files()[1].path, txt_path);
  EXPECT_EQ(reader->get_files()[1].data.size(), 0);
  EXPECT_EQ(reader->get_files()[1].options, FileStorage::Virtualized | FileStorage::Streamed);
  
  EXPECT_EQ(reader->get_files()[2].path, logo_svg_path);
  EXPECT_EQ(reader->get_files()[2].data.size(), 0);
  EXPECT_EQ(reader->get_files()[2].options, FileStorage::Virtualized | FileStorage::Streamed);

  Bytes data;
  ASSERT_TRUE(reader->stream_from_disk(script_path, data));
  compare_buffers(data, script_view);

  ASSERT_TRUE(reader->stream_from_disk(txt_path, data));
  compare_buffers(data, txt_view);

  ASSERT_TRUE(reader->stream_from_disk(logo_svg_path, data));
  compare_buffers(data, logo_svg_view);
}