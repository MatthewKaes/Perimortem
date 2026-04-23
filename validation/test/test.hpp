// Perimortem Engine
// Copyright © Matt Kaes

// Google Test has been flaky with Bazel and Clang so using a simplified stack.
//
// This test framework isolates any of the C++ standard lib into a single
// compilation unit keeping test compilation as light weight as possible.

/* Explicity don't protect the include to catch multiple includes */
// #pragma once

namespace Validation::Test {

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

#define EXPECT(expression)                                     \
  {                                                            \
    if (!(expression)) {                                       \
      Validation::Test::log_message(__FILE__, __LINE__,        \
                                    #expression " was false"); \
      result = Validation::Test::TestResult::Failed;           \
    }                                                          \
  }

#define EXPECT_EQ(expression, expect)                            \
  {                                                              \
    auto __value = (expression);                                 \
    if (__value != (expect)) {                                   \
      Validation::Test::log_message(__FILE__, __LINE__,          \
                                    #expression " != " #expect); \
      result = Validation::Test::TestResult::Failed;             \
    }                                                            \
  }

#define EXPECT_NEQ(expression, expect)                           \
  {                                                              \
    auto __value = (expression);                                 \
    if (__value == (expect)) {                                   \
      Validation::Test::log_message(__FILE__, __LINE__,          \
                                    #expression " != " #expect); \
      result = Validation::Test::TestResult::Failed;             \
    }                                                            \
  }

#define ASSERT_EQ(expression, expect)                            \
  {                                                              \
    auto __value = (expression);                                 \
    if (__value != (expect)) {                                   \
      Validation::Test::log_message(__FILE__, __LINE__,          \
                                    #expression " != " #expect); \
      result = Validation::Test::TestResult::Failed;             \
      return;                                                    \
    }                                                            \
  }

#define ASSERT_NEQ(expression, expect)                           \
  {                                                              \
    auto __value = (expression);                                 \
    if (__value == (expect)) {                                   \
      Validation::Test::log_message(__FILE__, __LINE__,          \
                                    #expression " != " #expect); \
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
