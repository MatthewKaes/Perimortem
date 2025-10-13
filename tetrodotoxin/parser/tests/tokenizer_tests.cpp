// Perimortem Engine
// Copyright © Matt Kaes

#include <gtest/gtest.h>

#include "parser/tokenizer.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

using namespace Tetrodotoxin::Language::Parser;

#define CLASS_DUMP(klass)                                \
  case Classifier::klass:                                \
    info_stream << std::left << std::setw(12) << #klass; \
    break;

auto dump_tokens(Tokenizer& tokenizer) -> std::string {
  std::stringstream info_stream;
  info_stream << "Token stream size: " << tokenizer.get_tokens().size() << std::endl;
  info_stream << std::left << std::setw(12) << "KLASS";
  info_stream << std::left << std::setw(8) << "LINE";
  info_stream << std::left << std::setw(8) << "COLUMN";
  info_stream << std::left << "  DATA" << std::endl;
  for (const auto& token : tokenizer.get_tokens()) {
    switch (token.klass) {
      CLASS_DUMP(Comment)
      CLASS_DUMP(String)
      CLASS_DUMP(Numeric)
      CLASS_DUMP(Float)
      CLASS_DUMP(Attribute)
      CLASS_DUMP(Identifier)
      CLASS_DUMP(Type)
      CLASS_DUMP(ScopeStart)
      CLASS_DUMP(ScopeEnd)
      CLASS_DUMP(GroupStart)
      CLASS_DUMP(GroupEnd)
      CLASS_DUMP(IndexStart)
      CLASS_DUMP(IndexEnd)
      CLASS_DUMP(Seperator)
      CLASS_DUMP(Assign)
      CLASS_DUMP(AddAssign)
      CLASS_DUMP(SubAssign)
      CLASS_DUMP(Define)
      CLASS_DUMP(EndStatement)
      CLASS_DUMP(AddOp)
      CLASS_DUMP(SubOp)
      CLASS_DUMP(DivOp)
      CLASS_DUMP(MulOp)
      CLASS_DUMP(ModOp)
      CLASS_DUMP(LessOp)
      CLASS_DUMP(GreaterOp)
      CLASS_DUMP(LessEqOp)
      CLASS_DUMP(GreaterEqOp)
      CLASS_DUMP(CmpOp)
      CLASS_DUMP(CallOp)
      CLASS_DUMP(AccessOp)
      CLASS_DUMP(AndOp)
      CLASS_DUMP(OrOp)
      CLASS_DUMP(NotOp)
      CLASS_DUMP(K_this)
      CLASS_DUMP(K_if)
      CLASS_DUMP(K_for)
      CLASS_DUMP(K_else)
      CLASS_DUMP(K_return)
      CLASS_DUMP(K_func)
      CLASS_DUMP(K_type)
      CLASS_DUMP(K_from)
      CLASS_DUMP(K_debug)
      CLASS_DUMP(K_warning)
      CLASS_DUMP(K_error)
      CLASS_DUMP(K_true)
      CLASS_DUMP(K_false)
      CLASS_DUMP(K_new)
      CLASS_DUMP(K_init)
      default:
        // Unknown
        break;
    }

    info_stream << std::right << std::setw(8) << token.location.line;
    info_stream << std::right << std::setw(8) << token.location.column;
    info_stream << "  \""
                << std::string_view((char*)token.data.data(), token.data.size())
                << "\"" << std::endl;
  }

  return info_stream.str();
}

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
  EXPECT_TRUE(str == dump_tokens(t));
}