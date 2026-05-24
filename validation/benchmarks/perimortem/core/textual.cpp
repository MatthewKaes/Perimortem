// Perimortem Engine
// Copyright © Matt Kaes

#ifdef PERI_BENCH_CPP
#include <charconv>
#include <cstring>
#include <string_view>
#endif

#include "validation/benchmark.hpp"

#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"
#include "perimortem/core/reader/textual.hpp"
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
  Static::Vector<Int, 8> values = {{10, 1401, -120, 987109, 3, -10, 242, 5131}};

  Count i = 0;
  while (writer.get_location() < io_buffer.get_size() - 256) {
    writer << values[i++ & 0x7];
  }
  Count loc = writer.get_location();
  Benchmark::prevent_optimization(loc);
}

PERIMORTEM_BENCHMARK(TextualBench, write_bools) {
  Writer::Textual writer(io_buffer.get_access());
  Static::Vector<Bool, 8> values = {
    {True, False, True, False, False, True, False, True}};

  Count i = 0;
  while (writer.get_location() < io_buffer.get_size() - 256) {
    writer << values[i++ & 0x7];
  }
  Count loc = writer.get_location();
  Benchmark::prevent_optimization(loc);
}

PERIMORTEM_BENCHMARK(TextualBench, write_floats) {
  Writer::Textual writer(io_buffer.get_access());
  Static::Vector<Real_64, 8> values = {
    {1.0, -21.05, 781.012, 1200041.01, 2390.0037, 0.01234567, 890.098, 0}};

  Count i = 0;
  while (writer.get_location() < io_buffer.get_size() - 256) {
    writer << values[i++ & 0x7];
  }
  Count loc = writer.get_location();
  Benchmark::prevent_optimization(loc);
}

static Harness TextualReadBench = {
  .name = "Processing Text"_view,
};

PERIMORTEM_BENCHMARK(TextualReadBench, read_ints) {
  Static::Vector<View::Bytes, 8> values = {
    {"10"_view, "1401"_view, "-120"_view, "987109"_view, "3"_view, "-10"_view,
     "242"_view, "5131"_view}};
  Count accumulator = 0;
  for (Count i = 0; i < 256; i++) {
    Reader::Textual reader(values[i & 0x7]);
    accumulator += reader.read_int();
  }
  Benchmark::prevent_optimization(accumulator);
}

PERIMORTEM_BENCHMARK(TextualReadBench, read_bools) {
  Static::Vector<View::Bytes, 8> values = {{
    "true"_view, "false"_view, "tree"_view, "folly"_view,
    "true"_view, "false"_view, "tree"_view, "false"_view,
  }};
  Int accumulator = 0;
  for (Count i = 0; i < 256; i++) {
    Reader::Textual reader(values[i & 0x7]);
    Bool result = reader.read_flag();
    if (reader.is_valid()) {
      accumulator += result ? 1 : -1;
    }
  }
  Benchmark::prevent_optimization(accumulator);
}

PERIMORTEM_BENCHMARK(TextualReadBench, read_floats) {
  Static::Vector<View::Bytes, 8> values = {{
    "1.0"_view,
    "-21.05"_view,
    "781.012"_view,
    "1200041.01"_view,
    "2390.0037"_view,
    "0.01234567"_view,
    "890.098"_view,
    "0"_view,
  }};
  Real_64 accumulator = 0.0;
  for (Count i = 0; i < 256; i++) {
    Reader::Textual reader(values[i & 0x7]);
    accumulator += reader.read_real_64();
  }
  Benchmark::prevent_optimization(accumulator);
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
  constexpr bool values[] = {true,  false, true,  false,
                             false, true,  false, true};
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
  constexpr double values[] = {1.0,       -21.05,     781.012, 1200041.01,
                               2390.0037, 0.01234567, 890.098, 0.0};
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

static Harness TextualReadComp = {
  .name = "Processing Text"_view,
};

static auto cpp_charconv_read_ints() -> void {
  constexpr std::string_view values[] = {"10", "1401", "-120", "987109",
                                         "3",  "-10",  "242",  "5131"};
  Count accumulator = 0;
  for (Count i = 0; i < 256; i++) {
    int value = 0;
    auto sv = values[i & 0x7];
    std::from_chars(sv.data(), sv.data() + sv.size(), value);
    accumulator += Count(value);
  }
  Benchmark::prevent_optimization(accumulator);
}

static auto cpp_charconv_read_bools() -> void {
  constexpr std::string_view values[] = {
    "true", "false", "tree", "folly",
    "true", "false", "tree", "false"};
  Int accumulator = 0;
  for (Count i = 0; i < 256; i++) {
    auto sv = values[i & 0x7];
    if (sv == "true") {
      accumulator += 1;
    } else if (sv == "false") {
      accumulator += -1;
    }
  }
  Benchmark::prevent_optimization(accumulator);
}

static auto cpp_charconv_read_floats() -> void {
  constexpr std::string_view values[] = {
    "1.0",       "-21.05",     "781.012", "1200041.01",
    "2390.0037", "0.01234567", "890.098", "0"};
  Real_64 accumulator = 0.0;
  for (Count i = 0; i < 256; i++) {
    double value = 0.0;
    auto sv = values[i & 0x7];
    std::from_chars(sv.data(), sv.data() + sv.size(), value);
    accumulator += Real_64(value);
  }
  Benchmark::prevent_optimization(accumulator);
}

static Benchmark::Comparison read_ints_comp = {
  .harness = &TextualReadComp,
  .label = "read ints"_view,
  .variants = {{"textual"_view, "read_ints"_view}},
};
PERIMORTEM_COMPARISON(read_ints_comp) {
  cpp_charconv_read_ints();
}

static Benchmark::Comparison read_bools_comp = {
  .harness = &TextualReadComp,
  .label = "read bools"_view,
  .variants = {{"textual"_view, "read_bools"_view}},
};
PERIMORTEM_COMPARISON(read_bools_comp) {
  cpp_charconv_read_bools();
}

static Benchmark::Comparison read_floats_comp = {
  .harness = &TextualReadComp,
  .label = "read floats"_view,
  .variants = {{"textual"_view, "read_floats"_view}},
};
PERIMORTEM_COMPARISON(read_floats_comp) {
  cpp_charconv_read_floats();
}

#endif  // PERI_BENCH_CPP
