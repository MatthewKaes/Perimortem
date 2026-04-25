// Perimortem Engine
// Copyright © Matt Kaes

// Google Test has been flaky with Bazel and Clang so using a simplified stack.
//
// This test framework isolates any of the C++ standard lib into a single
// compilation unit keeping test compilation as light weight as possible.

/* Explicity don't protect the include to catch multiple includes */
// #pragma once

namespace Validation::Test {

extern auto expected(bool value, bool actual = false) -> void;
extern auto expected(const char* value, bool actual = false) -> void;
extern auto expected(int value, bool actual = false) -> void;
extern auto expected(long long value, bool actual = false) -> void;
extern auto expected(unsigned long value, bool actual = false) -> void;
extern auto expected(unsigned long long value, bool actual = false) -> void;
extern auto expected_text(const unsigned char* value,
                          unsigned long long size,
                          bool actual = false) -> void;

extern auto do_nothing() -> void;

enum class TestResult {
  Pass,
  Failed,
};

using TestFunc = void (*)(TestResult& result);
using InitFunc = void (*)();
using SetupFunc = void (*)();
using TeardownFunc = void (*)();

struct Harness {
  const char* name = "";
  InitFunc init = do_nothing;
  SetupFunc setup = do_nothing;
  TeardownFunc teardown = do_nothing;
};

extern auto log_message(const char* file, int line, const char* msg) -> void;
extern auto create(const Harness& harness,
                   const char* name,
                   TestFunc func,
                   const char* file,
                   long line) -> void;

// Macros to inject type data without the test library needing to take
// a dependency on Perimortem directly.
#define PRINT_RESULT()                          \
  {                                             \
    Validation::Test::expected(__check, false); \
    Validation::Test::expected(__value, true);  \
  }

#define PRINT_TEXT()                                                           \
  {                                                                            \
    Validation::Test::expected_text(__check.get_data(), __check.get_size(), false); \
    Validation::Test::expected_text(__value.get_data(), __value.get_size(), true);  \
  }

#define EXPECT(expression)                                       \
  {                                                              \
    auto __value = (expression);                                 \
    auto __check = true;                                         \
    if (__value != __check) {                                    \
      Validation::Test::log_message(__FILE__, __LINE__,          \
                                    #expression " failed test"); \
      PRINT_RESULT();                                            \
      result = Validation::Test::TestResult::Failed;             \
    }                                                            \
  }

#define ASSERT(expression)                                       \
  {                                                              \
    auto __value = (expression);                                 \
    auto __check = true;                                         \
    if (__value != __check) {                                    \
      Validation::Test::log_message(__FILE__, __LINE__,          \
                                    #expression " failed test"); \
      PRINT_RESULT();                                            \
      result = Validation::Test::TestResult::Failed;             \
      return;                                                    \
    }                                                            \
  }

#define EXPECT_EQ(expression, expect)                            \
  {                                                              \
    auto __value = (expression);                                 \
    auto __check = (expect);                                     \
    if (__value != __check) {                                    \
      Validation::Test::log_message(__FILE__, __LINE__,          \
                                    #expression " != " #expect); \
      PRINT_RESULT();                                            \
      result = Validation::Test::TestResult::Failed;             \
    }                                                            \
  }

#define EXPECT_NEQ(expression, expect)                           \
  {                                                              \
    auto __value = (expression);                                 \
    auto __check = (expect);                                     \
    if (__value == __check) {                                    \
      Validation::Test::log_message(__FILE__, __LINE__,          \
                                    #expression " == " #expect); \
      PRINT_RESULT();                                            \
      result = Validation::Test::TestResult::Failed;             \
    }                                                            \
  }

#define ASSERT_EQ(expression, expect)                            \
  {                                                              \
    auto __value = (expression);                                 \
    auto __check = (expect);                                     \
    if (__value != __check) {                                    \
      Validation::Test::log_message(__FILE__, __LINE__,          \
                                    #expression " != " #expect); \
      PRINT_RESULT();                                            \
      result = Validation::Test::TestResult::Failed;             \
      return;                                                    \
    }                                                            \
  }

#define ASSERT_NEQ(expression, expect)                           \
  {                                                              \
    auto __value = (expression);                                 \
    auto __check = (expect);                                     \
    if (__value == __check) {                                    \
      Validation::Test::log_message(__FILE__, __LINE__,          \
                                    #expression " == " #expect); \
      PRINT_RESULT();                                            \
      result = Validation::Test::TestResult::Failed;             \
      return;                                                    \
    }                                                            \
  }

#define EXPECT_TEXT(expression, expect)                            \
  {                                                              \
    auto __value = (expression);                                 \
    auto __check = (expect);                                     \
    if (__value != __check) {                                    \
      Validation::Test::log_message(__FILE__, __LINE__,          \
                                    #expression " != " #expect); \
      PRINT_TEXT();                                            \
      result = Validation::Test::TestResult::Failed;             \
    }                                                            \
  }


#define ASSERT_TEXT(expression, expect)                            \
  {                                                              \
    auto __value = (expression);                                 \
    auto __check = (expect);                                     \
    if (__value != __check) {                                    \
      Validation::Test::log_message(__FILE__, __LINE__,          \
                                    #expression " != " #expect); \
      PRINT_TEXT();                                            \
      result = Validation::Test::TestResult::Failed;             \
      return;                                                    \
    }                                                            \
  }

class UnitTest {
 public:
  UnitTest(const Harness& harness,
           const char* name,
           TestFunc func,
           const char* file,
           long line) {
    create(harness, name, func, file, line);
  }
};

}  // namespace Validation::Test

#define PERIMORTEM_UNIT_TEST(harness, name)                              \
  auto __##name(Validation::Test::TestResult& result) -> void;           \
  namespace {                                                            \
  Validation::Test::UnitTest _reg_##name = {DynamicMap, #name, __##name, \
                                            __FILE__, __LINE__};         \
  }                                                                      \
  auto __##name(Validation::Test::TestResult& result) -> void
