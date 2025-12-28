// Perimortem Engine
// Copyright © Matt Kaes

#include <gtest/gtest.h>

#include "src/service.hpp"

#include <fstream>
#include <filesystem>

using namespace Tetrodotoxin::Lexical;
using namespace Tetrodotoxin::Lsp;

auto read_all_bytes(const std::filesystem::path &p) -> std::string {
  if(!std::filesystem::is_regular_file(p))
    return std::string();

  std::ifstream ifs(p, std::ios::binary | std::ios::ate);
  std::ifstream::pos_type pos = ifs.tellg();

  if (pos == 0)
    return std::string();

  std::string data;
  data.resize(pos);
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
  auto bytes = read_all_bytes("tetrodotoxin/parser/tests/scripts/Simple.ttx");
  Tokenizer t;
  t.parse(Perimortem::Memory::ByteView(bytes));

  // std::string result =
  //     Service::format(t, "Simple.ttx", "2.0", 0);

  // std::cout << result << std::endl;
}
