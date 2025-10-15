// Perimortem Engine
// Copyright © Matt Kaes

#include <benchmark/benchmark.h>

#include "parser/token.hpp"
#include "parser/ttx.hpp"

#include "concepts/narrow_resolver.hpp"
#include "concepts/sparse_index.hpp"
#include "concepts/sparse_lookup.hpp"

#include <fstream>
#include <filesystem>

using namespace Perimortem::Concepts;
using namespace Tetrodotoxin::Language::Parser;

auto read_all_bytes(const std::filesystem::path &p) -> Bytes {
  if(!std::filesystem::is_regular_file(p))
    return Bytes();

  std::ifstream ifs(p, std::ios::binary | std::ios::ate);
  std::ifstream::pos_type pos = ifs.tellg();

  if (pos == 0)
    return Bytes();

  Bytes data(pos);
  ifs.seekg(0, std::ios::beg);
  ifs.read((char *)data.data(), pos);
  return data;
}

template <class T>
void doNotOptimizeAway(T&& datum) {
  asm volatile("" : "+r"(datum));
}

static void tokenize_rect_ttx(benchmark::State& state) {
  for (auto _ : state) {
    auto bytes = read_all_bytes("tetrodotoxin/parser/tests/scripts/Rect.ttx");
    Tokenizer t(bytes);
    doNotOptimizeAway(t.get_tokens().size());
  }
}
BENCHMARK(tokenize_rect_ttx);
