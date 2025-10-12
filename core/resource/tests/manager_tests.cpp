// Perimortem Engine
// Copyright © Matt Kaes

#include <gtest/gtest.h>

#include "resource/manager.hpp"

using namespace Perimortem::Resource;
using namespace Perimortem::Storage;

extern auto read_all_bytes(const std::filesystem::path &p) -> Bytes;
extern auto compare_buffers(const ByteView &result, const ByteView &target)
    -> void;

struct ManagerTests : public ::testing::Test {
protected:
  virtual void SetUp() {
    test_root = ".bin/resource/tests/";
    test_root /=
        ::testing::UnitTest::GetInstance()->current_test_info()->name();

    source_root = "core/resource/tests/";
    source_root /=
        ::testing::UnitTest::GetInstance()->current_test_info()->name();

    std::filesystem::remove_all(test_root);
    std::filesystem::create_directories(test_root);

    for (int i = 0; i < Path::sector_count; i++) {
      disk_path[i] = test_root / Path::logical_disks[i];
      disk_path[i].replace_extension(virutal_disk_extension);
    }
  }

  virtual void TearDown() {}

  const std::filesystem::path &get_disk(Path::Sector sector) {
    return disk_path[(int)sector];
  }

  std::filesystem::path test_root;
  std::filesystem::path source_root;

  std::filesystem::path disk_path[Path::sector_count];
};

TEST_F(ManagerTests, single_resource) {
  Manager manager(test_root);

  std::string data = "Hello Filesystem!";
  Bytes raw_data = {data.begin(), data.end()};
  manager.create(Path("[res]://test/file.txt"), {data.begin(), data.end()});

  manager.flush_changes();

  // Check for any allocated disks
  ASSERT_TRUE(std::filesystem::exists(get_disk(Path::Sector::Resource)));
  // Empty disks are optional...
  ASSERT_FALSE(std::filesystem::exists(get_disk(Path::Sector::User)));
  ASSERT_FALSE(std::filesystem::exists(get_disk(Path::Sector::Scripts)));

  auto res = VirtualDiskReader::mount_disk(get_disk(Path::Sector::Resource));

  ASSERT_TRUE(res);
  ASSERT_EQ(res->get_files().size(), 1);

  ASSERT_EQ(res->get_files()[0].path, "test/file.txt");
  compare_buffers(res->get_files()[0].data, raw_data);
}

TEST_F(ManagerTests, user_resource) {
  Manager manager(test_root);

  std::string data = "Hello Filesystem!";
  Bytes raw_data = {data.begin(), data.end()};

  // Default to user when not specified.
  manager.create(Path("test/file_generic"), {data.begin(), data.end()});

  // User can also be explictly indexed.
  manager.create(Path("[usr]://test/file_user"), {data.begin(), data.end()});

  manager.flush_changes();

  // Check for any allocated disks
  ASSERT_TRUE(std::filesystem::exists(get_disk(Path::Sector::User)));
  // Empty disks are optional...
  ASSERT_FALSE(std::filesystem::exists(get_disk(Path::Sector::Resource)));
  ASSERT_FALSE(std::filesystem::exists(get_disk(Path::Sector::Scripts)));

  auto usr = VirtualDiskReader::mount_disk(get_disk(Path::Sector::User));

  ASSERT_TRUE(usr);
  ASSERT_EQ(usr->get_files().size(), 2);

  ASSERT_EQ(usr->get_files()[0].path, "test/file_user");
  ASSERT_EQ(usr->get_files()[1].path, "test/file_generic");

  compare_buffers(usr->get_files()[0].data, raw_data);
  compare_buffers(usr->get_files()[1].data, raw_data);
}

TEST_F(ManagerTests, recreate_resource) {
  Manager manager(test_root);

  std::string data = "Test Bytes";
  Bytes raw_data = {data.begin(), data.end()};
  auto path = Path("test/folder/example.txt");

  // Default to user when not specified.
  auto resource = manager.create(path, {data.begin(), data.end()});
  ASSERT_NE(resource, nullptr);

  // User can also be explictly indexed.
  resource = manager.create(path, {});
  ASSERT_EQ(resource, nullptr);

  // Data should still be valid.
  resource = manager.lookup(path);
  ASSERT_NE(resource, nullptr);
  EXPECT_TRUE(resource->in_memory());
  compare_buffers(resource->read_content(), raw_data);
}

TEST_F(ManagerTests, import_file) {
  Manager manager(test_root);
  std::filesystem::path svg_path = ".data/icon.svg";
  auto path = Path("[res]://game/images/icon.svg");

  manager.import(path, svg_path);

  // Resources should be loaded on import.
  auto resource = manager.lookup(path);
  ASSERT_NE(resource, nullptr);
  EXPECT_TRUE(resource->in_memory());

  auto data = read_all_bytes(svg_path);
  compare_buffers(resource->read_content(), data);

  // Files are imported as stand alone.
  manager.flush_changes();

  // Check for any allocated disks
  ASSERT_TRUE(std::filesystem::exists(get_disk(Path::Sector::Resource)));
  // Empty disks are optional...
  ASSERT_FALSE(std::filesystem::exists(get_disk(Path::Sector::User)));
  ASSERT_FALSE(std::filesystem::exists(get_disk(Path::Sector::Scripts)));

  auto res = VirtualDiskReader::mount_disk(get_disk(Path::Sector::Resource));

  ASSERT_TRUE(res);
  ASSERT_EQ(res->get_files().size(), 1);

  ASSERT_EQ(res->get_files()[0].path, path.get_origin());
  ASSERT_EQ(res->get_files()[0].data.size(), 0); // File should be on disk.
}

TEST_F(ManagerTests, load_from_file) {
  Manager manager("core/resource/tests/simple_project");
  std::filesystem::path svg_path = ".data/icon.svg";
  auto path = Path("[res]://game/images/icon.svg");

  // Resources should be loaded on import.
  auto resource = manager.lookup(path);
  ASSERT_NE(resource, nullptr);
  EXPECT_TRUE(resource->in_memory());

  auto data = read_all_bytes(svg_path);
  compare_buffers(resource->read_content(), data);
}