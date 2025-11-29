// Perimortem Engine
// Copyright © Matt Kaes

#include <gtest/gtest.h>

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
  std::string json = load_text("core/tests/json/init_rpc.json");
  auto data = parse(test, ManagedString(json), pos);

  ASSERT_NE(data, nullptr);
  EXPECT_TRUE(data->contains("method"));
  EXPECT_EQ(data->at("method")->get_string().get_view(), "initialize");

  EXPECT_TRUE(data->contains("jsonrpc"));
  EXPECT_EQ(data->at("jsonrpc")->get_string().get_view(), "2.0");

  EXPECT_TRUE(data->contains("id"));
  EXPECT_EQ(data->at("id")->get_int(), 10);

  EXPECT_TRUE(data->contains("params"));
  EXPECT_TRUE(data->at("params")->contains("rootPath"));
  auto path = data->at("params")->at("rootPath")->get_string();
  EXPECT_EQ(path.get_view(),
            "/home/test/Perimortem/tetrodotoxin/tests/scripts");
}

TEST_F(JsonTests, tokenize_rpc) {
  Arena test;
  uint32_t pos = 0;
  std::string json = load_text("core/tests/json/tokenize_rpc.json");
  auto data = parse(test, ManagedString(json), pos);

  ASSERT_NE(data, nullptr);
  EXPECT_TRUE(data->contains("method"));
  EXPECT_EQ(data->at("method")->get_string().get_view(), "tokenize");

  EXPECT_TRUE(data->contains("jsonrpc"));
  EXPECT_EQ(data->at("jsonrpc")->get_string().get_view(), "2.0");

  EXPECT_TRUE(data->contains("id"));
  EXPECT_EQ(data->at("id")->get_int(), 1);

  ASSERT_TRUE(data->contains("params"));
  auto params = data->at("params");
  ASSERT_TRUE(params->contains("source"));
}

TEST_F(JsonTests, rpc_header) {
  std::string json = load_text("core/tests/json/init_rpc.json");

  auto data = Perimortem::Storage::Json::RpcHeader(
      Perimortem::Memory::ManagedString(json.c_str(), json.size()));

  EXPECT_EQ(data.get_method(), "initialize");
  EXPECT_EQ(data.get_version(), "2.0");
  EXPECT_EQ(data.get_id(), 10);
  EXPECT_EQ(data.get_params_offset(), 56);
}

TEST_F(JsonTests, jsonrpc) {
  Arena test;
  uint32_t pos = 0;
  std::string json = load_text("core/tests/json/initialized_rpc.json");
  auto data = parse(test, ManagedString(json), pos);

  ASSERT_NE(data, nullptr);
  EXPECT_TRUE(data->contains("method"));
  EXPECT_EQ(data->at("method")->get_string().get_view(), "initialized");

  EXPECT_TRUE(data->contains("jsonrpc"));
  EXPECT_EQ(data->at("jsonrpc")->get_string().get_view(), "2.0");
  ASSERT_TRUE(data->contains("params"));

  ASSERT_NE(data, nullptr);
}

TEST_F(JsonTests, jsonrpc_from_header) {
  Arena test;
  std::string json = load_text("core/tests/json/init_rpc.json");
  auto header = Perimortem::Storage::Json::RpcHeader(
      Perimortem::Memory::ManagedString(json.c_str(), json.size()));
  uint32_t pos = header.get_params_offset();
  auto data = parse(test, ManagedString(json), pos);

  ASSERT_NE(data, nullptr);
  ASSERT_TRUE(data->contains("processId"));
  ASSERT_EQ(data->at("processId")->get_int(), 18186);
  ASSERT_EQ(data->at("clientInfo")->at("name")->get_string(),
            ManagedString("VisualStudioCode"));
}