// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/algorithm/search.hpp"

#include "validation/benchmark.hpp"

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

using namespace Perimortem::Core;
using namespace Validation;
static constexpr Count batch_count = 1024;

static constexpr Static::Bytes source =
    "package engine.renderer\n"
    "\n"
    "using graphics.pipeline\n"
    "using math.matrix\n"
    "\n"
    "struct RenderPass {\n"
    "  width:  Int\n"
    "  height: Int\n"
    "  depth:  Int\n"
    "}\n"
    "\n"
    "func build_pipeline(pass: RenderPass) -> Pipeline {\n"
    "  if pass.width == 0 { return error(\"zero width\") }\n"
    "  while pending_jobs > 0 {\n"
    "    entity job = queue.next()\n"
    "    job.execute()\n"
    "  }\n"
    "  struct Config { samples: Int }\n"
    "  alias PipelineRef = Pipeline\n"
    "  return pipeline.build(pass)\n"
    "}\n"
    "\n"
    "func main() -> Int {\n"
    "  init pass = RenderPass { width: 1920, height: 1080, depth: 4 }\n"
    "  new result = build_pipeline(pass)\n"
    "  debug result.status\n"
    "  return 0\n"
    "}"_bytes;

static Harness AlgorithmSearch = {
  .name = "Searching"_view,
  .batch_count = batch_count,
};

PERIMORTEM_BENCHMARK(AlgorithmSearch, 2_byte) {
  Count result = 0;
  for (Count i = 0; i < batch_count; i++) {
    result += Algorithm::search(source.get_view(), "if"_view);
  }
  Benchmark::prevent_optimization(result);
}

PERIMORTEM_BENCHMARK(AlgorithmSearch, 4_byte) {
  Count result = 0;
  for (Count i = 0; i < batch_count; i++) {
    result += Algorithm::search(source.get_view(), "func"_view);
  }
  Benchmark::prevent_optimization(result);
}

PERIMORTEM_BENCHMARK(AlgorithmSearch, 6_byte) {
  Count result = 0;
  for (Count i = 0; i < batch_count; i++) {
    result += Algorithm::search(source.get_view(), "struct"_view);
  }
  Benchmark::prevent_optimization(result);
}

PERIMORTEM_BENCHMARK(AlgorithmSearch, 12_byte) {
  Count result = 0;
  for (Count i = 0; i < batch_count; i++) {
    result += Algorithm::search(source.get_view(), "alias Pipeli"_view);
  }
  Benchmark::prevent_optimization(result);
}

PERIMORTEM_BENCHMARK(AlgorithmSearch, 62_byte) {
  Count result = 0;
  for (Count i = 0; i < batch_count; i++) {
    result += Algorithm::search(
        source.get_view(),
        "init pass = RenderPass { width: 1920, height: 1080, depth: 4 }"_view);
  }
  Benchmark::prevent_optimization(result);
}

PERIMORTEM_BENCHMARK(AlgorithmSearch, total_miss) {
  Count result = 0;
  for (Count i = 0; i < batch_count; i++) {
    result += Algorithm::search(source.get_view(), "zarningz"_view);
  }
  Benchmark::prevent_optimization(result);
}

PERIMORTEM_BENCHMARK(AlgorithmSearch, near_miss) {
  Count result = 0;
  for (Count i = 0; i < batch_count; i++) {
    result += Algorithm::search(source.get_view(), "stract"_view);
  }
  Benchmark::prevent_optimization(result);
}

PERIMORTEM_BENCHMARK(AlgorithmSearch, near_miss_large) {
  Count result = 0;
  for (Count i = 0; i < batch_count; i++) {
    result += Algorithm::search(
        source.get_view(),
        "init pass = RendeaPass { width: 1920, height: 1080, depth: 5 }"_view);
  }
  Benchmark::prevent_optimization(result);
}
