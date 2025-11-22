// Perimortem Engine
// Copyright © Matt Kaes

#include <benchmark/benchmark.h>

#include "storage/formats/json.hpp"

#include <filesystem>
#include <fstream>

#define PERI_BENCH_3P
#ifdef PERI_BENCH_3P
// nlohmann json
#include "json.hpp"
#endif

// Number of words to load
const int tests = 100;

template <class T>
void doNotOptimizeAway(T&& datum) {
  asm volatile("" : "+r"(datum));
}

auto load_json(const std::filesystem::path& p) -> std::string {
  std::ifstream ifs(p, std::ios::binary | std::ios::ate);
  std::ifstream::pos_type pos = ifs.tellg();

  if (pos == 0) {
    return "";
  }

  std::string data;
  data.resize(pos);
  ifs.seekg(0, std::ios::beg);
  ifs.read((char*)data.data(), pos);
  return data;
}

auto generate_test_data(const std::filesystem::path& p)
    -> std::array<std::string, 4> {
  std::array<std::string, 4> source;

  // TODO:
  // Loading 4 copies to index randomly helps provide a closer test to real
  // world conditions, however the branch predictor still does an amazing job
  // and still optimized core parse paths.
  //
  // Generating 4 similar inits wasn't enough to fully emulate real world
  // results so no point in bloating the repo.
  for (int i = 0; i < 4; i++)
    source[i] = load_json("core/tests/json/init_rpc.json");

  return source;
}

#ifdef PERI_BENCH_3P
static void nlohmann_json_init_rpc(benchmark::State& state) {
  std::array<std::string, 4> source =
      generate_test_data("core/tests/json/init_rpc.json");

  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      auto data = nlohmann::json::parse(source[rand() % source.size()]);
      auto rpc_version = data["jsonrpc"];
      doNotOptimizeAway(rpc_version == "2.0");

      auto path = data["params"]["rootPath"];
      doNotOptimizeAway(rpc_version == "/home/test/Perimortem/tetrodotoxin/tests/scripts");
    }
  }
}
BENCHMARK(nlohmann_json_init_rpc);
#endif

static void json_init_rpc(benchmark::State& state) {
  Perimortem::Memory::Arena json_arena;
  std::array<std::string, 4> source =
      generate_test_data("core/tests/json/init_rpc.json");

  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      json_arena.reset();
      uint32_t position = 0;
      auto data = Perimortem::Storage::Json::parse(json_arena, source[rand() % source.size()], position);
      auto rpc_version = (*data)["jsonrpc"]->get_string();
      doNotOptimizeAway(rpc_version->get_view() == "2.0");

      auto path = (*(*data)["params"])["rootPath"]->get_string();
      doNotOptimizeAway(path->get_view() == "/home/test/Perimortem/tetrodotoxin/tests/scripts");
    }
  }
}
BENCHMARK(json_init_rpc);