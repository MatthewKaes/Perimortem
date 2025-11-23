// Perimortem Engine
// Copyright © Matt Kaes

#include <gtest/gtest.h>

#include "storage/formats/json.hpp"
#include "storage/formats/lazy_json.hpp"

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

auto load_json(const std::filesystem::path& p) -> std::string {
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
  std::string json = load_json("core/tests/json/init_rpc.json");
  auto data = parse(test, json, pos);

  ASSERT_NE(data, nullptr);
  ASSERT_TRUE(data->contains("method") && (*data)["method"]->get_string());
  auto method_name = *(*data)["method"]->get_string();
  EXPECT_EQ(method_name.get_view(), "initialize");

  ASSERT_TRUE(data->contains("jsonrpc") && (*data)["jsonrpc"]->get_string());
  auto jsonrpc_version = *(*data)["jsonrpc"]->get_string();
  EXPECT_EQ(jsonrpc_version.get_view(), "2.0");

  ASSERT_TRUE(data->contains("id") && (*data)["id"]->get_int());
  uint32_t id_value = *(*data)["id"]->get_int();
  EXPECT_EQ(id_value, 10);

  ASSERT_TRUE(data->contains("params"));
  auto params = (*data)["params"];
  ASSERT_TRUE(params->contains("rootPath"));
  auto path = *(*params)["rootPath"]->get_string();
  EXPECT_EQ(path.get_view(),
            "/home/test/Perimortem/tetrodotoxin/tests/scripts");
}

TEST_F(JsonTests, tokenize_rpc) {
  Arena test;
  uint32_t pos = 0;
  std::string json = load_json("core/tests/json/tokenize_rpc.json");
  auto data = parse(test, json, pos);

  ASSERT_NE(data, nullptr);
  ASSERT_TRUE(data->contains("method") && (*data)["method"]->get_string());
  auto method_name = *(*data)["method"]->get_string();
  EXPECT_EQ(method_name.get_view(), "tokenize");

  ASSERT_TRUE(data->contains("jsonrpc") && (*data)["jsonrpc"]->get_string());
  auto jsonrpc_version = *(*data)["jsonrpc"]->get_string();
  EXPECT_EQ(jsonrpc_version.get_view(), "2.0");

  ASSERT_TRUE(data->contains("id") && (*data)["id"]->get_int());
  uint32_t id_value = *(*data)["id"]->get_int();
  EXPECT_EQ(id_value, 1);

  ASSERT_TRUE(data->contains("params"));
  auto params = (*data)["params"];
  ASSERT_TRUE(params->contains("source"));
}

TEST_F(JsonTests, lazy_init_rpc) {
  std::string json = load_json("core/tests/json/init_rpc.json");

  auto data = Perimortem::Storage::Json::LazyNode(
      Perimortem::Memory::ManagedString(json.c_str(), json.size()));

  auto top_level = data.get_object<4>();
  ASSERT_TRUE(top_level.contains("method"));
  auto method_name = top_level["method"].get_string();
  EXPECT_EQ(method_name.get_view(), "initialize");

  ASSERT_TRUE(top_level.contains("jsonrpc"));
  auto rpc_version = top_level["jsonrpc"].get_string();
  EXPECT_EQ(rpc_version.get_view(), "2.0");

  ASSERT_TRUE(top_level.contains("id"));
  auto id_value = top_level["id"].get_int();
  EXPECT_EQ(id_value, 10);

  ASSERT_TRUE(top_level.contains("params"));
  auto path = top_level["params"].get_object<8>()["rootPath"].get_string();
  EXPECT_EQ(path.get_view(),
            "/home/test/Perimortem/tetrodotoxin/tests/scripts");
}

TEST_F(JsonTests, jsonrpc) {
  Arena test;
  uint32_t pos = 0;
  std::string json = load_json("core/tests/json/initialized_rpc.json");
  auto data = parse(test, json, pos);

  ASSERT_NE(data, nullptr);
  ASSERT_TRUE(data->contains("method") && (*data)["method"]->get_string());
  [[maybe_unused]] auto method_name = *(*data)["method"]->get_string();
  ASSERT_TRUE(data->contains("jsonrpc") && (*data)["jsonrpc"]->get_string());
  [[maybe_unused]] auto jsonrpc_version = *(*data)["jsonrpc"]->get_string();
  ASSERT_TRUE(data->contains("params"));

  ASSERT_NE(data, nullptr);
}