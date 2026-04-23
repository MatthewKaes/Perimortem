// Perimortem Engine
// Copyright © Matt Kaes

#include <gtest/gtest.h>

#include "parser/script.hpp"

#include "lexical/token.hpp"
#include "types/compiler/attribute.hpp"
#include "types/program.hpp"

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

using namespace Tetrodotoxin::Parser;
using namespace Tetrodotoxin::Types;

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
  Program host;

  Script script(true);
  auto library =
      script.parse(host, errors, std::filesystem::current_path(), source);
  ASSERT_TRUE(library.is<Library>());
  EXPECT_EQ(errors.size(), 1);
  EXPECT_EQ(library.get_doc(), "");
  EXPECT_EQ(library.get_name(), "");
}

// Demonstrate some basic assertions.
TEST_F(ParserTests, data_ttx) {
  Errors errors;
  std::string source = read_all_bytes("tetrodotoxin/tests/scripts/Data.ttx");
  Program host;

  Script script(true);
  auto library = script.parse(
      host, errors,
      std::filesystem::path("tetrodotoxin/tests/scripts/Data.ttx"), source);
  EXPECT_EQ(errors.size(), 0);

  // Output any errors
  for (const auto& err : errors) {
    std::cout << err.get_message();
  }

  EXPECT_EQ(library.get_doc(), "\nA basic package for test parsing.\n\n");
  EXPECT_EQ(library.get_name(), "Data");
  auto maybe_attribute = library.resolve_scope("@Name");
  ASSERT_TRUE(maybe_attribute);
  auto data_attribute = maybe_attribute->cast<Compiler::Attribute>();
  ASSERT_TRUE(data_attribute);

  EXPECT_EQ(data_attribute->value.get_view(), "Data");

  EXPECT_FALSE(library.is_entity());
}

TEST_F(ParserTests, leak_test) {
  std::string source = read_all_bytes("tetrodotoxin/parser/scripts/Simple.ttx");
  Program host;

  auto initial_state = Perimortem::Memory::Bibliotheca::reserved_size();
  for (int i = 0; i < 100; i++) {
    Errors errors;
    Script script(true);
    script.parse(
        host, errors,
        std::filesystem::path("tetrodotoxin/parser/scripts/Simple.ttx"),
        source);
  }
  EXPECT_EQ(initial_state, Perimortem::Memory::Bibliotheca::reserved_size());
}