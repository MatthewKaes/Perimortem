// Perimortem Engine
// Copyright © Matt Kaes

#include <benchmark/benchmark.h>

#include "storage/formats/json.hpp"
#include "storage/formats/lazy_json.hpp"

#include <filesystem>
#include <fstream>

#define PERI_BENCH_3P
#ifdef PERI_BENCH_3P
// nlohmann json
#include "json.hpp"
#endif

// Number of words to load
const int tests = 10000;

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

auto generate_test_data()
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
    source[i] = load_json("core/tests/json/tokenize_rpc.json");

  return source;
}

#ifdef PERI_BENCH_3P
static void nlohmann_json_init_rpc(benchmark::State& state) {
  std::array<std::string, 4> source =
      generate_test_data();

  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      auto data = nlohmann::json::parse(source[rand() % source.size()]);
      auto rpc_version = data["jsonrpc"];
      doNotOptimizeAway(rpc_version == "2.0");

      auto path = data["params"]["source"];
      auto size = path.size();
      doNotOptimizeAway(size);
    }
  }
}
BENCHMARK(nlohmann_json_init_rpc);
#endif

static void json_init_rpc(benchmark::State& state) {
  Perimortem::Memory::Arena json_arena;
  std::array<std::string, 4> source =
      generate_test_data();

  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      json_arena.reset();
      uint32_t position = 0;
      auto data = Perimortem::Storage::Json::parse(
          json_arena, source[rand() % source.size()], position);
      auto rpc_version = (*data)["jsonrpc"]->get_string();
      doNotOptimizeAway(rpc_version->get_view() == "2.0");

      auto path = (*(*data)["params"])["source"]->get_string();
      doNotOptimizeAway(path->get_size());
    }
  }
}
BENCHMARK(json_init_rpc);

static void lazy_json_init_rpc(benchmark::State& state) {
  std::array<std::string, 4> source =
      generate_test_data();

  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      const auto& selected = source[rand() % source.size()];
      auto data = Perimortem::Storage::Json::LazyNode(
          Perimortem::Memory::ManagedString(selected.c_str(), selected.size()));
      auto top_level = data.get_object<4>();
      doNotOptimizeAway(top_level["jsonrpc"].get_view().empty());

      auto path = top_level["params"]
                      .get_object<8>()["source"]
                      .get_string();
      doNotOptimizeAway(path.get_size());
    }
  }
}
BENCHMARK(lazy_json_init_rpc);

static void lazy_json_init_rpc_header_only(benchmark::State& state) {
  std::array<std::string, 4> source =
      generate_test_data();

  for (auto _ : state) {
    for (int i = 1; i <= tests; i++) {
      const auto& selected = source[rand() % source.size()];
      auto data = Perimortem::Storage::Json::LazyNode(
          Perimortem::Memory::ManagedString(selected.c_str(), selected.size()));
      auto top_level = data.get_object<3>();
      doNotOptimizeAway(top_level["jsonrpc"].get_view().empty());
      doNotOptimizeAway(top_level["id"].get_view().empty());
    }
  }
}
BENCHMARK(lazy_json_init_rpc_header_only);
