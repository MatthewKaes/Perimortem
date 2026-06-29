// Perimortem Engine
// Copyright © Matt Kaes

// Simplified test framework for Perimortem. Now that validation links directly
// against Perimortem, the framework uses Perimortem types throughout rather
// than raw C equivalents.
//
// The framework still targets a single compilation unit (unit_test.cpp) to keep
// each test TU as lean as possible.

/* Explicitly don't protect the include to catch multiple includes */
// #pragma once

#include "validation/harness.hpp"

#include "perimortem/core/diagnostics/log.hpp"

namespace Validation::Test {
auto capture_sink(
    Perimortem::Core::Diagnostics::Log::Level level,
    Perimortem::Core::View::Bytes message,
    const Perimortem::Core::Diagnostics::Source& location) -> void;
auto captured_message() -> Perimortem::Core::View::Bytes;
auto error_contains(Perimortem::Core::View::Bytes message) -> Bool;

auto expected(Bool value, Bool actual) -> void;
auto expected(Perimortem::Core::View::Bytes value, Bool actual) -> void;
auto expected(Signed_16 value, Bool actual) -> void;
auto expected(Bits_16 value, Bool actual) -> void;
auto expected(Bits_32 value, Bool actual) -> void;
auto expected(Bits_64 value, Bool actual) -> void;
auto expected(Signed_32 value, Bool actual) -> void;
auto expected(Signed_32 value, Bool actual) -> void;
auto expected(Signed_64 value, Bool actual) -> void;
auto expected(CppSize value, Bool actual) -> void;
auto expected(Real_64 value, Bool actual) -> void;
auto expected_text(
    Perimortem::Core::View::Bytes value,
    Perimortem::Core::View::Bytes other,
    Bool actual) -> void;
auto expected_hex(Perimortem::Core::View::Bytes value, Bool actual) -> void;

auto do_nothing() -> void;

enum class TestResult {
  Pass,
  Failed,
};

using TestFunc = void (*)(TestResult& result);

auto log_message(
    Perimortem::Core::View::Bytes file,
    Count line,
    Perimortem::Core::View::Bytes msg) -> void;

auto create(
    const Harness& harness,
    Perimortem::Core::View::Bytes name,
    TestFunc func,
    Perimortem::Core::View::Bytes file,
    Count line) -> void;

#define PRINT_RESULT()                          \
  {                                             \
    Validation::Test::expected(__check, false); \
    Validation::Test::expected(__value, true);  \
  }

#define PRINT_TEXT()                                          \
  {                                                           \
    Validation::Test::expected_text(__check, __value, false); \
    Validation::Test::expected_text(__value, __check, true);  \
  }

#define PRINT_HEX()                                 \
  {                                                 \
    Validation::Test::expected_hex(__check, false); \
    Validation::Test::expected_hex(__value, true);  \
  }

#define EXPECT(expression)                                               \
  {                                                                      \
    auto __value = bool(expression);                                     \
    auto __check = true;                                                 \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " failed test"));                              \
      PRINT_RESULT();                                                    \
      result = Validation::Test::TestResult::Failed;                     \
    }                                                                    \
  }

#define ASSERT(expression)                                               \
  {                                                                      \
    auto __value = bool(expression);                                     \
    auto __check = true;                                                 \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " failed test"));                              \
      PRINT_RESULT();                                                    \
      result = Validation::Test::TestResult::Failed;                     \
      return;                                                            \
    }                                                                    \
  }

#define EXPECT_NOT(expression)                                           \
  {                                                                      \
    auto __value = bool(expression);                                     \
    auto __check = false;                                                \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " should be false"));                          \
      PRINT_RESULT();                                                    \
      result = Validation::Test::TestResult::Failed;                     \
    }                                                                    \
  }

#define ASSERT_NOT(expression)                                           \
  {                                                                      \
    auto __value = bool(expression);                                     \
    auto __check = false;                                                \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " should be false"));                          \
      PRINT_RESULT();                                                    \
      result = Validation::Test::TestResult::Failed;                     \
      return;                                                            \
    }                                                                    \
  }

#define EXPECT_EQ(expression, expect)                                    \
  {                                                                      \
    auto __value = (expression);                                         \
    auto __check = (expect);                                             \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " != " #expect));                              \
      PRINT_RESULT();                                                    \
      result = Validation::Test::TestResult::Failed;                     \
    }                                                                    \
  }

#define EXPECT_NEQ(expression, expect)                                   \
  {                                                                      \
    auto __value = (expression);                                         \
    auto __check = (expect);                                             \
    if (__value == __check) {                                            \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " == " #expect));                              \
      PRINT_RESULT();                                                    \
      result = Validation::Test::TestResult::Failed;                     \
    }                                                                    \
  }

#define ASSERT_EQ(expression, expect)                                    \
  {                                                                      \
    auto __value = (expression);                                         \
    auto __check = (expect);                                             \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " != " #expect));                              \
      PRINT_RESULT();                                                    \
      result = Validation::Test::TestResult::Failed;                     \
      return;                                                            \
    }                                                                    \
  }

#define ASSERT_NEQ(expression, expect)                                   \
  {                                                                      \
    auto __value = (expression);                                         \
    auto __check = (expect);                                             \
    if (__value == __check) {                                            \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " == " #expect));                              \
      PRINT_RESULT();                                                    \
      result = Validation::Test::TestResult::Failed;                     \
      return;                                                            \
    }                                                                    \
  }

#define EXPECT_TEXT(expression, expect)                                  \
  {                                                                      \
    auto __value = (expression);                                         \
    auto __check = (expect);                                             \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " != " #expect));                              \
      PRINT_TEXT();                                                      \
      result = Validation::Test::TestResult::Failed;                     \
    }                                                                    \
  }

#define ASSERT_TEXT(expression, expect)                                  \
  {                                                                      \
    auto __value = (expression);                                         \
    auto __check = (expect);                                             \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " != " #expect));                              \
      PRINT_TEXT();                                                      \
      result = Validation::Test::TestResult::Failed;                     \
      return;                                                            \
    }                                                                    \
  }

#define EXPECT_HEX(expression, expect)                                   \
  {                                                                      \
    auto __value = (expression);                                         \
    auto __check = (expect);                                             \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " != " #expect));                              \
      PRINT_HEX();                                                       \
      result = Validation::Test::TestResult::Failed;                     \
    }                                                                    \
  }

#define ASSERT_HEX(expression, expect)                                   \
  {                                                                      \
    auto __value = (expression);                                         \
    auto __check = (expect);                                             \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " != " #expect));                              \
      PRINT_HEX();                                                       \
      result = Validation::Test::TestResult::Failed;                     \
      return;                                                            \
    }                                                                    \
  }

class TestEntry {
 public:
  TestEntry(
      const Harness& harness,
      Perimortem::Core::View::Bytes name,
      TestFunc func,
      Perimortem::Core::View::Bytes file,
      Count line) {
    create(harness, name, func, file, line);
  }
};

}  // namespace Validation::Test

#define PERIMORTEM_UNIT_TEST(harness, name)                                 \
  auto __##harness##__##name(Validation::Test::TestResult& result) -> void; \
  namespace {                                                               \
  Validation::Test::TestEntry _reg_##harness##_##name = {                   \
    harness, #name##_view, __##harness##__##name,                           \
    Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__};         \
  }                                                                         \
  auto __##harness##__##name(Validation::Test::TestResult& result) -> void
