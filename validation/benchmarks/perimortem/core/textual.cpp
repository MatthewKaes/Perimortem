// Perimortem Engine
// Copyright © Matt Kaes

#ifdef PERI_BENCH_CPP
#include <charconv>
#include <cstring>
#endif

#include "perimortem/core/reader/textual.hpp"

#include "validation/benchmark.hpp"

#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;
using namespace Validation;

// Buffer large enough for multiple reads/writes per iteration.
static constexpr Count io_buffer_size = 4096;
static Static::Bytes<io_buffer_size> io_buffer;

static Harness TextualBench = {
  .name = "Processing Text"_view,
};

PERIMORTEM_BENCHMARK(TextualBench, write_ints) {
  Writer::Textual writer(io_buffer.get_access());
  Static::Vector<Int, 8> values = {{
    10,
    1401,
    -120,
    987109,
    3,
    -10,
    242,
    5131,
  }};

  Count i = 0;
  while (writer.get_location() < io_buffer.get_size() - 256) {
    writer << values[i++ & 0x7];
  }
  Count loc = writer.get_location();
  Benchmark::prevent_optimization(loc);
}

PERIMORTEM_BENCHMARK(TextualBench, write_bools) {
  Writer::Textual writer(io_buffer.get_access());
  Static::Vector<Bool, 8> values;
  values[0] = True;
  values[1] = False;
  values[2] = True;
  values[3] = False;
  values[4] = False;
  values[5] = True;
  values[6] = False;
  values[7] = True;

  Count i = 0;
  while (writer.get_location() < io_buffer.get_size() - 256) {
    writer << values[i++ & 0x7];
  }
  Count loc = writer.get_location();
  Benchmark::prevent_optimization(loc);
}

PERIMORTEM_BENCHMARK(TextualBench, write_floats) {
  Writer::Textual writer(io_buffer.get_access());
  Static::Vector<Real_64, 8> values = {{
    1.0,
    -21.05,
    781.012,
    1200041.01,
    2390.0037,
    0.01234567,
    890.098,
    0,
  }};

  Count i = 0;
  while (writer.get_location() < io_buffer.get_size() - 256) {
    writer << values[i++ & 0x7];
  }
  Count loc = writer.get_location();
  Benchmark::prevent_optimization(loc);
}

#ifdef PERI_BENCH_CPP

static auto cpp_charconv_write_ints() -> void {
  char buf[io_buffer_size];
  char* end = buf + io_buffer_size;
  constexpr int values[] = {10, 1401, -120, 987109, 3, -10, 242, 5131};
  char* pos = buf;
  Count i = 0;
  while (pos < end - 256) {
    auto [ptr, ec] = std::to_chars(pos, end, values[i++ & 0x7]);
    if (ec == std::errc{}) {
      pos = ptr;
    }
  }
  Count loc = Count(pos - buf);
  Benchmark::prevent_optimization(loc);
}

static auto cpp_charconv_write_bools() -> void {
  char buf[io_buffer_size];
  char* end = buf + io_buffer_size;
  // charconv has no bool support so we write "true"/"false" directly to match
  // the output format Writer::Textual produces.
  constexpr bool values[] = {true, false, true, false, false, true, false, true};
  char* pos = buf;
  Count i = 0;
  while (pos < end - 256) {
    if (values[i++ & 0x7]) {
      memcpy(pos, "true", 4);
      pos += 4;
    } else {
      memcpy(pos, "false", 5);
      pos += 5;
    }
  }
  Count loc = Count(pos - buf);
  Benchmark::prevent_optimization(loc);
}

static auto cpp_charconv_write_floats() -> void {
  char buf[io_buffer_size];
  char* end = buf + io_buffer_size;
  constexpr double values[] = {
      1.0, -21.05, 781.012, 1200041.01, 2390.0037, 0.01234567, 890.098, 0.0};
  char* pos = buf;
  Count i = 0;
  while (pos < end - 256) {
    auto [ptr, ec] = std::to_chars(pos, end, values[i++ & 0x7]);
    if (ec == std::errc{}) {
      pos = ptr;
    }
  }
  Count loc = Count(pos - buf);
  Benchmark::prevent_optimization(loc);
}

static Benchmark::Comparison write_ints_comp = {
  .harness = &TextualBench,
  .label = "write ints"_view,
  .variants = {{"textual"_view, "write_ints"_view}},
};
PERIMORTEM_COMPARISON(write_ints_comp) {
  cpp_charconv_write_ints();
}

static Benchmark::Comparison write_bools_comp = {
  .harness = &TextualBench,
  .label = "write bools"_view,
  .variants = {{"textual"_view, "write_bools"_view}},
};
PERIMORTEM_COMPARISON(write_bools_comp) {
  cpp_charconv_write_bools();
}

static Benchmark::Comparison write_floats_comp = {
  .harness = &TextualBench,
  .label = "write floats"_view,
  .variants = {{"textual"_view, "write_floats"_view}},
};
PERIMORTEM_COMPARISON(write_floats_comp) {
  cpp_charconv_write_floats();
}

#endif  // PERI_BENCH_CPP
