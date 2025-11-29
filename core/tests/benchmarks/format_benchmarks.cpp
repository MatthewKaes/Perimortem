// Perimortem Engine
// Copyright © Matt Kaes

#include <benchmark/benchmark.h>

#include "storage/formats/base64.hpp"
#include "storage/formats/json.hpp"
#include "storage/formats/lazy_json.hpp"

#include <filesystem>
#include <fstream>

#ifdef PERI_BENCH_3P
// nlohmann json
#include "json.hpp"
#endif

// Don't constexpr as clang completely chokes and creates a ton of overhead.
auto test_count_per_round() -> uint32_t;

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

auto generate_test_data() -> std::array<std::string, 4> {
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
[[maybe_unused]] static void nlohmann_json_rpc(benchmark::State& state) {
  std::array<std::string, 4> source = generate_test_data();

  for (auto _ : state) {
    for (int i = 1; i <= test_count_per_round(); i++) {
      auto data = nlohmann::json::parse(source[rand() % source.size()]);
      auto rpc_version = data["jsonrpc"];
      doNotOptimizeAway(rpc_version == "2.0");

      auto path = data["params"]["source"];
      auto size = path.size();
      doNotOptimizeAway(size);
    }
  }
}

[[maybe_unused]] static void nlohmann_json_rpc_with_decode(
    benchmark::State& state) {
  std::array<std::string, 4> source = generate_test_data();

  for (auto _ : state) {
    for (int i = 1; i <= test_count_per_round(); i++) {
      auto data = nlohmann::json::parse(source[rand() % source.size()]);
      auto rpc_version = data["jsonrpc"];
      doNotOptimizeAway(rpc_version == "2.0");

      auto path = data["params"]["source"];
      Perimortem::Storage::Base64::Decoded decode(
          Perimortem::Memory::ManagedString(path.get<std::string>()));
      doNotOptimizeAway(decode.get_view().empty());
    }
  }
}
#endif

[[maybe_unused]] static void lazy_json_rpc(benchmark::State& state) {
  std::array<std::string, 4> source = generate_test_data();

  for (auto _ : state) {
    for (int i = 1; i <= test_count_per_round(); i++) {
      const auto& selected = source[rand() % source.size()];
      auto data = Perimortem::Storage::Json::LazyNode(
          Perimortem::Memory::ManagedString(selected.c_str(), selected.size()));
      auto top_level = data.get_object<4>();
      doNotOptimizeAway(top_level["jsonrpc"].get_view().empty());
      doNotOptimizeAway(top_level["id"].get_view().empty());

      auto path = top_level["params"].get_object<8>()["source"].get_string();
      doNotOptimizeAway(path.get_size());
    }
  }
}

[[maybe_unused]] static void lazy_json_rpc_header_only(
    benchmark::State& state) {
  std::array<std::string, 4> source = generate_test_data();

  for (auto _ : state) {
    for (int i = 1; i <= test_count_per_round(); i++) {
      const auto& selected = source[rand() % source.size()];
      auto data = Perimortem::Storage::Json::LazyNode(
          Perimortem::Memory::ManagedString(selected.c_str(), selected.size()));
      auto top_level = data.get_object<3>();
      doNotOptimizeAway(top_level["jsonrpc"].get_view().empty());
      doNotOptimizeAway(top_level["id"].get_view().empty());
    }
  }
}

[[maybe_unused]] static void json_rpc(benchmark::State& state) {
  Perimortem::Memory::Arena json_arena;
  std::array<std::string, 4> source = generate_test_data();

  for (auto _ : state) {
    for (int i = 1; i <= test_count_per_round(); i++) {
      json_arena.reset();
      uint32_t position = 0;
      auto data = Perimortem::Storage::Json::parse(
          json_arena, source[rand() % source.size()], position);
      auto rpc_version = (*data)["jsonrpc"]->get_string();
      doNotOptimizeAway(rpc_version->get_view() == "2.0");
    }
  }
}

[[maybe_unused]] static void json_rpc_with_decode(benchmark::State& state) {
  Perimortem::Memory::Arena json_arena;
  std::array<std::string, 4> source = generate_test_data();

  for (auto _ : state) {
    for (int i = 1; i <= test_count_per_round(); i++) {
      json_arena.reset();
      uint32_t position = 0;
      auto data = Perimortem::Storage::Json::parse(
          json_arena, source[rand() % source.size()], position);
      auto rpc_version = (*data)["jsonrpc"]->get_string();
      doNotOptimizeAway(rpc_version->get_view() == "2.0");

      auto path = (*(*data)["params"])["source"]->get_string();
      Perimortem::Storage::Base64::Decoded decode{
          Perimortem::Memory::ManagedString(*path)};
      doNotOptimizeAway(decode.get_view().empty());
    }
  }
}

[[maybe_unused]] static void just_source_decode(benchmark::State& state) {
  Perimortem::Memory::Arena json_arena;
  std::array<std::string, 4> source = generate_test_data();

  json_arena.reset();
  uint32_t position = 0;
  auto data = Perimortem::Storage::Json::parse(
      json_arena, source[rand() % source.size()], position);
  auto rpc_version = (*data)["jsonrpc"]->get_string();
  doNotOptimizeAway(rpc_version->get_view() == "2.0");

  auto path = (*(*data)["params"])["source"]->get_string();
  for (auto _ : state) {
    for (int i = 1; i <= test_count_per_round(); i++) {
      Perimortem::Storage::Base64::Decoded decode{
          Perimortem::Memory::ManagedString(*path)};
      doNotOptimizeAway(decode.get_view().empty());
    }
  }
}


// Configuration

#ifdef PERI_BENCH_3P
BENCHMARK(nlohmann_json_rpc);
BENCHMARK(nlohmann_json_rpc_with_decode);
#endif
BENCHMARK(lazy_json_rpc);
BENCHMARK(lazy_json_rpc_header_only);
BENCHMARK(json_rpc);
BENCHMARK(json_rpc_with_decode);
BENCHMARK(just_source_decode);

// Standard Benchmarking
auto test_count_per_round() -> uint32_t {
  return 10000;
}

// // Stress for Porfiling
// auto test_count_per_round() -> uint32_t {
//   return 1000000;
// }