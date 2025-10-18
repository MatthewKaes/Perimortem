// Perimortem Engine
// Copyright © Matt Kaes

#include <gtest/gtest.h>

#include "src/service.hpp"

#include <fstream>
#include <filesystem>

using namespace Tetrodotoxin::Language::Parser;
using namespace Tetrodotoxin::Lsp;

auto read_all_bytes(const std::filesystem::path &p) -> Bytes {
  if(!std::filesystem::is_regular_file(p))
    return Bytes();

  std::ifstream ifs(p, std::ios::binary | std::ios::ate);
  std::ifstream::pos_type pos = ifs.tellg();

  if (pos == 0)
    return Bytes();

  Bytes data(pos);
  ifs.seekg(0, std::ios::beg);
  ifs.read((char *)data.data(), pos);
  return data;
}

struct TtxLspServiceTests : public ::testing::Test {
 protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

// Demonstrate some basic assertions.
TEST_F(TtxLspServiceTests, format) {
  auto bytes = read_all_bytes("tetrodotoxin/parser/tests/scripts/Rect.ttx");
  Tokenizer t(bytes);

  std::string result =
      Service::format(t, "Rect.ttx", "2.0", 0);

  std::cout << result << std::endl;
}
