// Perimortem Engine
// Copyright © Matt Kaes

#include <gtest/gtest.h>

#include "parser/script.hpp"
#include "parser/token.hpp"
#include "parser/type.hpp"

#include "types/program.hpp"
#include "types/std/byt.hpp"
#include "types/std/dec.hpp"
#include "types/std/int.hpp"
#include "types/std/num.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

// #define VERBOSE_ERRORS

#ifdef VERBOSE_ERRORS
#define ERROR_TEST(count)              \
  EXPECT_EQ(ctx.errors.size(), count); \
  for (const auto& err : ctx.errors) { \
    std::cout << err.get_message();    \
  }                                    \
  errors.clear();
#else
#define ERROR_TEST(count)                \
  EXPECT_EQ(ctx.errors.size(), count);   \
  if (ctx.errors.size() != count) {      \
    for (const auto& err : ctx.errors) { \
      std::cout << err.get_message();    \
    }                                    \
  }                                      \
  errors.clear();
#endif

using namespace Tetrodotoxin::Language::Parser;

struct ParserTests : public ::testing::Test {
 protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

auto read_all_bytes(const std::filesystem::path& p) -> std::string;
auto source_to_bytes(const char* source) -> std::string_view;

// Demonstrate some basic assertions.
TEST_F(ParserTests, empty) {
  Errors errors;
  std::string source = "";
  Tokenizer tokenizer(source);
  Types::Program host;

  auto script =
      Script::parse(host, errors, std::filesystem::current_path(), tokenizer);
  ASSERT_TRUE(script->is<Types::Library>());
  EXPECT_EQ(errors.size(), 2);
  EXPECT_EQ(script->doc, "");
  EXPECT_EQ(script->get_name(), std::filesystem::current_path());
}

// Demonstrate some basic assertions.
TEST_F(ParserTests, simple_ttx) {
  Errors errors;
  std::string source =
      read_all_bytes("tetrodotoxin/parser/tests/scripts/simple.ttx");
  Tokenizer tokenizer(source);
  Types::Program host;

  auto script = Script::parse(
      host, errors,
      std::filesystem::path("tetrodotoxin/parser/tests/scripts/simple.ttx"),
      tokenizer);
  EXPECT_EQ(errors.size(), 0);

  // Output any errors
  for (const auto& err : errors) {
    std::cout << err.get_message();
  }

  EXPECT_EQ(script->doc, "\n <Document String>\n\n");
  EXPECT_FALSE(script->is_entity);
}

// Demonstrate some basic assertions.
TEST_F(ParserTests, std_lib) {
  Errors errors;
  std::string source =
      read_all_bytes("tetrodotoxin/parser/tests/scripts/simple.ttx");
  Tokenizer tokenizer(source);
  Types::Program host;

  auto script = Script::parse(
      host, errors,
      std::filesystem::path("tetrodotoxin/parser/tests/scripts/simple.ttx"),
      tokenizer);
  EXPECT_EQ(errors.size(), 0);

  // Output any errors
  for (const auto& err : errors) {
    std::cout << err.get_message();
  }

  script->expand_scope();
}