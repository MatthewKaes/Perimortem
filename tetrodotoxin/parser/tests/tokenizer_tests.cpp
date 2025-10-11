// Perimortem Engine
// Copyright © Matt Kaes

#include <gtest/gtest.h>

#include "parser/tokenizer.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

using namespace Tetrodotoxin::Language::Parser;

struct TokenizerTests : public ::testing::Test {
protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

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

void compare_tokens(const Token &result, Classifier klass,
                    std::string_view data, Location loc) {
  EXPECT_EQ(result.klass, klass);

  EXPECT_EQ(result.location.line, loc.line);
  EXPECT_EQ(result.location.column, loc.column);

  ASSERT_EQ(result.data.size(), data.size());
  for (int i = 0; i < data.size(); ++i) {
    EXPECT_EQ(result.data[i], data[i])
        << "views differ at byte " << std::hex << i;
  }
}

auto source_to_bytes(const char *source) -> ByteView {
  return ByteView((const uint8_t *)source, strlen(source));
}

#define START_CHECK() int check_index = 0;
#define TOKEN_CHECK(klass, data, row, col)                                     \
  compare_tokens(t.get_tokens()[check_index], Classifier::klass, data,         \
                 Location(0, 0, row, col));                                          \
  check_index++;

// Demonstrate some basic assertions.
TEST_F(TokenizerTests, empty) {
  Tokenizer t(source_to_bytes(R"()"));
  ASSERT_EQ(t.get_tokens().size(), 1);
  ASSERT_EQ(t.get_tokens()[0].klass, Classifier::EndOfStream);
}

// Demonstrate some basic assertions.
TEST_F(TokenizerTests, just_whitespace) {
  Tokenizer t(source_to_bytes("     \n\n \t  "));
  ASSERT_EQ(t.get_tokens().size(), 1);
  ASSERT_EQ(t.get_tokens()[0].klass, Classifier::EndOfStream);
}

// Demonstrate some basic assertions.
TEST_F(TokenizerTests, numbers) {
  Tokenizer t(source_to_bytes("0 0. 1.123 .0 1var21"));
  EXPECT_EQ(t.get_tokens().size(), 8);

  // Token Stream
  START_CHECK()
  TOKEN_CHECK(Numeric, "0", 1, 1);
  TOKEN_CHECK(Float, "0.", 1, 3);
  TOKEN_CHECK(Float, "1.123", 1, 6);
  TOKEN_CHECK(AccessOp, ".", 1, 12);
  TOKEN_CHECK(Numeric, "0", 1, 13);
  TOKEN_CHECK(Numeric, "1", 1, 15);
  TOKEN_CHECK(Identifier, "var21", 1, 16);
  TOKEN_CHECK(EndOfStream, "", 1, sizeof("0 0. 1.123 .0 1var21"));
}

// Demonstrate some basic assertions.
TEST_F(TokenizerTests, token_rect_ttx) {
  auto bytes = read_all_bytes("tetrodotoxin/parser/tests/scripts/Rect.ttx");
  Tokenizer t(bytes);

  auto token_stream = read_all_bytes("tetrodotoxin/parser/tests/token_streams/Rect.ttx");
  std::string str = std::string((char *)token_stream.data(), token_stream.size());
  std::cout << str;
  EXPECT_TRUE(str == t.dump_tokens());
}