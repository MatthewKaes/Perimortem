// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/process/child.hpp"
#include "validation/unit_test.hpp"

using namespace Perimortem;
using namespace Validation;

static Harness CppApi = {
  .name = "Tetrodotoxin::Terminal::Abi::Cpp"_view,
};

PERIMORTEM_UNIT_TEST(CppApi, generated_bytes_surface) {
  Process::Request request = {
    .executable = ".bin/bin/validation/cpp_api_integration"_view,
  };
  Process::Observation observation = Process::run(request);

  EXPECT(observation.launched);
  EXPECT_NOT(observation.timed_out);
  EXPECT_EQ(observation.exit_status, 0);
  EXPECT(observation.standard_output.is_empty());
  EXPECT(observation.standard_error.is_empty());
  EXPECT(observation.runner_error.is_empty());
}
