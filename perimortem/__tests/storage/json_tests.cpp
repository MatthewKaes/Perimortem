// Perimortem Engine
// Copyright © Matt Kaes

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcharacter-conversion"
#include <gtest/gtest.h>
#pragma clang diagnostic pop

#include "storage/formats/json.hpp"
#include "storage/formats/rpc_header.hpp"

#include <filesystem>
#include <fstream>

struct JsonTests : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {}

  virtual void SetUp() {}

  virtual void TearDown() {}
};

using namespace Perimortem::Memory;
using namespace Perimortem::Storage::Json;

auto load_text(const std::filesystem::path& p) -> std::string {
  std::ifstream ifs(p, std::ios::binary | std::ios::ate);
  std::ifstream::pos_type pos = ifs.tellg();

  if (pos == 0) {
    return "";
  }

  std::string data;
  data.resize(pos);
  ifs.seekg(0, std::ios::beg);
  ifs.read((char*)data.data(), pos);
  return data;
}

TEST_F(JsonTests, init_rpc) {
  Arena test;
  uint32_t pos = 0;
  std::string json = load_text("perimortem/tests/json/init_rpc.json");

  Node node;
  node.from_source(test, View::Byte(json), pos);

  EXPECT_TRUE(node.contains("method"));
  EXPECT_EQ(node.at("method").get_string().get_view(), "initialize");

  EXPECT_TRUE(node.contains("jsonrpc"));
  EXPECT_EQ(node.at("jsonrpc").get_string().get_view(), "2.0");

  EXPECT_TRUE(node.contains("id"));
  EXPECT_EQ(node.at("id").get_int(), 10);

  EXPECT_TRUE(node.contains("params"));
  EXPECT_TRUE(node.at("params").contains("rootPath"));
  auto path = node.at("params").at("rootPath").get_string();
  EXPECT_EQ(path.get_view(),
            "/home/test/Perimortem/tetrodotoxin/tests/scripts");
}

TEST_F(JsonTests, tokenize_rpc) {
  Arena test;
  uint32_t pos = 0;
  std::string json = load_text("perimortem/tests/json/tokenize_rpc.json");

  Node node;
  node.from_source(test, View::Byte(json), pos);

  EXPECT_TRUE(node.contains("method"));
  EXPECT_EQ(node.at("method").get_string().get_view(), "tokenize");

  EXPECT_TRUE(node.contains("jsonrpc"));
  EXPECT_EQ(node.at("jsonrpc").get_string().get_view(), "2.0");

  EXPECT_TRUE(node.contains("id"));
  EXPECT_EQ(node.at("id").get_int(), 1);

  ASSERT_TRUE(node.contains("params"));
  auto params = node.at("params");
  ASSERT_TRUE(params.contains("source"));
}

TEST_F(JsonTests, rpc_header) {
  std::string json = load_text("perimortem/tests/json/init_rpc.json");

  Arena arena;
  auto data = Perimortem::Storage::Json::JsonRpc(
      arena, View::Bytes(json.c_str(), json.size()));

  EXPECT_EQ(data.get_method(), "initialize"_bv);
  EXPECT_EQ(data.get_version(), "2.0"_bv);
  EXPECT_EQ(data.get_id(), "10"_bv);
  EXPECT_EQ(data.get_params_offset(), 56);
}

TEST_F(JsonTests, jsonrpc) {
  Arena test;
  uint32_t pos = 0;
  std::string json = load_text("perimortem/tests/json/initialized_rpc.json");

  Node node;
  node.from_source(test, View::Byte(json), pos);

  EXPECT_TRUE(node.contains("method"));
  EXPECT_EQ(node.at("method").get_string().get_view(), "initialized");

  EXPECT_TRUE(node.contains("jsonrpc"));
  EXPECT_EQ(node.at("jsonrpc").get_string().get_view(), "2.0");
  ASSERT_TRUE(node.contains("params"));
}

TEST_F(JsonTests, jsonrpc_from_header) {
  Arena test;
  std::string json = load_text("perimortem/tests/json/init_rpc.json");
  Arena arena;
  auto header = Perimortem::Storage::Json::JsonRpc(
      arena, View::Bytes(json.c_str(), json.size()));
  uint32_t pos = header.get_params_offset();

  Node node;
  node.from_source(test, View::Byte(json), pos);

  ASSERT_TRUE(node.contains("processId"));
  ASSERT_EQ(node.at("processId").get_int(), 18186);
  ASSERT_EQ(node.at("clientInfo").at("name").get_string(),
            View::Byte("VisualStudioCode"));
}