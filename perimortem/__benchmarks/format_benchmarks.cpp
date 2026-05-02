// Perimortem Engine
// Copyright © Matt Kaes

#include <benchmark/benchmark.h>

#include "storage/formats/base64.hpp"
#include "storage/formats/json.hpp"
#include "storage/formats/rpc_header.hpp"

#include <filesystem>
#include <fstream>

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
    source[i] = load_json("perimortem/tests/json/tokenize_rpc.json");

  return source;
}

[[maybe_unused]] static void rpc_header(benchmark::State& state) {
  std::array<std::string, 4> source = generate_test_data();
  Perimortem::Memory::Allocator::Arena arena;

  for (auto _ : state) {
    for (int i = 1; i <= test_count_per_round(); i++) {
      const auto& selected = source[rand() % source.size()];
      auto data = Perimortem::Storage::Json::JsonRpc(
          arena, Core::View::Bytes(selected));
      doNotOptimizeAway(data.get_id().empty());
    }
  }
}

[[maybe_unused]] static void json_rpc(benchmark::State& state) {
  Perimortem::Memory::Allocator::Arena json_arena;
  std::array<std::string, 4> source = generate_test_data();

  for (auto _ : state) {
    for (int i = 1; i <= test_count_per_round(); i++) {
      json_arena.reset();
      Perimortem::Storage::Json::Node node;
      uint32_t position = 0;
      node.from_source(
          json_arena,
          Core::View::Bytes(source[rand() % source.size()]),
          position);
      auto rpc_version = node["jsonrpc"].get_string();
      doNotOptimizeAway(rpc_version == "2.0"_bv);
    }
  }
}

[[maybe_unused]] static void json_rpc_from_header(benchmark::State& state) {
  Perimortem::Memory::Allocator::Arena json_arena;
  std::array<std::string, 4> source = generate_test_data();

  for (auto _ : state) {
    for (int i = 1; i <= test_count_per_round(); i++) {
      const auto& selected = source[rand() % source.size()];
      auto header = Perimortem::Storage::Json::JsonRpc(
          json_arena, Core::View::Bytes(selected));
      uint32_t position = header.get_params_offset();

      json_arena.reset();
      Perimortem::Storage::Json::Node node;
      node.from_source(json_arena, Core::View::Bytes(selected),
                       position);
      auto source = node["source"].get_string();
      doNotOptimizeAway(source.get_size());
    }
  }
}

[[maybe_unused]] static void json_rpc_with_decode(benchmark::State& state) {
  Perimortem::Memory::Allocator::Arena json_arena;
  std::array<std::string, 4> source = generate_test_data();

  for (auto _ : state) {
    for (int i = 1; i <= test_count_per_round(); i++) {
      json_arena.reset();
      uint32_t position = 0;
      Perimortem::Storage::Json::Node node;
      node.from_source(
          json_arena,
          Core::View::Bytes(source[rand() % source.size()]),
          position);

      auto path = node["params"]["source"].get_string();
      Perimortem::Storage::Base64::Decoded decode(
          json_arena, Core::View::Bytes(path));
      doNotOptimizeAway(decode.get_view().empty());
    }
  }
}

[[maybe_unused]] static void just_source_decode(benchmark::State& state) {
  Perimortem::Memory::Allocator::Arena json_arena;
  std::array<std::string, 4> source = generate_test_data();

  json_arena.reset();
  uint32_t position = 0;
  Perimortem::Storage::Json::Node node;
  node.from_source(json_arena,
                   Core::View::Bytes(source[rand() % source.size()]),
                   position);

  auto path = node["params"]["source"].get_string();
  for (auto _ : state) {
    for (int i = 1; i <= test_count_per_round(); i++) {
      Perimortem::Storage::Base64::Decoded decode(
          json_arena, Core::View::Bytes(path));
      doNotOptimizeAway(decode.get_view().empty());
    }
  }
}

// Configuration

#ifdef PERI_BENCH_3P
BENCHMARK(nlohmann_json_rpc);
BENCHMARK(nlohmann_json_rpc_with_decode);
#endif
BENCHMARK(rpc_header);
BENCHMARK(json_rpc);
BENCHMARK(json_rpc_from_header);
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