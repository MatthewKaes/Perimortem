// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/benchmark.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/serialization/json/node.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Serialization;
using namespace Validation;

static Static::Bytes<1 << 15> json_data;
static Writer::Textual json_text(json_data);
static constexpr Count batch_count = 1024;

auto load_json(View::Bytes source_path) -> void {
  File source_file;
  source_file.read(source_path);

  json_text.set_pointer(0);
  json_text << source_file.get_view();
}

static Harness JsonSmall = {
  .name = "Json"_view,
  .setup = []() { load_json("validation/data/json/small.json"_view); },
};

PERIMORTEM_BENCHMARK(JsonSmall, small) {
  Count size = 0;

  Allocator::Arena arena;
  for (Count i = 0; i < batch_count; i++) {
    Json::Node node;
    node.parse(arena, json_text);
    size += node.get_size();
    arena.reset();
  }

  Benchmark::prevent_optimization(size);
}

static Harness JsonTokenizeRpc = {
  .name = "Json"_view,
  .setup = []() { load_json("validation/data/json/tokenize_rpc.json"_view); },
};

PERIMORTEM_BENCHMARK(JsonTokenizeRpc, tokenize_rpc) {
  Count size = 0;

  Allocator::Arena arena;
  for (Count i = 0; i < batch_count; i++) {
    Json::Node node;
    node.parse(arena, json_text);
    size += node.get_size();
    arena.reset();
  }

  Benchmark::prevent_optimization(size);
}

static Harness JsonInitRpc = {
  .name = "Json"_view,
  .setup = []() { load_json("validation/data/json/init_rpc.json"_view); },
};

PERIMORTEM_BENCHMARK(JsonInitRpc, jsonrpc_init) {
  Count size = 0;

  Allocator::Arena arena;
  for (Count i = 0; i < batch_count; i++) {
    Json::Node node;
    node.parse(arena, json_text);
    size += node.get_size();
    arena.reset();
  }

  Benchmark::prevent_optimization(size);
}
