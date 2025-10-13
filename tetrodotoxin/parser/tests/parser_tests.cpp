// Perimortem Engine
// Copyright © Matt Kaes

#include <gtest/gtest.h>

#include "parser/ast/type.hpp"
#include "parser/token.hpp"
#include "parser/ttx.hpp"

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

auto read_all_bytes(const std::filesystem::path& p) -> Bytes;
auto source_to_bytes(const char* source) -> ByteView;

// Demonstrate some basic assertions.
TEST_F(ParserTests, empty) {
  auto ttx = Ttx::parse("<empty>", source_to_bytes(""));
  EXPECT_EQ(ttx->get_errors().size(), 2);
}

// Demonstrate some basic assertions.
TEST_F(ParserTests, simple_ttx) {
  auto bytes = read_all_bytes("tetrodotoxin/parser/tests/scripts/simple.ttx");
  auto ttx = Ttx::parse("tetrodotoxin/parser/tests/scripts/simple.ttx", bytes);
  EXPECT_EQ(ttx->get_errors().size(), 0);

  // Output any errors
  for (const auto& err : ttx->get_errors()) {
    std::cout << err.get_message();
  }

  ASSERT_TRUE(ttx->documentation);
  EXPECT_EQ(ttx->documentation->body, "A basic class for testing parsing");
  EXPECT_EQ(ttx->name, "Test");
}

#define TYPE_TEST(type_name, base)                    \
  type = Type::parse(ctx);                            \
  for (const auto& err : ctx.errors) {                \
    std::cout << err.get_message();                   \
  }                                                   \
  ctx.advance();                                      \
  ASSERT_TRUE(type);                                  \
  EXPECT_EQ(ctx.errors.size(), 0);                    \
  EXPECT_FALSE(type->parameters);                     \
  EXPECT_EQ(type->handler, Type::Handler::type_name); \
  EXPECT_##base(Type::uses_stack(type->handler));     \
  errors.clear();

TEST_F(ParserTests, type_base) {
  std::optional<Type> type;
  auto source =
      read_all_bytes("tetrodotoxin/parser/tests/scripts/types/Base.ttx");
  Tokenizer tokenizer(source);
  Errors errors;
  Context ctx("tetrodotoxin/parser/tests/scripts/types/Base.ttx", source,
              tokenizer, errors);

  TYPE_TEST(Byt, TRUE);
  TYPE_TEST(Int, TRUE);
  TYPE_TEST(Num, TRUE);
  TYPE_TEST(Str, TRUE);
  TYPE_TEST(Vec, TRUE);
  TYPE_TEST(Any, TRUE);
}

TEST_F(ParserTests, type_list) {
  std::optional<Type> type;
  auto source =
      read_all_bytes("tetrodotoxin/parser/tests/scripts/types/List.ttx");
  Tokenizer tokenizer(source);
  Errors errors;
  Context ctx("tetrodotoxin/parser/tests/scripts/types/List.ttx", source,
              tokenizer, errors);

  // List
  type = Type::parse(ctx);
  ctx.advance();
  EXPECT_FALSE(type);
  ERROR_TEST(1);

  type = Type::parse(ctx);
  ctx.advance();
  EXPECT_FALSE(type);
  ERROR_TEST(1);

  type = Type::parse(ctx);
  ctx.advance();
  EXPECT_FALSE(type);
  ERROR_TEST(1);

  type = Type::parse(ctx);
  ctx.advance();
  ASSERT_TRUE(type);
  EXPECT_EQ(type->handler, Type::Handler::List);
  ASSERT_TRUE(type->parameters);
  ASSERT_TRUE(type->parameters->size() == 1);
  EXPECT_EQ(type->parameters->at(0).handler, Type::Handler::Int);
  ERROR_TEST(0);

  type = Type::parse(ctx);
  ctx.advance();
  EXPECT_FALSE(type);
  ERROR_TEST(1);

  type = Type::parse(ctx);
  ASSERT_TRUE(type);
  auto decent = &*type;
  ctx.advance();
  for (int i = 0; i < 3; i++) {
    EXPECT_EQ(decent->handler, Type::Handler::List);
    ASSERT_TRUE(decent->parameters);
    ASSERT_TRUE(decent->parameters->size() == 1);
    EXPECT_EQ(decent->parameters->at(0).handler,
              i == 2 ? Type::Handler::Int : Type::Handler::List);
    ERROR_TEST(0);
    decent = &decent->parameters->at(0);
  }
}

TEST_F(ParserTests, type_dict) {
  std::optional<Type> type;
  auto source =
      read_all_bytes("tetrodotoxin/parser/tests/scripts/types/Dict.ttx");
  Tokenizer tokenizer(source);
  Errors errors;
  Context ctx("tetrodotoxin/parser/tests/scripts/types/Dict.ttx", source,
              tokenizer, errors);
  // Dict
  type = Type::parse(ctx);
  ctx.advance();
  EXPECT_FALSE(type);
  ERROR_TEST(1);

  type = Type::parse(ctx);
  ctx.advance();
  EXPECT_FALSE(type);
  ERROR_TEST(1);

  type = Type::parse(ctx);
  ctx.advance();
  EXPECT_FALSE(type);
  ERROR_TEST(1);

  type = Type::parse(ctx);
  ctx.advance();
  EXPECT_FALSE(type);
  ERROR_TEST(1);

  type = Type::parse(ctx);
  ctx.advance();
  EXPECT_FALSE(type);
  ERROR_TEST(1);

  // type = Type::parse(ctx);
  // ctx.advance();
  // ASSERT_TRUE(type);
  // EXPECT_EQ(type->handler, Type::Handler::Dict);
  // ASSERT_TRUE(type->parameters);
  // ASSERT_TRUE(type->parameters->size() == 2);
  // EXPECT_EQ(type->parameters->at(0).handler, Type::Handler::Int);
  // EXPECT_EQ(type->parameters->at(1).handler, Type::Handler::Str);
  // ERROR_TEST(0);

  type = Type::parse(ctx);
  ctx.advance();
  EXPECT_FALSE(type);
  ERROR_TEST(1);

  type = Type::parse(ctx);
  ctx.advance();
  EXPECT_FALSE(type);
  ERROR_TEST(1);

  type = Type::parse(ctx);
  ctx.advance();
  EXPECT_FALSE(type);
  ERROR_TEST(1);

  type = Type::parse(ctx);
  ctx.advance();
  ASSERT_TRUE(type);
  EXPECT_EQ(type->handler, Type::Handler::Dict);
  ASSERT_TRUE(type->parameters);
  ASSERT_TRUE(type->parameters->size() == 2);
  EXPECT_EQ(type->parameters->at(0).handler, Type::Handler::Int);
  ASSERT_EQ(type->parameters->at(1).handler, Type::Handler::List);
  EXPECT_EQ(type->parameters->at(1).parameters->at(0).handler,
            Type::Handler::Defined);
  EXPECT_EQ(type->parameters->at(1).parameters->at(0).name, "Value");
  ERROR_TEST(0);

  type = Type::parse(ctx);
  ctx.advance();
  EXPECT_FALSE(type);
  ERROR_TEST(3);

  type = Type::parse(ctx);
  ctx.advance();
  ASSERT_TRUE(type);
  EXPECT_EQ(type->handler, Type::Handler::Dict);
  EXPECT_EQ(type->parameters->at(0).handler, Type::Handler::Int);
  EXPECT_EQ(type->parameters->at(1).handler, Type::Handler::Dict);
  EXPECT_EQ(type->parameters->at(1).parameters->at(0).handler,
            Type::Handler::Int);
  EXPECT_EQ(type->parameters->at(1).parameters->at(1).handler,
            Type::Handler::List);
  EXPECT_EQ(type->parameters->at(1).parameters->at(1).parameters->at(0).handler,
            Type::Handler::Defined);
  EXPECT_EQ(type->parameters->at(1).parameters->at(1).parameters->at(0).name,
            "Value");
  ERROR_TEST(0);

  type = Type::parse(ctx);
  ctx.advance();
  EXPECT_FALSE(type);
  ERROR_TEST(2);
}

TEST_F(ParserTests, type_func) {
  // std::optional<Type> type;
  // auto source =
  //     read_all_bytes("tetrodotoxin/parser/tests/scripts/types/Func.ttx");
  // Tokenizer tokenizer(source);
  // Errors errors;
  // Context ctx("tetrodotoxin/parser/tests/scripts/types/Func.ttx", source,
  //             tokenizer, errors);

  // type = Type::parse(ctx);
  // ctx.advance();
  // EXPECT_FALSE(type);
  // ERROR_TEST(1);

  // type = Type::parse(ctx);
  // ctx.advance();
  // EXPECT_FALSE(type);
  // ERROR_TEST(1);

  // type = Type::parse(ctx);
  // ctx.advance();
  // ASSERT_TRUE(type);
  // EXPECT_EQ(type->handler, Type::Handler::Func);
  // ASSERT_TRUE(type->parameters);
  // ASSERT_TRUE(type->parameters->size() == 1);
  // EXPECT_EQ(type->parameters->at(0).handler, Type::Handler::Void);
  // ERROR_TEST(0);

  // type = Type::parse(ctx);
  // ctx.advance();
  // ASSERT_TRUE(type);
  // EXPECT_EQ(type->handler, Type::Handler::Func);
  // ASSERT_TRUE(type->parameters);
  // ASSERT_TRUE(type->parameters->size() == 2);
  // EXPECT_EQ(type->parameters->at(0).handler, Type::Handler::Void);
  // EXPECT_EQ(type->parameters->at(1).handler, Type::Handler::Int);
  // ERROR_TEST(0);

  // type = Type::parse(ctx);
  // ctx.advance();
  // ASSERT_TRUE(type);
  // EXPECT_EQ(type->handler, Type::Handler::Func);
  // ASSERT_TRUE(type->parameters);
  // ASSERT_TRUE(type->parameters->size() == 3);
  // EXPECT_EQ(type->parameters->at(0).handler, Type::Handler::Void);
  // EXPECT_EQ(type->parameters->at(1).handler, Type::Handler::Int);
  // EXPECT_EQ(type->parameters->at(2).handler, Type::Handler::Str);
  // ERROR_TEST(0);

  // type = Type::parse(ctx);
  // ctx.advance();
  // ASSERT_TRUE(type);
  // EXPECT_EQ(type->handler, Type::Handler::Func);
  // ASSERT_TRUE(type->parameters);
  // ASSERT_TRUE(type->parameters->size() == 3);
  // EXPECT_EQ(type->parameters->at(0).handler, Type::Handler::Void);
  // EXPECT_EQ(type->parameters->at(1).handler, Type::Handler::List);
  // EXPECT_EQ(type->parameters->at(1).parameters->at(0).handler,
  //           Type::Handler::Int);
  // EXPECT_EQ(type->parameters->at(2).handler, Type::Handler::Str);
  // ERROR_TEST(0);
}